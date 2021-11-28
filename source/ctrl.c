#include <pspctrl.h>
#include "ctrl.h"

SceCtrlData Input; 

void gaasCTRLSampleInput(int sampling_cycle) {
    sceCtrlSetSamplingCycle(sampling_cycle);
    sceCtrlPeekBufferPositive(&Input, 1); 
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
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