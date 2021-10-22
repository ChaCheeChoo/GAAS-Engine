#include <pspgu.h>
#include <pspgum.h>
#include <pspmath.h>
#include <stdio.h>

#include "imageloader.h"
#include "graphics.h"

#define BUF_WIDTH (512)
#define SCR_WIDTH (480)
#define SCR_HEIGHT (272)
#define PIXEL_SIZE (4) /* change this if you change to another screenmode */
#define FRAME_SIZE (BUF_WIDTH * SCR_HEIGHT * PIXEL_SIZE)
#define ZBUF_SIZE (BUF_WIDTH SCR_HEIGHT * 2) /* zbuffer seems to be 16-bit? */

void gaasGFXInit() {
	fbp0 = gaasVRAMGetStaticVramBuffer(BUF_WIDTH,SCR_HEIGHT,GU_PSM_8888);
	fbp1 = gaasVRAMGetStaticVramBuffer(BUF_WIDTH,SCR_HEIGHT,GU_PSM_8888);
	zbp = gaasVRAMGetStaticVramBuffer(BUF_WIDTH,SCR_HEIGHT,GU_PSM_4444);

	sceGuInit();
    sceGuStart(GU_DIRECT,DisplayList);

    sceGuDrawBuffer(GU_PSM_8888,fbp0,BUF_WIDTH);
    sceGuDispBuffer(SCR_WIDTH,SCR_HEIGHT,fbp1,BUF_WIDTH);
    sceGuDepthBuffer(zbp,BUF_WIDTH);

    sceGuDepthRange(65535,0);
    sceGuDepthMask(GU_FALSE);

    sceGuOffset(2048 - (SCR_WIDTH / 2),2048 - (SCR_HEIGHT / 2));
    sceGuViewport(2048,2048,SCR_WIDTH,SCR_HEIGHT);

    sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);

	sceGuScissor(0,0,SCR_WIDTH,SCR_HEIGHT);
	sceGuEnable(GU_SCISSOR_TEST);

	sceGuAlphaFunc(GU_GREATER,0,0xff);
	sceGuEnable(GU_ALPHA_TEST);

	sceGuDepthFunc(GU_GEQUAL);
	sceGuEnable(GU_DEPTH_TEST);

	sceGuFrontFace(GU_CW);

	sceGuTexMode(GU_PSM_5650,0,0,0);
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
	sceGuTexFilter(GU_NEAREST,GU_NEAREST);

	sceGuEnable(GU_CULL_FACE);

	sceGuEnable(GU_TEXTURE_2D);

	sceGuEnable(GU_CLIP_PLANES);

	sceGuEnable(GU_LIGHTING);
	sceGuEnable(GU_LIGHT0);
	sceGuEnable(GU_LIGHT1);
	sceGuEnable(GU_LIGHT2);
	sceGuEnable(GU_LIGHT3);

	sceGuEnable(GU_FOG);

	sceGuEnable(GU_BLEND);
	sceGuBlendFunc(GU_ADD,GU_SRC_ALPHA,GU_ONE_MINUS_SRC_ALPHA,0,0);

	sceGuShadeModel(GU_SMOOTH);

	sceGuFinish();
	sceGuSync(0,0);

	sceDisplayWaitVblankStart();
	sceGuDisplay(GU_TRUE);
}

void gaasGFXAnimatedSpriteAlpha(int collumns, int rows, int width, int height, gaasImage* source, int x, int y, int update) {
	if(source==NULL) return;
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
	sceGuTexMode(source->format, 0, 0, source->swizzled);
	sceGuTexImage(0, source->tw, source->th, source->tw, source->data);
	sceGuTexScale((float)1.0, (float)1.0);
	sceGuTexWrap(GU_CLAMP, GU_CLAMP);
	sceGuTexFilter(GU_NEAREST, GU_NEAREST);
	sceGuDisable(GU_DEPTH_TEST);
	int j = 0;

	static int up;
	static int row;
	static int col;

	while (j < width) {
		struct Vertex {
        	unsigned short u,v;
        	short x,y,z;
		} *vertices = (struct Vertex*) sceGuGetMemory(2 * sizeof(struct Vertex));
		int sliceWidth = 64;
		
		if (j + sliceWidth > width) sliceWidth = width - j;
		vertices[0].u = width*col + j;
		vertices[0].v = height*row;
		vertices[0].x = x + j;
		vertices[0].y = y;
		vertices[0].z = 1;
		vertices[1].u = width*col + j + sliceWidth;
		vertices[1].v = height*row + height;
		vertices[1].x = x + j + sliceWidth;
		vertices[1].y = y + height;
		vertices[1].z = 1;
		sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, 0, vertices);
		j += sliceWidth;
	}

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

	sceGuEnable(GU_DEPTH_TEST);
}

void gaasGFXTexture(gaasImage* source, int filter, int repeat, int tfx) {
	if(source==NULL) return;
	sceGuTexMode(source->format,0,0,source->swizzled);
	sceGuTexImage(0, source->tw, source->th, source->tw, source->data);
	sceGuTexWrap(repeat, repeat); 
	sceGuTexFunc(tfx,GU_TCC_RGBA);
	sceGuTexFilter(filter, filter);
	sceGuTexScale(1.0f, 1.0f);
}

