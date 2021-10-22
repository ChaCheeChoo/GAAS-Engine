#ifndef CTRL
#define CTRL

#include <psptypes.h>
#include <pspctrl.h>
#include <stddef.h>

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