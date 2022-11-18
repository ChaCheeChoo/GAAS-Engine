#ifndef GLTFLOADER
#define GLTFLOADER

#include <psptypes.h>
#include "imageloader.h"

typedef struct Vector4UnsignedShort {
	unsigned short x;
	unsigned short y;
	unsigned short z;
	unsigned short w;
} Vector4UnsignedShort;

static struct BBox {
	float x,y,z;
};

enum INTERPOLATION_TYPE {
	interpolation_type_linear,
	interpolation_type_step,
	interpolation_type_cubic_spline,
};

enum ANIMATION_TYPE {
	animation_path_type_invalid,
	animation_path_type_translation,
	animation_path_type_rotation,
	animation_path_type_scale,
	animation_path_type_weights,
};

//different types of vertex structures
struct UVColorNormalVertex {
    float u, v;
	unsigned int color;
	float nx, ny, nz;
	float x, y, z;
};

struct UVNormalVertex {
    float u, v;
	float nx, ny, nz;
	float x, y, z;
};

struct UVColorVertex {
    float u, v;
	unsigned int color;
	float x, y, z;
};

struct UVVertex {
    float u, v;
	float x, y, z;
};

struct AnimChannel {
	int nodeID;
	int animType;
	int inputCount;
	float* input;
	int outputCount;
	ScePspFVector3* output;
	int interpolationType;
};

//node struct; an array of these will be generated
struct Node {
	char name[256];
	int hasAnyTransforms;
	ScePspFVector3 pos;
	ScePspFVector3 rot;
	ScePspFVector3 scl;
	ScePspFMatrix4 matrix;
	ScePspFMatrix4 parentMatrix;
	int animId;
	int totChannels;
	int meshId;
};

//material struct; an array of these will be generated; referenced by mesh via ID
struct Material {
	char name[256]; //limited because making it a pointer and allocating causes crash
	int repeat_u, repeat_v;
	int filter_min, filter_mag;
	int unlit;
	int doubleSided;
	int alphaMode;
	gaasImageMipmap* tex;
};

struct Animation {
	int totalChan;
	struct AnimChannel* channels;
};

//mesh struct; an array of these will be generated using the mesh_count of scene; referenced by node via ID
struct Mesh {
	char name[256]; //limited because making it a pointer and allocating causes crash
	int prim;
	int vtype;
	int vertCount;
	int meshType;
	int matId;
	struct UVColorNormalVertex* UVColorNormalMesh;
	struct UVNormalVertex* UVNormalMesh;
	struct UVColorVertex* UVColorMesh;
	struct UVVertex* UVMesh;
	unsigned short* indices;
	struct BBox bbox[8];
	float min[3];
	float max[3];
};

typedef struct gaasGLTF {
    int TotRenderLists;
    int RenderListValid;
    int* RenderList[32];
    float currentTime;
    int currentFrame;
    unsigned int TotalMeshes;
    unsigned int TotalNodes;
    unsigned int TotalMaterials;
    unsigned int TotalAnims;
    struct Node* nodes;
    struct Mesh* meshes;
    struct Material* materials;
    struct Animation* animations;
} gaasGLTF;

/**
 * Loads glTF scene
 * file - file to load, either .gltf/.glb or your game eboot if using gwd
 * miplevels - how many mipmap levels to use, 0 for none
 * fileoffset - if using gwd set using gaasGWDGetOffsetFromName, else set to 0
 * filesize - if using gwd set using gaasGWDGetSizeFromName, else set to 0
 * overwriteColor - flag to overwrite vertex color in scene, if 1 then newColor will be used
**/
gaasGLTF* gaasGLTFLoad(const char* file, int miplevels, int fileoffset, int filesize, int overwriteColor, unsigned int newColor);

/**
 * render lists are used for selective rendering
 * when you pass a value into selectRender the renderer will only draw Nodes that are in that specific list
 * file - render list file
 * offset - set if using gwd
 * size - set if using gwd
 * 
 * Example of rendering list file:
 *      TotChunk = 2 //total number of chunks
 *      chunk 0 4 //first chunk | 0 means it's the first chunk in the array; 4 means the amount of objects in the chunk
 *      chunk 1 2 //second chunk | 1 means it's the second chunk in the array; 2 means the amount of objects in the chunk
 * 
 *      node 0 0 ExampleObject_00 
 *      node 0 1 ExampleObject_01 //object definition | 0 means this object is in the first chunk array; 1 means it's the second object in the chunk array
 *      node 0 2 ExampleObject_02
 *      node 0 3 ExampleObject_03
 * 
 *      node 1 0 ExampleObject_04
 *      node 1 1 ExampleObject_05
 *      
 * 
 * INFO: the last node in the first chunk will fail to render if only one chunk is present
**/
void gaasGLTFLoadRenderList(gaasGLTF* scene, const char* file, int offset, int size);

/**
 * Render glTF scene
 * selectRender - pass in a number from 0 to TotalChunks and it'll render only nodes in that chunk array; 
 *      if you want to render everything pass in -1
 * selectCamera - unused (for now)
 * usebb - generates bounding boxes around all objects for conditional rendering;
 *      an object will only be rendered if its bounding box is within camera view
 * debugNode - will highlight and render the name of the object matching the supplied value
 *      pass in -1 for normal rendering
**/
void gaasGLTFRender(gaasGLTF* scene, int selectRender, int selectCamera, int usebb, int debugNode);

/**
 * Deletes scene from memory
**/
void gaasGLTFFree(gaasGLTF* scene);

#endif