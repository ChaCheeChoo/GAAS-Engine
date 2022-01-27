#include <stdio.h>
#include <stdlib.h>
#include <pspgu.h>
#include <pspgum.h>
#include <string.h>

#include "objloader.h"

static struct BBox {
	float x,y,z;
};

static struct VertexUVNormal {
	float u,v;
	float nx,ny,nz;
	float x,y,z;
};

static struct Model {
    int vertCount;
    struct VertexUVNormal *vert;
	struct BBox bbox[8];
	float min[3];
	float max[3];
    int indexCount;
    unsigned short *index;
	int check;
};

static struct Model objModel[512]; //maximum number of meshes that can be loaded at once
static int nextObj=0;

/**
 * Loads OBJ models (no mtl support)
**/
int gaasOBJLoad(const char *filename, int usesoffset, int fileoffset, int filesize) {
	char line[256];

    FILE* fp;
    struct VertexUVNormal *vert=0;
    int readsize;
    
    fp = fopen(filename, "r");

    if(!fp) {
        printf("Failed to load obj\n");
        return -1;
    }

    if(usesoffset==1) {
        readsize=filesize+fileoffset+1;
    } else {
        fseek(fp, 0, SEEK_END);
        readsize=ftell(fp);
        fseek(fp, 0, SEEK_SET);
    }

    if(usesoffset==1) {
        fseek(fp, fileoffset, SEEK_SET);
    }

    if(fp) {
		char line[256];
		line[255]=0;
		int v=0;
		int vt=0;
		int vn=0;
		int f=0;

		while( fgets(line,255,fp) && ftell(fp)<=readsize) {
			switch(line[0]) {
			    case 'v':
				    if(line[1]==' ') v++;
				    if(line[1]=='t') vt++;
			        if(line[1]=='n') vn++;
				    break;
			    case 'f':
				    f+=3;
				    break;
			}
		}
		objModel[nextObj].indexCount=f;
		objModel[nextObj].index=(unsigned short *)calloc(sizeof(short),f);
		objModel[nextObj].vertCount=f;
		vert=(struct VertexUVNormal *)calloc(sizeof(struct VertexUVNormal),v+vt+vn);
		objModel[nextObj].vert=(struct VertexUVNormal *)calloc(sizeof(struct VertexUVNormal),f);
        fclose(fp);
	}

    fp = fopen(filename, "r");

	if(usesoffset==1) {
        fseek(fp, fileoffset, SEEK_SET);
    }

    int v=0;
	int vt=0;
	int vn=0;
	int f=0;

    while(fgets(line,255,fp) && ftell(fp)<=readsize) {
        float tu,tv;
		float x,y,z;
		float nx,ny,nz;

        int f1,f2,f3;	// face vertex
		int n1,n2,n3;	// normal
		int t1,t2,t3;	// texture

        line[255]=0;
		if(strchr(line,'\n')) strchr(line,'\n')[0]=0;
		if(strchr(line,'\r')) strchr(line,'\r')[0]=0;

        if(sscanf(line, "v %f %f %f", &x, &y, &z)==3) {
			if(x<objModel[nextObj].min[0]) objModel[nextObj].min[0]=x;
			if(y<objModel[nextObj].min[1]) objModel[nextObj].min[1]=y;
			if(z<objModel[nextObj].min[2]) objModel[nextObj].min[2]=z;
			if(x>objModel[nextObj].max[0]) objModel[nextObj].max[0]=x;
			if(y>objModel[nextObj].max[1]) objModel[nextObj].max[1]=y;
			if(z>objModel[nextObj].max[2]) objModel[nextObj].max[2]=z;
            vert[v].x = x;
            vert[v].y = y;
            vert[v].z = z;
            v++;
        } else if(sscanf(line, "vt %f %f", &tu, &tv)==2) {
            vert[vt].u = tu;
            vert[vt].v = 1.0f-tv;
            vt++;
        } else if(sscanf(line, "vn %f %f %f", &nx, &ny, &nz)==3) {
            vert[vn].nx = nx;
            vert[vn].ny = ny;
            vert[vn].nz = nz;
            vn++;
        } else if(sscanf(line,"f %d/%d/%d %d/%d/%d %d/%d/%d",&f1,&t1,&n1,&f2,&t2,&n2,&f3,&t3,&n3)==9) {
            objModel[nextObj].vert[f].x  = vert[f1-1].x;
			objModel[nextObj].vert[f].y  = vert[f1-1].y;
			objModel[nextObj].vert[f].z  = vert[f1-1].z;
			objModel[nextObj].vert[f].u  = vert[t1-1].u;
			objModel[nextObj].vert[f].v  = vert[t1-1].v;
			objModel[nextObj].vert[f].nx = vert[n1-1].nx;
			objModel[nextObj].vert[f].ny = vert[n1-1].ny;
			objModel[nextObj].vert[f].nz = vert[n1-1].nz;
			objModel[nextObj].index[f++] = f1-1;

			objModel[nextObj].vert[f].x  = vert[f2-1].x;
			objModel[nextObj].vert[f].y  = vert[f2-1].y;
			objModel[nextObj].vert[f].z  = vert[f2-1].z;
			objModel[nextObj].vert[f].u  = vert[t2-1].u;
			objModel[nextObj].vert[f].v  = vert[t2-1].v;
			objModel[nextObj].vert[f].nx = vert[n2-1].nx;
			objModel[nextObj].vert[f].ny = vert[n2-1].ny;
			objModel[nextObj].vert[f].nz = vert[n2-1].nz;
			objModel[nextObj].index[f++] = f2-1;

			objModel[nextObj].vert[f].x  = vert[f3-1].x;
			objModel[nextObj].vert[f].y  = vert[f3-1].y;
			objModel[nextObj].vert[f].z  = vert[f3-1].z;
			objModel[nextObj].vert[f].u  = vert[t3-1].u;
			objModel[nextObj].vert[f].v  = vert[t3-1].v;
			objModel[nextObj].vert[f].nx = vert[n3-1].nx;
			objModel[nextObj].vert[f].ny = vert[n3-1].ny;
			objModel[nextObj].vert[f].nz = vert[n3-1].nz;
			objModel[nextObj].index[f++] = f3-1;
		}
    }

	objModel[nextObj].bbox[0].x = objModel[nextObj].min[0];
	objModel[nextObj].bbox[0].y = objModel[nextObj].min[1];
	objModel[nextObj].bbox[0].z = objModel[nextObj].min[2];

	objModel[nextObj].bbox[1].x = objModel[nextObj].max[0];
	objModel[nextObj].bbox[1].y = objModel[nextObj].min[1];
	objModel[nextObj].bbox[1].z = objModel[nextObj].min[2];

	objModel[nextObj].bbox[2].x = objModel[nextObj].min[0];
	objModel[nextObj].bbox[2].y = objModel[nextObj].min[1];
	objModel[nextObj].bbox[2].z = objModel[nextObj].max[2];

	objModel[nextObj].bbox[3].x = objModel[nextObj].max[0];
	objModel[nextObj].bbox[3].y = objModel[nextObj].min[1];
	objModel[nextObj].bbox[3].z = objModel[nextObj].max[2];

	
	objModel[nextObj].bbox[4].x = objModel[nextObj].min[0];
	objModel[nextObj].bbox[4].y = objModel[nextObj].max[1];
	objModel[nextObj].bbox[4].z = objModel[nextObj].min[2];

	objModel[nextObj].bbox[5].x = objModel[nextObj].max[0];
	objModel[nextObj].bbox[5].y = objModel[nextObj].max[1];
	objModel[nextObj].bbox[5].z = objModel[nextObj].min[2];

	objModel[nextObj].bbox[6].x = objModel[nextObj].min[0];
	objModel[nextObj].bbox[6].y = objModel[nextObj].max[1];
	objModel[nextObj].bbox[6].z = objModel[nextObj].max[2];

	objModel[nextObj].bbox[7].x = objModel[nextObj].max[0];
	objModel[nextObj].bbox[7].y = objModel[nextObj].max[1];
	objModel[nextObj].bbox[7].z = objModel[nextObj].max[2];

    fclose(fp);

    free(vert);
	vert=0;
    free(objModel[nextObj].index);
	objModel[nextObj].index=0;
	objModel[nextObj].check = 420;

	//printf("%f %f %f\n", objModel[nextObj].bbox[0].x, objModel[nextObj].bbox[0].y, objModel[nextObj].bbox[0].z);

    int objId=nextObj;
    nextObj++;

	if(objId>512) {
		printf("ran out of obj ID's\n");
		return -1;
	}

    return objId;
}

