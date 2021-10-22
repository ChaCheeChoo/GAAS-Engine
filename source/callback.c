#include <psptypes.h>
#include <pspthreadman.h>
#include <psppower.h>
#include <pspkernel.h>

#include "callback.h"

static int exitRequest = 0;

int gaasCLBIsRunning() { 
	return !exitRequest; 
}

int exitCallback(int arg1, int arg2, void *common) { 
	exitRequest = 1; 
	return 0; 
}

int callbackThread(SceSize args, void *argp) { 
	int callbackID;

	callbackID = sceKernelCreateCallback("exit_callback", exitCallback, NULL); 
	sceKernelRegisterExitCallback(callbackID);

	sceKernelSleepThreadCB();

	return 0; 
}

int gaasCLBSetupExitCallback() {
	int threadID = 0;

	threadID = sceKernelCreateThread("callback_thread", callbackThread, 0x11, 0xFA0, THREAD_ATTR_USER, 0); 
	 
	if(threadID >= 0) { 
		sceKernelStartThread(threadID, 0, 0); 
	}

	return threadID;
}