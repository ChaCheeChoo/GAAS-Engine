#include <pspctrl.h>
#include "ctrl.h"

SceCtrlData Input; 

void gaasCTRLInit(int sampling_cycle) {
    sceCtrlSetSamplingCycle(sampling_cycle);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
}

void gaasCTRLSampleInput() {
    sceCtrlPeekBufferPositive(&Input, 1); 
}
 
int gaasCTRLCheckButtonPressed(int button) { 
    if (Input.Buttons & button) {
        return 1; 
    } else {
        return 0;
    } 
}

int gaasCTRLGetButtonPressed() {
    return Input.Buttons;
}

int gaasCTRLGetAnalogX() {
    return Input.Lx;
}

int gaasCTRLGetAnalogY() {
    return Input.Ly;
}