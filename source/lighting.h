#ifndef LIGHTING
#define LIGHTING

#include <psptypes.h>
#include <stddef.h>

/**
 * NOTE: GU_LIGHTING must be enabled for this to work along with GU_LIGHT0, 1, ... depending on which ones you want to use
**/

/**
 * Sets the ambient light color
 * This is the color used on polygons that don't get hit by any lights
 * color - ambient color
**/
void gaasLIGHTSetAmbient(unsigned int color);

/**
 * Sets a Directional Light
 * Directional Lights are lights that hit everything in the scene from an angle
 * angle is determined by drawing an invisible line from pos to 0, 0, 0
 * light - which GU_LIGHT to use
 * pos - 3d postion of light
 * color - light color
**/
void gaasLIGHTDirectionalLight(int light, ScePspFVector3 pos, unsigned int color);

/**
 * NOTE: I wouldn't recommend using the functions below since they're pretty limited, you can't set light power and specular power for example
 * if you need to use point or spot lights set them up manually
**/

/**
 * Sets a Point Light
 * light - which GU_LIGHT to use
 * pos - 3d postion of light
 * color - light color
 * speccolor - specular color
**/
void gaasLIGHTPointLight(int light, ScePspFVector3 pos, unsigned int color, unsigned int speccolor);

/**
 * Sets a Spotlight
 * light - which GU_LIGHT to use
 * pos - 3d postion of light
 * dir - not sure if this is rotation or point that the light will aim towards
 * color - light color
 * speccolor - specular color
 * exponent - ¯\_(ツ)_/¯ I have no idea
 * cutoff - spotlight cutoff angle
**/
void gaasLIGHTSpotLight(int light, ScePspFVector3 pos, ScePspFVector3 dir, unsigned int color, unsigned int speccolor, float exponent, float cutoff);

#endif