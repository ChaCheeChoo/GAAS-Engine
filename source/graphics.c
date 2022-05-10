#include <pspgu.h>
#include <pspgum.h>
#include <stdio.h>
#include <vram.h>

#include "imageloader.h"
#include "graphics.h"

#define BUF_WIDTH (512)
#define SCR_WIDTH (480)
#define SCR_HEIGHT (272)
#define PIXEL_SIZE (4) /* change this if you change to another screenmode */
#define FRAME_SIZE (BUF_WIDTH * SCR_HEIGHT * PIXEL_SIZE)
#define ZBUF_SIZE (BUF_WIDTH * SCR_HEIGHT * 2) /* zbuffer seems to be 16-bit? */

void gaasGFXInit(int PixelSize, int Psm) {
	/* fbp0 = gaasVRAMGetStaticVramBuffer(BUF_WIDTH,SCR_HEIGHT,GU_PSM_8888);
	fbp1 = gaasVRAMGetStaticVramBuffer(BUF_WIDTH,SCR_HEIGHT,GU_PSM_8888);
	zbp = gaasVRAMGetStaticVramBuffer(BUF_WIDTH,SCR_HEIGHT,GU_PSM_4444); */
	int frameBufferSize = PixelSize*BUF_WIDTH*SCR_HEIGHT;

	fbp0 = valloc(frameBufferSize)-0x4000000;
	fbp1 = valloc(frameBufferSize)-0x4000000;
	zbp = valloc(ZBUF_SIZE)-0x4000000;

	printf("fbp0: 0x%x  fbp1: 0x%x  zbp: 0x%x\n", fbp0, fbp1, zbp);

	sceGuInit();
    sceGuStart(GU_DIRECT,DisplayList);

    sceGuDrawBuffer(Psm,fbp0,BUF_WIDTH);
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
	sceGuTexOffset(0.0f,0.0f); 
}

void gaasGFXTextureMip(gaasImageMipmap* source, int filter_min, int filter_mag, int repeat_u, int repeat_v, int tfx, int mode, float bias) {
	if(source==NULL) return;
	int levels=source->levels;

	sceGuTexMode(source->image[0]->format,levels-1,0,source->image[0]->swizzled);
	sceGuTexLevelMode(mode, bias);
	sceGuTexWrap(repeat_u, repeat_v); 
	sceGuTexFunc(tfx,GU_TCC_RGBA);
	sceGuTexFilter(filter_min, filter_mag);
	sceGuTexOffset(0.0f,0.0f); 
	sceGuTexScale(1.0f, 1.0f);
	for(int i=0; i<levels; i++) {
		sceGuTexImage(i, source->image[i]->tw, source->image[i]->th, source->image[i]->tw, source->image[i]->data);
	}
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

void gaasGFXSpriteTinted(int startx, int starty, int width, int height, gaasImage* source, int x, int y, unsigned int tint) {
	if(source==NULL) return;
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
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
			unsigned int color;
        	short x,y,z;
		} *vertices = (struct Vertex*) sceGuGetMemory(2 * sizeof(struct Vertex));
		int sliceWidth = 64;
		if (j + sliceWidth > width) sliceWidth = width - j;
		vertices[0].u = startx + j;
		vertices[0].v = starty;
		vertices[0].x = x + j;
		vertices[0].y = y;
		vertices[0].z = 1;
		vertices[0].color = tint;
		vertices[1].u = startx + j + sliceWidth;
		vertices[1].v = starty + height;
		vertices[1].x = x + j + sliceWidth;
		vertices[1].y = y + height;
		vertices[1].z = 1;
		vertices[1].color = tint;
		sceGuDrawArray(GU_SPRITES, GU_VERTEX_16BIT | GU_COLOR_8888 | GU_TEXTURE_16BIT | GU_TRANSFORM_2D, 2, 0, vertices);
		j += sliceWidth;
	}
	sceGuEnable(GU_DEPTH_TEST);
	sceGuTexScale(1.0f, 1.0f);
}

void gaasGFXTextureScroller(gaasImage* source, float scrollx, float scrolly, float maxscroll, int filter, int repeat, int tfx) {
	if(source==NULL) return;

	static float scroll_prog_x;
	static float scroll_prog_y;

	sceGuTexMode(source->format,0,0,source->swizzled);
	sceGuTexImage(0, source->tw, source->th, source->tw, source->data);
	sceGuTexWrap(repeat, repeat); 
	sceGuTexFunc(tfx,GU_TCC_RGBA);
	sceGuTexFilter(filter, filter);
	sceGuTexScale(1.0f, 1.0f);
	sceGuTexOffset(scroll_prog_x, scroll_prog_y);

	//loop back stuffz
	if(scrollx>0 || scrolly>0) {
		if(scroll_prog_x>=maxscroll || scroll_prog_y>=maxscroll) {
			scroll_prog_x = 0.0f;
			scroll_prog_y = 0.0f;
		} else {
			scroll_prog_x+=scrollx;
			scroll_prog_y+=scrolly;
		}
	} else {
		if(scroll_prog_x<=maxscroll || scroll_prog_y<=maxscroll) {
			scroll_prog_x = 0.0f;
			scroll_prog_y = 0.0f;
		} else {
			scroll_prog_x+=scrollx;
			scroll_prog_y+=scrolly;
		}
	}
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

void gaasGFXRenderTargetTexture(Texture* texture, int filter, int repeat, int tfx) {
	sceGuTexMode(texture->format,0,0,0);
	sceGuTexImage(texture->mipmap,texture->width,texture->height,texture->stride,texture->data);
	sceGuTexFunc(tfx,GU_TCC_RGB);
	sceGuTexFilter(filter,filter);
	sceGuTexWrap(repeat,repeat);
	sceGuTexScale(1.0f,1.0f);
	sceGuTexOffset(0.0f,0.0f); 
}

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