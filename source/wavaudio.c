#include <pspkernel.h>
#include <pspaudio.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "wavaudio.h"
#include "collision.h"

#define GAAS_NUM_AUDIO_CHANNELS 8
#define GAAS_NUM_AUDIO_SAMPLES 512
#define GAAS_VOLUME_MAX 0x8000

#define AUDIOBUF_SIZE	(16*GAAS_NUM_AUDIO_SAMPLES)
#define BUF_COUNT		(4)

enum {
	RIFF_ID	= 'FFIR',	//RIFF
	WAVE_ID	= 'EVAW',	//WAVE
	FMT_ID	= ' tmf',	//fmt<space>
	DATA_ID	= 'atad'	//data
};

struct gaasAudioChannel {
	int fd;
	SceUID threadhandle;
	int channel;
	int newOffset;
	int killThread;
	gaasWAVSound* sound;
	char audioBuf[BUF_COUNT][AUDIOBUF_SIZE] __attribute__((aligned(64)));
};

struct gaasAudioChannel AudioStatus[8];

int wav_player(SceSize args, void *argp);

static int seekChunk(int fd, int size, u32 id) {

	typedef struct {
		u32 id;
		u32 size;
	} ChunkHeader;
	while (size > 0) {
		ChunkHeader header;
		sceIoRead(fd, &header, sizeof(header));
		if (header.id == id) {
			int offset = -sizeof(header);
			sceIoLseek(fd, offset, PSP_SEEK_CUR);
			return 1;
		}

		int offset = sceIoLseek(fd, header.size, PSP_SEEK_CUR);
		if (offset >= size) {
			break;
		}
	}

	return 0;
}

gaasWAVSound* gaasWAVLoad(const char *file, int usesoffset, int fileoffset) {
    int fd = sceIoOpen(file, PSP_O_RDONLY, 0777);
	gaasWAVSound* temp = (gaasWAVSound*)malloc(sizeof(gaasWAVSound));

	if(usesoffset==1) {
		sceIoLseek(fd, fileoffset, PSP_SEEK_SET);
	}

	if (fd < 0) {
		printf("Error opening file %s at offset: %d\n", file, fileoffset);
		return NULL;
	}

	sceIoRead(fd, &temp->riffHeader, sizeof(temp->riffHeader));
	if (temp->riffHeader.riffId != RIFF_ID || temp->riffHeader.waveId != WAVE_ID) {
		printf("file not riff-wave %s\n", file);
		sceIoClose(fd);
		return NULL;
	}
	if (!seekChunk(fd, temp->riffHeader.size, FMT_ID)) {
		printf("unable to find fmt header %s\n", file);
		sceIoClose(fd);
		return NULL;
	}
	
	sceIoRead(fd, &temp->fmtHeader, sizeof(temp->fmtHeader));
	if (temp->fmtHeader.fmtId != FMT_ID || temp->fmtHeader.format != 0x0001) {
		printf("file not correct format %s\n", file);
		printf("format = 0x%x\n", temp->fmtHeader.format);
		sceIoClose(fd);
		return NULL;
	}
	if (!seekChunk(fd, temp->riffHeader.size, DATA_ID)) {
		printf("unable to find data header %s\n", file);
		sceIoClose(fd);
		return NULL;
	}
	
	sceIoRead(fd, &temp->dataHeader, sizeof(temp->dataHeader));
	if (temp->dataHeader.dataId != DATA_ID) {
		printf("could not find 'data'-tag %s\n", file);
		sceIoClose(fd);
		return NULL;
	}
	
	temp->info.bytesPerSample = temp->fmtHeader.bitsPerSample / 8;
	temp->info.numChannels = temp->fmtHeader.numChannels;
	temp->info.bytesPerBlock = temp->info.bytesPerSample * temp->fmtHeader.numChannels;
	temp->info.frequency = temp->fmtHeader.samplesPerSec;
	temp->info.dataOffset = sceIoLseek(fd, 0, PSP_SEEK_CUR);
	temp->info.dataLength = temp->dataHeader.chunkSize;

	temp->volumeLeft = 0x8000;
	temp->volumeRight = 0x8000;

	temp->free=0;
	temp->paused=0;

	temp->data = malloc(temp->info.dataLength);

	temp->loop = 0;

	int size = temp->info.dataLength;
	sceIoLseek(fd, temp->info.dataOffset, SEEK_SET);
	sceIoRead(fd, temp->data, size);

	strcpy(temp->filename, file);
	sceIoClose(fd);

	return temp;
}

int gaasWAVGetSoundChannel(gaasWAVSound *s) {
	int i;
	for (i=0; i<GAAS_NUM_AUDIO_CHANNELS; i++) {
		if (AudioStatus[i].sound == s) {
			return i;
		}
	}
	return -1;
}

void gaasWAVStop(gaasWAVSound* s) {
	s->free = 1;
}

void gaasWAVFree(gaasWAVSound* s) {
	s->free = 1;
	sceKernelDelayThread(500);//delay the main thread to give the audio thread time to shut down
	free(s->data);
	free(s);
	memset(&s, 0, sizeof(gaasWAVSound));
}

void gaasWAVSetLoop(gaasWAVSound* s, int loop) {
	s->loop = loop;
}

void gaasWAVSetPause(gaasWAVSound* s, int pause) {
	s->paused = pause;
}

