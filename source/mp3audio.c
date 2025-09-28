#include <pspkernel.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <pspdisplay.h>
#include <stdio.h>
#include <pspaudio.h>
#include <pspmp3.h>
#include <psputility.h>
#include "mp3audio.h"
#include "common.h"

#define GAAS_NUM_AUDIO_CHANNELS 8
#define GAAS_NUM_AUDIO_SAMPLES 512
#define GAAS_VOLUME_MAX 0x8000

/**
 * Note that the mp3 system will not continue playing if you turn your psp off and on, meaning you would have to manually restart it
 * There is also a small chance that the game will crash if you turn off the psp while it's reading the next mp3 chunk (I think)
**/

struct gaasCurrentMP3 {
    int fd;
    SceMp3InitArg mp3Init;
    int volume;
    int playing;
    int threadhandle;
	int handle;
	int paused;
	int loop;
	int status;
	int reservedonce;
};

struct gaasCurrentMP3 curmp3;

unsigned char mp3Buf[16*1024]  __attribute__((aligned(64)));
unsigned short pcmBuf[16*(1152/2)]  __attribute__((aligned(64)));

int fillStreamBuffer(int fd, int handle) {
	unsigned char* dst;
	long int write;
	long int pos;

	curmp3.status = sceMp3GetInfoToAddStreamData(handle, &dst, &write, &pos); 

	sceIoLseek32(fd, pos, SEEK_SET);
	int read = sceIoRead(fd, dst, write);
	
	if (read==0) {
		return 0;
	}
	
	curmp3.status = sceMp3NotifyAddStreamData(handle, read);
	return (pos>0);
}

int mp3_player(SceSize args, void *argp) {
	//Mp3 File Buf
	curmp3.status = sceMp3InitResource();
	curmp3.handle = sceMp3ReserveMp3Handle(&curmp3.mp3Init);

	if (curmp3.reservedonce==1) {
		sceAudioSRCChRelease();
	}
	
	// Fill the stream buffer with some data so that sceMp3Init has something to work with
	fillStreamBuffer(curmp3.fd, curmp3.handle);
	
	curmp3.status = sceMp3Init(curmp3.handle);
	printf("MP3 init %d\n", curmp3.status);
	
	int channel = -1;
	int samplingRate = sceMp3GetSamplingRate(curmp3.handle);
	int numChannels = sceMp3GetMp3ChannelNum(curmp3.handle);
	int lastDecoded = 0;
	int numPlayed = 0;

	curmp3.status = sceMp3SetLoopNum(curmp3.handle, curmp3.loop);

	while (curmp3.playing==1 && curmp3.status==0) {
		// Check if we need to fill our stream buffer
		if (sceMp3CheckStreamDataNeeded( curmp3.handle)>0){
			fillStreamBuffer(curmp3.fd, curmp3.handle);
		}

		// Decode some samples
		short* buf;
		int bytesDecoded;
		// We retry in case it's just that we reached the end of the stream and need to loop
		for (int retries = 0; retries<1; retries++){
			bytesDecoded = sceMp3Decode(curmp3.handle, &buf);
			if (bytesDecoded>0) {
				break;
			} else if(bytesDecoded<0) {
				curmp3.status = -999;
			}
			
			if (sceMp3CheckStreamDataNeeded(curmp3.handle)<=0) {
				break;
			}
			
			if (!fillStreamBuffer(curmp3.fd, curmp3.handle)){
				numPlayed = 0;
			}
		}
		
		// Called when audio has played for the specified number of loops (curmp3.loop)
		if (bytesDecoded==0 || bytesDecoded==0x80671402){
			numPlayed = 0;
			printf("MP3 over\n");
			curmp3.status=1;
			curmp3.playing=0;
		} else {
			if (channel < 0 && lastDecoded!=bytesDecoded){
				channel = sceAudioSRCChReserve(bytesDecoded/(2*numChannels), samplingRate, numChannels);
				curmp3.reservedonce = 1;
				printf("After reserve %d\n", channel);
			}
			numPlayed += sceAudioSRCOutputBlocking(curmp3.volume, buf);
		}
    }
	sceMp3ReleaseMp3Handle(curmp3.handle);
	sceMp3TermResource();
	sceIoClose(curmp3.fd);
	printf("MP3 stopped %d\n", curmp3.status);
	sceKernelExitThread(0);
	return 0;
}

void gaasMP3Init() {
	gaasLoadAVModules();

	curmp3.status = 1;
	curmp3.threadhandle = sceKernelCreateThread("mp3_thread", mp3_player, 0x12, 0x10000, PSP_THREAD_ATTR_USER, NULL);
}

void gaasMP3Load(const char* filename, int usesoffset, int offset, int size) {
	curmp3.fd = sceIoOpen(filename, PSP_O_RDONLY, 0777);

	if(usesoffset==0) {
		curmp3.mp3Init.mp3StreamStart = sceIoLseek32(curmp3.fd, 0, SEEK_SET);
		curmp3.mp3Init.mp3StreamEnd = sceIoLseek32(curmp3.fd, 0, SEEK_END);
	} else {
		curmp3.mp3Init.mp3StreamStart = sceIoLseek32(curmp3.fd, offset, SEEK_SET);
		curmp3.mp3Init.mp3StreamEnd = sceIoLseek32(curmp3.fd, offset+size, SEEK_SET);
	}
	curmp3.mp3Init.mp3Buf = mp3Buf;
	curmp3.mp3Init.mp3BufSize = sizeof(mp3Buf);
	curmp3.mp3Init.pcmBuf = (unsigned char*)pcmBuf;
	curmp3.mp3Init.pcmBufSize = sizeof(pcmBuf);
}

void gaasMP3Pause() {
	if (curmp3.paused == 0) {
		sceKernelSuspendThread(curmp3.threadhandle);
		curmp3.paused = 1;
	}
	return;
}

void gaasMP3Unpause() {
	if (curmp3.paused == 1) {
		sceKernelResumeThread(curmp3.threadhandle);
		curmp3.paused = 0;
	}
	return;
}

void gaasMP3End() {
	sceKernelDeleteThread(curmp3.threadhandle);

	gaasUnloadAVModules();

	return; 
}

void gaasMP3Play(int loop) {
	curmp3.volume = GAAS_VOLUME_MAX;
	curmp3.playing = 1;
	curmp3.loop = loop;
	curmp3.paused = 0;
	curmp3.status = 0;
	sceKernelStartThread(curmp3.threadhandle, 0, 0);
}

void gaasMP3SetVolume(int vol) {
	if(vol>GAAS_VOLUME_MAX) vol = GAAS_VOLUME_MAX;
	curmp3.volume = vol;
}

int gaasMP3GetStatus() {
	return curmp3.status;
}

void gaasMP3Stop() {
	if (curmp3.paused == 1) {
		sceKernelResumeThread(curmp3.threadhandle);
	}
	curmp3.status = 1;
	curmp3.playing = 0;
}