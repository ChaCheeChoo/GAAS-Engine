#ifndef MP3AUDIO
#define MP3AUDIO

#include <psptypes.h>
#include <stddef.h>

/**
 * initializes mp3 system
**/
void gaasMP3Init();

/**
 * shuts down mp3 system
**/
void gaasMP3End();

/**
 * Loads an mp3 file for streaming
 * filename - The file to load
 * usesoffset - flag that tells the loader wether to load from a specific file or from the PSAR in EBOOT
 *      if latter: filename needs to be "./EBOOT.PBP"
 * offset - location of file in EBOOT, only used when usesoffset==1
 * size - size of file in EBOOT, only used when usesoffset==1
**/
void gaasMP3Load(const char* filename, int usesoffset, int offset, int size);

/**
 * Pauses current song
**/
void gaasMP3Pause();

/**
 * Changes volume of current song
 * vol - volume (0x0000 - 0x8000)
**/
void gaasMP3SetVolume(int vol);

/**
 * Unpauses current song
**/
void gaasMP3Unpause();

/**
 * Starts playing loaded song
 * loop - how many times to loop (0 - play once; 1 - play twice; ...)
**/
void gaasMP3Play(int loop);

/**
 * Gets status of current song
 *      returns:
 *          0: audio playing
 *          1: audio stopped
 *          <0: sceMp3 error
**/
int gaasMP3GetStatus();

/**
 * Stops MP3 File
**/
void gaasMP3Stop();

#endif