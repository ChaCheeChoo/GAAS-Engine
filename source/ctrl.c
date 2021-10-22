#include <pspctrl.h>
#include "ctrl.h"

SceCtrlData Input; 
 
int gaasCTRLCheckButtonPressed(int button) { 
    sceCtrlSetSamplingCycle(0);
    sceCtrlPeekBufferPositive(&Input, 1); 
    if (Input.Buttons & button) {
        return 1; 
    } else {
        return 0;
    } 
}

int gaasCTRLGetButtonPressed() {
    sceCtrlSetSamplingCycle(0);
    sceCtrlPeekBufferPositive(&Input, 1); 
    return Input.Buttons;
}

int gaasCTRLGetAnalogX() {
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
    return Input.Lx;
}

int gaasCTRLGetAnalogY() {
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG); 
    return Input.Ly;
}