void gaasOBJFreeAll() {
	for(int i=0;i<nextObj;i++) {
		if(objModel[i].vert) free(objModel[i].vert);
		objModel[i].vert=0;
		if(objModel[i].index) free(objModel[i].index);
		objModel[i].index=0;
	}
	nextObj=0;
}

void gaasOBJFreeSingle(int i) {
	if(objModel[i].vert) free(objModel[i].vert);
	objModel[i].vert=0;
	if(objModel[i].index) free(objModel[i].index);
	objModel[i].index=0;
	nextObj--;
}

void gaasOBJRender(int obj, int color, int usebb) {
	//if(objModel[obj].check==420) {
    	sceGuColor(color);
		if(usebb==1) {
			//sceGumDrawArray(GU_POINTS, GU_VERTEX_32BITF | GU_TRANSFORM_3D, 8, 0, objModel[obj].bbox); //debug stuff
			sceGuBeginObject(GU_VERTEX_32BITF, 8, 0, objModel[obj].bbox);
		}
    	sceGuFrontFace(GU_CCW);
    	sceGumDrawArray(GU_TRIANGLES,GU_VERTEX_32BITF|GU_NORMAL_32BITF|GU_TEXTURE_32BITF|GU_TRANSFORM_3D,objModel[obj].vertCount,0,objModel[obj].vert);
    	sceGuFrontFace(GU_CW);
		if(usebb==1) {
			sceGuEndObject();
		}
	//}
}