void gaasWAVSetVolume(gaasWAVSound* s, u16 volleft, u16 volright) {
	if(volleft>GAAS_VOLUME_MAX) {
		s->volumeLeft = GAAS_VOLUME_MAX;
	} else if (volleft<0) {
		s->volumeLeft = 0;
	} else {
		s->volumeLeft = volleft;
	}
	if(volright>GAAS_VOLUME_MAX) {
		s->volumeRight = GAAS_VOLUME_MAX;
	} else if (volright<0) {
		s->volumeRight = 0;
	} else {
		s->volumeRight = volright;
	}
}

void gaasWAVInit() {
	char str[32];

	strcpy(str,"wav_thread_0");
	for(int i=0; i<GAAS_NUM_AUDIO_CHANNELS; i++) {
		str[11]='0'+i;

		AudioStatus[i].channel = sceAudioChReserve(i, GAAS_NUM_AUDIO_SAMPLES, PSP_AUDIO_FORMAT_STEREO);
	
		AudioStatus[i].threadhandle = sceKernelCreateThread(str, wav_player, 0x12, 0x10000, PSP_THREAD_ATTR_USER, NULL);
	
		if (AudioStatus[i].threadhandle < 0) {
			printf("Error creating %s\n", str);
			return;
		}
	}
}

void gaasWAVEnd() {
	for(int i=0; i<GAAS_NUM_AUDIO_CHANNELS; i++) {
		sceAudioChRelease(AudioStatus[i].channel);

		sceKernelDeleteThread(AudioStatus[i].threadhandle);
	}
}

/**
 * you should always check if gaasWAVGetSoundChannel returns -1 before playing a sound
 * also make sure no other sounds are being played on the same channel
**/
void gaasWAVPlay(int channel, gaasWAVSound* sound) {
	AudioStatus[channel].sound = sound;

	sceKernelStartThread(AudioStatus[channel].threadhandle, sizeof(channel), &channel); 
}

void gaasWAVCalc3D(gaasWAVSound* sound, float range, struct ScePspFVector3 src, struct ScePspFVector3 dst) {
	float distance = gaasCOLVectorDistance(src, dst);
	float volume;
	if(distance<range) {
		gaasWAVSetPause(sound, 0);
		volume = distance/range;
		volume = 1.0-volume;
		volume = volume*GAAS_VOLUME_MAX;
	} else {
		gaasWAVSetPause(sound, 1);
	}

	gaasWAVSetVolume(sound, volume, volume);
}

int wav_player(SceSize args, void *argp) {
    int chan =*(int *)argp;

	if (AudioStatus[chan].sound->info.bytesPerSample != 16 /*bits*/ / 8 /*bits per byte*/|| 
		AudioStatus[chan].sound->info.numChannels != 2 /*channels stereo*/ ||
		AudioStatus[chan].sound->info.bytesPerBlock != 16 /*bits*/ * 2 /*channels stereo*/ / 8 /*bits per byte*/ ||
		AudioStatus[chan].sound->info.frequency != 44100 /*Hz*/)
    {
		printf("file needs to be 16 bits stereo 44.1 kHz\n");
		printf("bytesPerSample = %i\n", AudioStatus[chan].sound->info.bytesPerSample);
		printf("numChannels = %i\n", AudioStatus[chan].sound->info.numChannels);
		printf("bytesPerBlock = %i\n", AudioStatus[chan].sound->info.bytesPerBlock);
		printf("frequency = %i\n", AudioStatus[chan].sound->info.frequency);
		printf("dataOffset = %i\n", AudioStatus[chan].sound->info.dataOffset);
		printf("dataLength = %i\n", AudioStatus[chan].sound->info.dataLength);
		return -1;
	}

	sceAudioSetChannelDataLen(AudioStatus[chan].channel, GAAS_NUM_AUDIO_SAMPLES);
	
	int size = AudioStatus[chan].sound->info.dataLength;
	int buf = 0;
	int offset = 0;
	while(offset != size) {
		if(AudioStatus[chan].sound->free==1) {
			break;
		}
		if(AudioStatus[chan].sound->paused==0) {
			int nextbuf = (buf+1) & (BUF_COUNT-1);
			memcpy(AudioStatus[chan].audioBuf[nextbuf], &AudioStatus[chan].sound->data[offset], AUDIOBUF_SIZE);

			for(int i = 0; i < AUDIOBUF_SIZE; i+=GAAS_NUM_AUDIO_SAMPLES*4) {
				sceAudioOutputPannedBlocking(AudioStatus[chan].channel, AudioStatus[chan].sound->volumeLeft, AudioStatus[chan].sound->volumeRight, &AudioStatus[chan].audioBuf[buf][i]);
        	}

			buf = nextbuf;

			AudioStatus[chan].newOffset+=AUDIOBUF_SIZE;
			if (AudioStatus[chan].newOffset) {
				offset += AudioStatus[chan].newOffset;
				AudioStatus[chan].newOffset = 0;
				offset &= ~0x4;
				if (offset < 0) {
					offset = 0;
        	    } else if(offset > (s32)AudioStatus[chan].sound->info.dataLength) {
					offset = AudioStatus[chan].sound->info.dataLength;
        	    }
			}

			if(offset==size && AudioStatus[chan].sound->loop==1) {
				offset=0;
				buf=0;
				nextbuf = 0;
				memcpy(AudioStatus[chan].audioBuf[0], &AudioStatus[chan].sound->data[offset], AUDIOBUF_SIZE);
			}
		} else {
			sceKernelDelayThread(100);//prevents thread from locking up when audio is paused
        }
	}

	for(int i=0; i<BUF_COUNT; i++) {
		memset(AudioStatus[chan].audioBuf[i], 0, AUDIOBUF_SIZE);
	}

	AudioStatus[chan].sound = NULL;
    sceKernelExitThread(0);

	return 0;
}