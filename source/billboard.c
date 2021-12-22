#include <psptypes.h>
#include <pspgu.h>
#include <pspgum.h>
#include "billboard.h"
#include "imageloader.h"

void gaasGFXBillboard(ScePspFVector3 pos, gaasImage* source, float scale, unsigned int color, ScePspFMatrix4 viewMatrix) {
	if(source==NULL) return;
	struct Vertex {
		float u,v;
		unsigned int color;
		float x,y,z;
	} *billboard = (struct Vertex*) sceGuGetMemory(4 * sizeof(struct Vertex));

	ScePspFMatrix4 LoadedViewMatrix;

	billboard[0].u = 0;
	billboard[0].v = 0;
	billboard[0].color = color;
	billboard[0].x = -1*scale;
	billboard[0].y = 0;
	billboard[0].z = -1*scale;

	billboard[1].u = 1;
	billboard[1].v = 0;
	billboard[1].color = color;
	billboard[1].x = -1*scale;
	billboard[1].y = 0;
	billboard[1].z = 1*scale;

	billboard[2].u = 0;
	billboard[2].v = 1;
	billboard[2].color = color;
	billboard[2].x = 1*scale;
	billboard[2].y = 0;
	billboard[2].z = -1*scale;

	billboard[3].u = 1;
	billboard[3].v = 1;
	billboard[3].color = color;
	billboard[3].x = 1*scale;
	billboard[3].y = 0;
	billboard[3].z = 1*scale;

	sceGumMatrixMode(GU_MODEL);
	sceGumLoadIdentity();
	{
		ScePspFVector3 rot = { 1.5708, 0, 1.5708 };
		ScePspFVector3 posinv = {-pos.x, -pos.y, -pos.z};

		LoadedViewMatrix=viewMatrix;

		// it just works
		//stupid problems require stupid solutions
		LoadedViewMatrix.w.x = 0.0f;
		LoadedViewMatrix.w.y = 0.0f;
		LoadedViewMatrix.w.z = 0.0f;

		sceGumRotateXYZ(&rot);
		sceGumMultMatrix(&LoadedViewMatrix);
		sceGumTranslate(&posinv);

		sceGumFullInverse();
	}
	
	sceGuTexMode(source->format,0,0,source->swizzled);
	sceGuTexImage(0, source->tw, source->th, source->tw, source->data);
	sceGuTexWrap(GU_REPEAT, GU_REPEAT);
	sceGuTexFunc(GU_TFX_ADD,GU_TCC_RGBA);
	sceGuTexFilter(GU_LINEAR,GU_LINEAR);
	sceGuTexScale(1.0f, 1.0f);
	sceGuTexOffset(0.0f,0.0f);
	
	sceGumDrawArray(GU_TRIANGLE_STRIP,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D,4,0,billboard);
}

void gaasGFXAnimatedBillboard(ScePspFVector3 pos, gaasImage* source, int collumns, int rows, float scale, unsigned int color, ScePspFMatrix4 viewMatrix, float update) {
	if(source==NULL) return;
	struct Vertex {
		float u,v;
		unsigned int color;
		float x,y,z;
	} *billboard = (struct Vertex*) sceGuGetMemory(4 * sizeof(struct Vertex));

	ScePspFMatrix4 LoadedViewMatrix;

	static int up;
	static int row;
	static int col;

	billboard[0].u = 0;
	billboard[0].v = 0;
	billboard[0].color = color;
	billboard[0].x = -1*scale;
	billboard[0].y = 0;
	billboard[0].z = -1*scale;

	billboard[1].u = 1;
	billboard[1].v = 0;
	billboard[1].color = color;
	billboard[1].x = -1*scale;
	billboard[1].y = 0;
	billboard[1].z = 1*scale;

	billboard[2].u = 0;
	billboard[2].v = 1;
	billboard[2].color = color;
	billboard[2].x = 1*scale;
	billboard[2].y = 0;
	billboard[2].z = -1*scale;

	billboard[3].u = 1;
	billboard[3].v = 1;
	billboard[3].color = color;
	billboard[3].x = 1*scale;
	billboard[3].y = 0;
	billboard[3].z = 1*scale;

	sceGumMatrixMode(GU_MODEL);
	sceGumLoadIdentity();
	{
		ScePspFVector3 rot = { 1.5708, 0, 1.5708 };

		LoadedViewMatrix=viewMatrix;

		// it just works
		//stupid problems require stupid solutions
		LoadedViewMatrix.w.x = 0.0f;
		LoadedViewMatrix.w.y = 0.0f;
		LoadedViewMatrix.w.z = 0.0f;

		sceGumRotateXYZ(&rot);
		sceGumMultMatrix(&LoadedViewMatrix);
		sceGumTranslate(&pos);

		sceGumFullInverse();
	}
	
	sceGuTexMode(source->format,0,0,source->swizzled);
	sceGuTexImage(0, source->tw, source->th, source->tw, source->data);
	sceGuTexWrap(GU_CLAMP, GU_CLAMP);
	sceGuTexFunc(GU_TFX_ADD,GU_TCC_RGBA);
	sceGuTexFilter(GU_LINEAR,GU_LINEAR);
	sceGuTexScale((float)1.0/collumns, (float)1.0/rows);
	sceGuTexOffset(((float)1.0/collumns)*col,((float)1.0/rows)*row);

	sceGumDrawArray(GU_TRIANGLE_STRIP,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D,4,0,billboard);

	if(up<update) {
		up++;
	} else {
		col++;
		up=0;
	}
	
	if (col >= collumns) {
		row++;
		col = 0;
	}

	if (row>=rows){
		col = 0;
		row = 0;
	}
}