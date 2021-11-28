#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <stdio.h>
#include <psputility.h>

int AVModulesInited = 0;

void gaasLoadAVModules() {
    if(AVModulesInited==0) {
	    sceUtilityLoadModule(PSP_MODULE_AV_AVCODEC);
	    sceUtilityLoadModule(PSP_MODULE_AV_MP3);
	    sceUtilityLoadModule(PSP_MODULE_AV_MPEGBASE);
        AVModulesInited=1;
    } else {
        printf("AV Modules already initialized\n");
    }
}

void gaasUnloadAVModules() {
    if(AVModulesInited==1) {
	    sceUtilityUnloadModule(PSP_MODULE_AV_MPEGBASE);
	    sceUtilityUnloadModule(PSP_MODULE_AV_MP3);
	    sceUtilityUnloadModule(PSP_MODULE_AV_AVCODEC);
        AVModulesInited=0;
    } else {
        printf("AV Modules not initialized\n");
    }
}