#include <pspgu.h>
#include <pspgum.h>
#include "lighting.h"

void gaasLIGHTSetAmbient(unsigned int color) {
	sceGuAmbient(color);
}

void gaasLIGHTDirectionalLight(int light, ScePspFVector3 pos, unsigned int color) {
	sceGuLight(light, GU_DIRECTIONAL, GU_DIFFUSE, &pos);
	sceGuLightColor(light, GU_DIFFUSE, color);
	sceGuLightAtt(light, 0,1.0f,0);
}

void gaasLIGHTPointLight(int light, ScePspFVector3 pos, unsigned int color, unsigned int speccolor) {
	sceGuLight(light, GU_POINTLIGHT, GU_DIFFUSE_AND_SPECULAR, &pos);
	sceGuLightColor(light, GU_DIFFUSE, color);
	sceGuLightColor(light, GU_SPECULAR, speccolor);
	sceGuLightAtt(light, 0,1.0f,0); 
	sceGuSpecular(8.0f);
}

void gaasLIGHTSpotLight(int light, ScePspFVector3 pos, ScePspFVector3 dir, unsigned int color, unsigned int speccolor, float exponent, float cutoff) {
	sceGuLight(light, GU_SPOTLIGHT, GU_DIFFUSE_AND_SPECULAR, &pos);
	sceGuLightSpot(light, &dir, exponent, cutoff);//1.22173f
	sceGuLightColor(light, GU_DIFFUSE, color);
	sceGuLightColor(light, GU_SPECULAR, speccolor);
	sceGuLightAtt(light, 0,1.0f,0); 
	sceGuSpecular(8.0f);
} 