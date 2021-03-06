#ifndef CTRL
#define CTRL

#include <psptypes.h>
#include <pspctrl.h>
#include <stddef.h>

/**
 * Initializes input system
 * Has to be run once
 * sampling_cycle - set to 0 to sample input on every vblank period
**/
void gaasCTRLInit(int sampling_cycle);

/**
 * Checks for overall input
 * Has to be run at the start of every frame
**/
void gaasCTRLSampleInput();

/**
 * Checks if a button has been pressed
 * button - pspctrl.h PspCtrlButtons enum
**/
int gaasCTRLCheckButtonPressed(int button);

/**
 * returns last pressed button
**/
int gaasCTRLGetButtonPressed();

/**
 * get analog stick position on the X axis
**/
int gaasCTRLGetAnalogX();

/**
 * get analog stick position on the Y axis
**/
int gaasCTRLGetAnalogY();

#endif