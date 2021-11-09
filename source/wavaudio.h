#ifndef WAVAUDIO
#define WAVAUDIO

#include <psptypes.h>
#include <stddef.h>

typedef struct RIFFHeaderTag {
	u32	riffId;
	u32	size;
	u32	waveId;
} RIFFHeader;

typedef struct FMTHeaderTag {
	u32	fmtId;
	u32	chunkSize;
	u16	format;
	u16	numChannels;
	u32	samplesPerSec;
	u32	avgBytesPerSec;
	u16	blockAlign;
	u16	bitsPerSample;
} FMTHeader;

typedef struct DATAHeaderTag {
	u32	dataId;
	u32	chunkSize;
} DATAHeader;

typedef struct {
	u16 bytesPerSample;
	u16 numChannels;
	u32 bytesPerBlock;
	u32 frequency;
	u32 dataOffset;
	u32 dataLength;
} WaveInfo;

typedef struct gaasWAVSound {
    RIFFHeader riffHeader;
    FMTHeader fmtHeader;
    DATAHeader dataHeader;
    WaveInfo info;
	int loop;
	u16 volumeLeft;
	u16 volumeRight;
	int paused;
	int free;
    char filename[256];
    void* data;
} gaasWAVSound;

/**
 * Initializes wav audio and sets up 8 audio channels and threads
**/
void gaasWAVInit();

/**
 * Deinitializes wav audio and releases 8 audio channels and threads
**/
void gaasWAVEnd();

/**
 * Loads a wav file into memory
 * NOTE: files need to be 16 bits stereo 44.1 kHz
 * file - The file to load
 * usesoffset - flag that tells the loader wether to load from a specific file or from the PSAR in EBOOT
 *      if latter: file needs to be "./EBOOT.PBP"
 * fileoffset - location of file in EBOOT, only used when usesoffset==1
 * 
 * returns: valid gaasWAVSound struct
**/
gaasWAVSound* gaasWAVLoad(const char *file, int usesoffset, int fileoffset);

/**
 * Returns audio channel id of given sound
 * Returns -1 if given sound isn't playing
 * s - valid gaasWAVSound struct
**/
int gaasWAVGetSoundChannel(gaasWAVSound *s);

/**
 * Requests given sound to stop playing
 * s - valid gaasWAVSound struct
**/
void gaasWAVStop(gaasWAVSound* s);

/**
 * Stops and free's given sound
 * s - valid gaasWAVSound struct
**/
void gaasWAVFree(gaasWAVSound* s);

/**
 * Sets whether sound loops or not
 * s - valid gaasWAVSound struct
 * loop - either 0 or 1
**/
void gaasWAVSetLoop(gaasWAVSound* s, int loop);

/**
 * Sets whether sound is paused or not
 * s - valid gaasWAVSound struct
 * pause - either 0 or 1
**/
void gaasWAVSetPause(gaasWAVSound* s, int pause);

/**
 * Sets given sound's volume
 * s - valid gaasWAVSound struct
 * volleft - left channel volume
 * volright - right channel volume
 * 
 * for some reason volumes less than 16000 end up playing on the opposite channel (only on ppsspp)
**/
void gaasWAVSetVolume(gaasWAVSound* s, u16 volleft, u16 volright);

/**
 * Updates volume of given sound depending on the distance between src and dst
 * s - valid gaasWAVSound struct
 * range - size of region where you start hearing sound, if distance is more than this sound won't be heard
 * src - vector3 position of where the sound is
 * dst - vector3 position of listener
**/
void gaasWAVCalc3D(gaasWAVSound* sound, float range, struct ScePspFVector3 src, struct ScePspFVector3 dst);

/**
 * Plays given sound
 * channel - audio channel to play sound on (0-7)
 * s - valid gaasWAVSound struct
**/
void gaasWAVPlay(int channel, gaasWAVSound* sound);

#endif