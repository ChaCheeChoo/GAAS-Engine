#ifndef CALLBACK
#define CALLBACK

#include <psptypes.h>
#include <stddef.h>

/**
 * Sets up Exit Callback
 * run once at game launch
 * Exit Callbacks are used so that you can exit the game properly
**/
int gaasCLBSetupExitCallback();

/**
 * Checks if the callback hasn't been called
 * returns:
 *      1: Game is running
 *      0: Exit requested
 * 
 * in your games while loop check if IsRunning returns 1, if it returns 0 game stops running and deinitialization functions get run
**/
int gaasCLBIsRunning();

#endif