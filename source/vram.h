#ifndef VRAM
#define VRAM

#ifdef __cplusplus
extern "C" {
#endif

/**
 * make a static allocation of vram memory and return pointer relative to vram start 
**/
void* gaasVRAMGetStaticVramBuffer(unsigned int width, unsigned int height, unsigned int psm);

/**
 * make a static allocation of vram memory and return absolute pointer 
**/
void* gaasVRAMGetStaticVramTexture(unsigned int width, unsigned int height, unsigned int psm);

#ifdef __cplusplus
}
#endif

#endif