void gaasGFXTextureMip(gaasImage* source, gaasImage* source1, gaasImage* source2, gaasImage* source3, gaasImage* source4, int max, int filter, int repeat, int tfx, int mode, float bias) {
	if(source==NULL) return;
	sceGuTexMode(source->format,max,0,source->swizzled);
	sceGuTexLevelMode(mode, bias);
	sceGuTexWrap(repeat, repeat); 
	sceGuTexFunc(tfx,GU_TCC_RGBA);
	sceGuTexFilter(filter, filter);

	switch (max)
	{
	case 0:
		sceGuTexImage(0, source->tw, source->th, source->tw, source->data);
		break;
	
	case 1:
		sceGuTexImage(0, source->tw, source->th, source->tw, source->data);
		sceGuTexImage(1, source1->tw, source1->th, source1->tw, source1->data);
		break;
	
	case 2:
		sceGuTexImage(0, source->tw, source->th, source->tw, source->data);
		sceGuTexImage(1, source1->tw, source1->th, source1->tw, source1->data);
		sceGuTexImage(2, source2->tw, source2->th, source2->tw, source2->data);
		break;
	
	case 3:
		sceGuTexImage(0, source->tw, source->th, source->tw, source->data);
		sceGuTexImage(1, source1->tw, source1->th, source1->tw, source1->data);
		sceGuTexImage(2, source2->tw, source2->th, source2->tw, source2->data);
		sceGuTexImage(3, source3->tw, source3->th, source3->tw, source3->data);
		break;
	
	case 4:
		sceGuTexImage(0, source->tw, source->th, source->tw, source->data);
		sceGuTexImage(1, source1->tw, source1->th, source1->tw, source1->data);
		sceGuTexImage(2, source2->tw, source2->th, source2->tw, source2->data);
		sceGuTexImage(3, source3->tw, source3->th, source3->tw, source3->data);
		sceGuTexImage(4, source4->tw, source4->th, source4->tw, source4->data);
		break;
	
	default:
		break;
	}
	sceGuTexScale(1.0f, 1.0f);
}

void gaasGFXFilledRect(int x, int y, int width, int height, unsigned int color) {
    struct Vertex {
		unsigned int color;
		short x,y,z;
	} *vert =(struct Vertex *) sceGuGetMemory(2 * sizeof(struct Vertex));
    vert[0].x = x;
    vert[0].y = y;
    vert[0].z = 0;
    vert[0].color = color;
    vert[1].x = x+width;
    vert[1].y = y+height;
    vert[1].z = 0;
    vert[1].color = color;
    sceGuDisable(GU_TEXTURE_2D);
    sceGuEnable(GU_BLEND);
    sceGuDisable(GU_DEPTH_TEST);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    sceGuDrawArray(GU_SPRITES, GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2,0,vert);
    sceGuEnable(GU_TEXTURE_2D);
	sceGuEnable(GU_DEPTH_TEST);
}

void gaasGFXSprite(int startx, int starty, int width, int height, gaasImage* source, int x, int y) {
	if(source==NULL) return;
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
	sceGuTexMode(source->format, 0, 0, source->swizzled);
	sceGuTexImage(0, source->tw, source->th, source->tw, source->data);
	sceGuTexFilter(GU_NEAREST, GU_NEAREST);
	float u = 1.0f / ((float)source->tw);
	float v = 1.0f / ((float)source->th);
	sceGuTexScale(u, v);
	
	sceGuDisable(GU_DEPTH_TEST);
	int j = 0;
	while (j < width) {
		struct Vertex {
        	unsigned short u,v;
        	short x,y,z;
		} *vertices = (struct Vertex*) sceGuGetMemory(2 * sizeof(struct Vertex));
		int sliceWidth = 64;
		if (j + sliceWidth > width) sliceWidth = width - j;
		vertices[0].u = startx + j;
		vertices[0].v = starty;
		vertices[0].x = x + j;
		vertices[0].y = y;
		vertices[0].z = 1;
		vertices[1].u = startx + j + sliceWidth;
		vertices[1].v = starty + height;
		vertices[1].x = x + j + sliceWidth;
		vertices[1].y = y + height;
		vertices[1].z = 1;
		sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, 0, vertices);
		j += sliceWidth;
	}
	sceGuEnable(GU_DEPTH_TEST);
	sceGuTexScale(1.0f, 1.0f);
}

void gaasGFXRenderTargetSprite(Texture* texture, int x, int y, int width, int height) {
	sceGuTexMode(texture->format,0,0,0);
	sceGuTexImage(texture->mipmap,texture->width,texture->height,texture->stride,texture->data);
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGB);
	sceGuTexFilter(GU_NEAREST,GU_NEAREST);
	sceGuTexScale(1.0f,1.0f);
	sceGuTexOffset(0.0f,0.0f); 
	sceGuAmbientColor(0xffffffff); 
	
	int startx = 0;
	int starty = 0;
	int alpha = 255;
	
	sceGuDisable(GU_DEPTH_TEST);
	int j = 0;
	while (j < width) {
		struct Vertex {
        	unsigned short u,v;
        	u32 color;
        	short x,y,z;
		} *vertices = (struct Vertex*) sceGuGetMemory(2 * sizeof(struct Vertex));
		int sliceWidth = 64;
		if (j + sliceWidth > width) sliceWidth = width - j;
		vertices[0].u = startx + j;
		vertices[0].v = starty;
		vertices[0].x = x + j;
		vertices[0].y = y;
		vertices[0].z = 1;
		vertices[0].color = GU_RGBA(255,255,255,alpha);
		vertices[1].u = startx + j + sliceWidth;
		vertices[1].v = starty + height;
		vertices[1].x = x + j + sliceWidth;
		vertices[1].y = y + height;
		vertices[1].z = 1;
		vertices[1].color = GU_RGBA(255,255,255,alpha);
		sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, 0, vertices);
		j += sliceWidth;
	}
	sceGuEnable(GU_DEPTH_TEST); 
	sceGuTexScale(1.0f, 1.0f);
}