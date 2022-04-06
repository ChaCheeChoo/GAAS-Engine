#include <pspkernel.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <psptypes.h>
#include <pspdisplay.h>
#include <psppower.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <pspmoduleinfo.h>
#include <pspgu.h>
#include <pspgum.h>
#include <psptypes.h>

#include "vram.h"
#include "drawtext.h"
#include "graphics.h"
#include "imageloader.h"
#include "glTFLoader.h"

#define CGLTF_IMPLEMENTATION
#include "./cgltf/cgltf.h" //uses modified version of cgltf for .gwd support

cgltf_data* data = NULL;

static int count = -420;
static int count_normal = -69;
static int count_tex = -13;
static int count_color = -37;
static unsigned int TotalMeshes;
static unsigned int TotalNodes;
static unsigned int TotalMaterials;

typedef struct Vector4UnsignedShort {
	unsigned short x;
	unsigned short y;
	unsigned short z;
	unsigned short w;
} Vector4UnsignedShort;

static struct BBox {
	float x,y,z;
};

//temporary buffers used before data is written to main vertex list
struct ScePspFVector2* temp_tex;
struct ScePspFVector3* temp_normal;
struct ScePspFVector3* temp_pos;
struct Vector4UnsignedShort* temp_color;

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

//node struct; an array of these will be generated
struct Node {
	char name[256];
	int hasAnyTransforms;
	ScePspFVector3 pos;
	ScePspFVector3 rot;
	ScePspFVector3 scl;
	ScePspFMatrix4 matrix;
	ScePspFMatrix4 parentMatrix;
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

struct Node* nodes;
struct Mesh* meshes;
struct Material* materials;

void* tempImageBuf;

void gaas_GLTF_internal_LoadMeshes(int mesh_id, int overwriteColor, unsigned int ColorToOverwriteWith) {
	//decide what primitive type a mesh will use
	switch (data->meshes->primitives->type) {
	    case cgltf_primitive_type_points:
	    	meshes[mesh_id].prim=GU_POINTS;
	    	break;
    
	    case cgltf_primitive_type_lines:
	    	meshes[mesh_id].prim=GU_LINES;
	    	break;

	    case cgltf_primitive_type_line_strip:
	    	meshes[mesh_id].prim=GU_LINE_STRIP;
	    	break;
    
	    case cgltf_primitive_type_triangles:
	    	meshes[mesh_id].prim=GU_TRIANGLES;
	    	break;

	    case cgltf_primitive_type_triangle_strip:
	    	meshes[mesh_id].prim=GU_TRIANGLE_STRIP;
	    	break;
    
	    case cgltf_primitive_type_triangle_fan:
	    	meshes[mesh_id].prim=GU_TRIANGLE_FAN;
	    	break;
    
	    default:
	    	break;
	}

	int index_tex = -1;
	int index_pos = -1;
	int index_norm = -1;
	int index_color = -1;

	//determine vtype and attributes, this will determine what data will be loaded
	int att_total = data->meshes[mesh_id].primitives->attributes_count;
	for(int i=0; i<att_total; i++) {
		switch (data->meshes[mesh_id].primitives->attributes[i].type) {
			case cgltf_attribute_type_position:
				index_pos=i;
				break;

			case cgltf_attribute_type_texcoord:
				index_tex=i;
				break;

			case cgltf_attribute_type_color:
				index_color=i;
				break;

			case cgltf_attribute_type_normal:
				index_norm=i;
				break;
			
			default:
				break;
		}
	}

	//printf("pos: %d | tex: %d | normal: %d | color: %d\n", index_pos, index_tex, index_norm, index_color);
	meshes[mesh_id].meshType = -1;//mesh types: 0 - UVColorNormalVertex; 1 - UVNormalVertex; 2 - UVColorVertex; 3 - UVVertex; -1 - invalid

	//determine mesh type
	if(index_pos!=-1 && index_tex!=-1 && index_norm!=-1 && index_color!=-1) {
		meshes[mesh_id].meshType=0;//all data is present
	} else if(index_pos!=-1 && index_tex!=-1 && index_norm!=-1 && index_color==-1) {
		meshes[mesh_id].meshType=1;//color data is missing
	} else if(index_pos!=-1 && index_tex!=-1 && index_norm==-1 && index_color!=-1) {
		meshes[mesh_id].meshType=2;//normal data is missing
	} else if(index_pos!=-1 && index_tex!=-1 && index_norm==-1 && index_color==-1) {
		meshes[mesh_id].meshType=3;//both color and normal data is missing
	} else {
		meshes[mesh_id].meshType=-1;//invalid data
		printf("Invalid data, unsupported vertex type|mesh: %d\n", mesh_id);
	}

	//printf("meshType: %d\n", meshes[mesh_id].meshType);

	//generate actual vtype data
	switch (meshes[mesh_id].meshType) {
		case 0:
			meshes[mesh_id].vtype = GU_TEXTURE_32BITF|GU_COLOR_8888|GU_NORMAL_32BITF|GU_VERTEX_32BITF|GU_INDEX_16BIT|GU_TRANSFORM_3D;
			break;
		
		case 1:
			meshes[mesh_id].vtype = GU_VERTEX_32BITF|GU_NORMAL_32BITF|GU_TEXTURE_32BITF|GU_INDEX_16BIT|GU_TRANSFORM_3D;
			break;
		
		case 2:
			meshes[mesh_id].vtype = GU_VERTEX_32BITF|GU_TEXTURE_32BITF|GU_COLOR_8888|GU_INDEX_16BIT|GU_TRANSFORM_3D;
			break;
		
		case 3:
			meshes[mesh_id].vtype = GU_TEXTURE_32BITF|GU_VERTEX_32BITF|GU_INDEX_16BIT|GU_TRANSFORM_3D;
			break;

		default:
			break;
	}
	

    //gets number of vertices
	meshes[mesh_id].vertCount = data->meshes[mesh_id].primitives->indices->count;
	//meshes[mesh_id].name=(char*)malloc(sizeof(data->meshes[mesh_id].name));
	strncpy(meshes[mesh_id].name, data->meshes[mesh_id].name, 255);
	
	if(data->meshes[mesh_id].primitives->attributes->data->has_min) {
		meshes[mesh_id].min[0] = data->meshes[mesh_id].primitives->attributes[index_pos].data->min[0];
		meshes[mesh_id].min[1] = data->meshes[mesh_id].primitives->attributes[index_pos].data->min[1];
		meshes[mesh_id].min[2] = data->meshes[mesh_id].primitives->attributes[index_pos].data->min[2];
	}
	if(data->meshes[mesh_id].primitives->attributes->data->has_max) {
		meshes[mesh_id].max[0] = data->meshes[mesh_id].primitives->attributes[index_pos].data->max[0];
		meshes[mesh_id].max[1] = data->meshes[mesh_id].primitives->attributes[index_pos].data->max[1];
		meshes[mesh_id].max[2] = data->meshes[mesh_id].primitives->attributes[index_pos].data->max[2];
	}
	if(data->meshes[mesh_id].primitives->attributes->data->has_min && data->meshes[mesh_id].primitives->attributes->data->has_max) {
		meshes[mesh_id].bbox[0].x = meshes[mesh_id].min[0];
		meshes[mesh_id].bbox[0].y = meshes[mesh_id].min[1];
		meshes[mesh_id].bbox[0].z = meshes[mesh_id].min[2];

		meshes[mesh_id].bbox[1].x = meshes[mesh_id].max[0];
		meshes[mesh_id].bbox[1].y = meshes[mesh_id].min[1];
		meshes[mesh_id].bbox[1].z = meshes[mesh_id].min[2];

		meshes[mesh_id].bbox[2].x = meshes[mesh_id].min[0];
		meshes[mesh_id].bbox[2].y = meshes[mesh_id].min[1];
		meshes[mesh_id].bbox[2].z = meshes[mesh_id].max[2];

		meshes[mesh_id].bbox[3].x = meshes[mesh_id].max[0];
		meshes[mesh_id].bbox[3].y = meshes[mesh_id].min[1];
		meshes[mesh_id].bbox[3].z = meshes[mesh_id].max[2];


		meshes[mesh_id].bbox[4].x = meshes[mesh_id].min[0];
		meshes[mesh_id].bbox[4].y = meshes[mesh_id].max[1];
		meshes[mesh_id].bbox[4].z = meshes[mesh_id].min[2];

		meshes[mesh_id].bbox[5].x = meshes[mesh_id].max[0];
		meshes[mesh_id].bbox[5].y = meshes[mesh_id].max[1];
		meshes[mesh_id].bbox[5].z = meshes[mesh_id].min[2];

		meshes[mesh_id].bbox[6].x = meshes[mesh_id].min[0];
		meshes[mesh_id].bbox[6].y = meshes[mesh_id].max[1];
		meshes[mesh_id].bbox[6].z = meshes[mesh_id].max[2];

		meshes[mesh_id].bbox[7].x = meshes[mesh_id].max[0];
		meshes[mesh_id].bbox[7].y = meshes[mesh_id].max[1];
		meshes[mesh_id].bbox[7].z = meshes[mesh_id].max[2];
	}

	//find matId because cgltf is a cunt that won't tell it to me directly
	for(int i=0; i<TotalMaterials; i++) {
		if(strcmp(materials[i].name, data->meshes[mesh_id].primitives->material->name)==0) {
			meshes[mesh_id].matId=i;
			break;
		} else {
			meshes[mesh_id].matId=-1;
		}
	}
	//printf("Material in Mesh %d: %d\n", mesh_id, meshes[mesh_id].matId);

	//get counts for each accessor
	//printf("Total attributes in mesh %d: %d\n", mesh_id, data->meshes[mesh_id].primitives->attributes_count);
    if(index_pos!=-1) count = data->meshes[mesh_id].primitives->attributes[index_pos].data->count;
    if(index_norm!=-1) count_normal = data->meshes[mesh_id].primitives->attributes[index_norm].data->count;
    if(index_tex!=-1) count_tex   = data->meshes[mesh_id].primitives->attributes[index_tex].data->count;
	if(index_color!=-1) count_color = data->meshes[mesh_id].primitives->attributes[index_color].data->count;
	//printf("Test 1: %d %d %d %d\n", count, count_normal, count_tex, count_color);

    //allocate space for mesh depending on mesh type
	switch (meshes[mesh_id].meshType) {
		case 0:
			//printf("Test 1.1: Allocating memory to mesh %d\n", mesh_id);
			meshes[mesh_id].UVColorNormalMesh=(struct UVColorNormalVertex*)calloc(sizeof(struct UVColorNormalVertex), count);
			meshes[mesh_id].UVNormalMesh=NULL;
			meshes[mesh_id].UVColorMesh=NULL;
			meshes[mesh_id].UVMesh=NULL;
			//printf("Test 1.2\n");
			break;
		
		case 1:
			//printf("Test 1.1: Allocating memory to mesh %d\n", mesh_id);
			meshes[mesh_id].UVColorNormalMesh=NULL;
			meshes[mesh_id].UVNormalMesh=(struct UVNormalVertex*)calloc(sizeof(struct UVNormalVertex), count);
			meshes[mesh_id].UVColorMesh=NULL;
			meshes[mesh_id].UVMesh=NULL;
			//printf("Test 1.2\n");
			break;
		
		case 2:
			//printf("Test 1.1: Allocating memory to mesh %d\n", mesh_id);
			meshes[mesh_id].UVColorNormalMesh=NULL;
			meshes[mesh_id].UVNormalMesh=NULL;
			meshes[mesh_id].UVColorMesh=(struct UVColorVertex*)calloc(sizeof(struct UVColorVertex), count);
			meshes[mesh_id].UVMesh=NULL;
			//printf("Test 1.2\n");
			break;
		
		case 3:
			//printf("Test 1.1: Allocating memory to mesh %d\n", mesh_id);
			meshes[mesh_id].UVColorNormalMesh=NULL;
			meshes[mesh_id].UVNormalMesh=NULL;
			meshes[mesh_id].UVColorMesh=NULL;
			meshes[mesh_id].UVMesh=(struct UVVertex*)calloc(sizeof(struct UVVertex), count);
			//printf("Test 1.2\n");
			break;

		default:
			break;
	}
	//printf("Test 2\n");

    if(index_pos!=-1) temp_pos=(struct ScePspFVector3 *)calloc(sizeof(struct ScePspFVector3), count);//allocate space for mesh vertex
    if(index_tex!=-1) temp_tex=(struct ScePspFVector2 *)calloc(sizeof(struct ScePspFVector2), count_tex);//allocate space for mesh UVs
    if(index_norm!=-1) temp_normal=(struct ScePspFVector3 *)calloc(sizeof(struct ScePspFVector3), count_normal);//allocate space for mesh normals
	if(index_color!=-1) temp_color=(struct Vector4UnsignedShort *)calloc(sizeof(struct Vector4UnsignedShort), count_color);//allocate space for mesh vertex colors
	//printf("Test 3\n");

	if(index_color!=-1) {
    	int buf_size_color = data->meshes[mesh_id].primitives->attributes[index_color].data->buffer_view->size;
    	int buf_offset_color = data->meshes[mesh_id].primitives->attributes[index_color].data->buffer_view->offset;
    	memcpy(temp_color, &data->meshes[mesh_id].primitives->attributes[index_color].data->buffer_view->buffer->data[buf_offset_color], buf_size_color);//copy buffer data directly into vertex buffer
	}
	//printf("Test 4\n");
	if(index_tex!=-1) {
    	int buf_size_tex = data->meshes[mesh_id].primitives->attributes[index_tex].data->buffer_view->size;
    	int buf_offset_tex = data->meshes[mesh_id].primitives->attributes[index_tex].data->buffer_view->offset;
    	memcpy(temp_tex, &data->meshes[mesh_id].primitives->attributes[index_tex].data->buffer_view->buffer->data[buf_offset_tex], buf_size_tex);//copy buffer data directly into vertex buffer
	}
	//printf("Test 5\n");
	if(index_norm!=-1) {
    	int buf_size_normal = data->meshes[mesh_id].primitives->attributes[index_norm].data->buffer_view->size;
    	int buf_offset_normal = data->meshes[mesh_id].primitives->attributes[index_norm].data->buffer_view->offset;
    	memcpy(temp_normal, &data->meshes[mesh_id].primitives->attributes[index_norm].data->buffer_view->buffer->data[buf_offset_normal], buf_size_normal);//copy buffer data directly into vertex buffer
	}
	//printf("Test 6\n");
	if(index_pos!=-1) {
    	int buf_size_mesh = data->meshes[mesh_id].primitives->attributes[index_pos].data->buffer_view->size;
    	int buf_offset_mesh = data->meshes[mesh_id].primitives->attributes[index_pos].data->buffer_view->offset;
    	memcpy(temp_pos, &data->meshes[mesh_id].primitives->attributes[index_pos].data->buffer_view->buffer->data[buf_offset_mesh], buf_size_mesh);//copy buffer data directly into vertex buffer
	}
	//printf("Test 7\n");


	switch (meshes[mesh_id].meshType) {
		case 0:
			for(int i=0; i<count_tex; i++) {
				meshes[mesh_id].UVColorNormalMesh[i].u = temp_tex[i].x;
				meshes[mesh_id].UVColorNormalMesh[i].v = temp_tex[i].y;
			}
			for(int i=0; i<count_color; i++) {
				float tempXfloat = 0.0f;
				float tempYfloat = 0.0f;
				float tempZfloat = 0.0f;
				float tempWfloat = 0.0f;
				if(temp_color[i].x!=0) {
					tempXfloat = temp_color[i].x/65535.0f*255.0f;
				} else {
					tempXfloat = 0.0f;
				}
				if(temp_color[i].y!=0) {
					tempYfloat = temp_color[i].y/65535.0f*255.0f;
				} else {
					tempYfloat = 0.0f;
				}
				if(temp_color[i].z!=0) {
					tempZfloat = temp_color[i].z/65535.0f*255.0f;
				} else {
					tempZfloat = 0.0f;
				}
				if(temp_color[i].w!=0) {
					tempWfloat = temp_color[i].w/65535.0f*255.0f;
				} else {
					tempWfloat = 0.0f;
				}

				if(overwriteColor==0) {
					meshes[mesh_id].UVColorNormalMesh[i].color = GU_RGBA((int)tempXfloat, (int)tempYfloat, (int)tempZfloat, (int)tempWfloat);
				} else {
					meshes[mesh_id].UVColorNormalMesh[i].color = ColorToOverwriteWith;
				}
				//printf("writing color: %d %d %d %d | %f %f %f %f | %x\n", temp_color[i].x, temp_color[i].y, temp_color[i].z, temp_color[i].w,  tempXfloat, tempYfloat, tempZfloat, tempWfloat,  meshes[mesh_id].UVColorNormalMesh[i].color);
			}
			for(int i=0; i<count_normal; i++) {
				meshes[mesh_id].UVColorNormalMesh[i].nx = temp_normal[i].x;
				meshes[mesh_id].UVColorNormalMesh[i].ny = temp_normal[i].y;
				meshes[mesh_id].UVColorNormalMesh[i].nz = temp_normal[i].z;
			}
			for(int i=0; i<count; i++) {
				meshes[mesh_id].UVColorNormalMesh[i].x = temp_pos[i].x;
				meshes[mesh_id].UVColorNormalMesh[i].y = temp_pos[i].y;
				meshes[mesh_id].UVColorNormalMesh[i].z = temp_pos[i].z;
			}
			break;
		
		case 1:
			for(int i=0; i<count_tex; i++) {
				meshes[mesh_id].UVNormalMesh[i].u = temp_tex[i].x;
				meshes[mesh_id].UVNormalMesh[i].v = temp_tex[i].y;
			}
			for(int i=0; i<count_normal; i++) {
				meshes[mesh_id].UVNormalMesh[i].nx = temp_normal[i].x;
				meshes[mesh_id].UVNormalMesh[i].ny = temp_normal[i].y;
				meshes[mesh_id].UVNormalMesh[i].nz = temp_normal[i].z;
			}
			for(int i=0; i<count; i++) {
				meshes[mesh_id].UVNormalMesh[i].x = temp_pos[i].x;
				meshes[mesh_id].UVNormalMesh[i].y = temp_pos[i].y;
				meshes[mesh_id].UVNormalMesh[i].z = temp_pos[i].z;
			}
			break;
		
		case 2:
			for(int i=0; i<count_tex; i++) {
				meshes[mesh_id].UVColorMesh[i].u = temp_tex[i].x;
				meshes[mesh_id].UVColorMesh[i].v = temp_tex[i].y;
			}
			for(int i=0; i<count_color; i++) {
				float tempXfloat = 0.0f;
				float tempYfloat = 0.0f;
				float tempZfloat = 0.0f;
				float tempWfloat = 0.0f;
				if(temp_color[i].x!=0) {
					tempXfloat = temp_color[i].x/65535.0f*255.0f;
				} else {
					tempXfloat = 0.0f;
				}
				if(temp_color[i].y!=0) {
					tempYfloat = temp_color[i].y/65535.0f*255.0f;
				} else {
					tempYfloat = 0.0f;
				}
				if(temp_color[i].z!=0) {
					tempZfloat = temp_color[i].z/65535.0f*255.0f;
				} else {
					tempZfloat = 0.0f;
				}
				if(temp_color[i].w!=0) {
					tempWfloat = temp_color[i].w/65535.0f*255.0f;
				} else {
					tempWfloat = 0.0f;
				}
				if(overwriteColor==0) {
					meshes[mesh_id].UVColorMesh[i].color = GU_RGBA((int)tempXfloat, (int)tempYfloat, (int)tempZfloat, (int)tempWfloat);
				} else {
					meshes[mesh_id].UVColorMesh[i].color = ColorToOverwriteWith;
				}
			}
			for(int i=0; i<count; i++) {
				meshes[mesh_id].UVColorMesh[i].x = temp_pos[i].x;
				meshes[mesh_id].UVColorMesh[i].y = temp_pos[i].y;
				meshes[mesh_id].UVColorMesh[i].z = temp_pos[i].z;
			}
			break;
		
		case 3:
			for(int i=0; i<count_tex; i++) {
				meshes[mesh_id].UVMesh[i].u = temp_tex[i].x;
				meshes[mesh_id].UVMesh[i].v = temp_tex[i].y;
			}
			for(int i=0; i<count; i++) {
				meshes[mesh_id].UVMesh[i].x = temp_pos[i].x;
				meshes[mesh_id].UVMesh[i].y = temp_pos[i].y;
				meshes[mesh_id].UVMesh[i].z = temp_pos[i].z;
			}
			break;

		default:
			break;
	}

    int buf_size_indices = data->meshes[mesh_id].primitives->indices->buffer_view->size;
    int buf_offset_indices = data->meshes[mesh_id].primitives->indices->buffer_view->offset;
    meshes[mesh_id].indices=(unsigned short*)malloc(buf_size_indices);//allocate space for indices
    memcpy(meshes[mesh_id].indices, &data->meshes[mesh_id].primitives->indices->buffer_view->buffer->data[buf_offset_indices], buf_size_indices);//copy buffer data directly into indices buffer

	if(temp_pos!=NULL)   free(temp_pos);
	if(temp_tex!=NULL)   free(temp_tex);
	if(temp_normal!=NULL)free(temp_normal);
	if(temp_color!=NULL) free(temp_color);
}

void gaas_GLTF_internal_LoadMaterials(int mat_id, int miplevels) {
	//basic material info
	strncpy(materials[mat_id].name, data->materials[mat_id].name, 255);
	materials[mat_id].doubleSided = data->materials[mat_id].double_sided;
	materials[mat_id].unlit = data->materials[mat_id].unlit;
	materials[mat_id].alphaMode = data->materials[mat_id].alpha_mode;
	//printf("%d  %s %d %d %d\n", mat_id, materials[mat_id].name, materials[mat_id].doubleSided, materials[mat_id].unlit, materials[mat_id].alphaMode);

	//printf("Mat %d tex %s\n", mat_id, data->materials[mat_id].pbr_metallic_roughness.base_color_texture.texture->image->name);

	//set up filtering and wrapping info
	switch(data->materials[mat_id].pbr_metallic_roughness.base_color_texture.texture->sampler->wrap_s) {
		case 33071:
			materials[mat_id].repeat_u=GU_CLAMP;
			break;
		case 33648:
			materials[mat_id].repeat_u=GU_CLAMP;
			break;
		case 10497:
			materials[mat_id].repeat_u=GU_REPEAT;
			break;
		default:
			break;
	}
	switch(data->materials[mat_id].pbr_metallic_roughness.base_color_texture.texture->sampler->wrap_t) {
		case 33071:
			materials[mat_id].repeat_v=GU_CLAMP;
			break;
		case 33648:
			materials[mat_id].repeat_v=GU_CLAMP;
			break;
		case 10497:
			materials[mat_id].repeat_v=GU_REPEAT;
			break;
		default:
			break;
	}
	switch(data->materials[mat_id].pbr_metallic_roughness.base_color_texture.texture->sampler->mag_filter) {
		case 9728:
			materials[mat_id].filter_mag=GU_NEAREST;
			break;
		case 9729:
			materials[mat_id].filter_mag=GU_LINEAR;
			break;
		default:
			break;
	}
	switch(data->materials[mat_id].pbr_metallic_roughness.base_color_texture.texture->sampler->min_filter) {
		case 9728:
			if(miplevels>0) {
				materials[mat_id].filter_mag=GU_NEAREST_MIPMAP_LINEAR;
			} else {
				materials[mat_id].filter_mag=GU_NEAREST;
			}
			break;
		case 9729:
			if(miplevels>0) {
				materials[mat_id].filter_mag=GU_LINEAR_MIPMAP_LINEAR;
			} else {
				materials[mat_id].filter_mag=GU_LINEAR;
			}
			break;
		case 9984:
			materials[mat_id].filter_mag=GU_NEAREST_MIPMAP_NEAREST;
			break;
		case 9985:
			materials[mat_id].filter_mag=GU_LINEAR_MIPMAP_NEAREST;
			break;
		case 9986:
			materials[mat_id].filter_mag=GU_NEAREST_MIPMAP_LINEAR;
			break;
		case 9987:
			materials[mat_id].filter_mag=GU_LINEAR_MIPMAP_LINEAR;
			break;
		default:
			break;
	}

	//load texture data
	int imageOffset = data->materials[mat_id].pbr_metallic_roughness.base_color_texture.texture->image->buffer_view->offset;
	int imageSize = data->materials[mat_id].pbr_metallic_roughness.base_color_texture.texture->image->buffer_view->size;
	tempImageBuf=(char*)malloc(imageSize);
	memcpy(tempImageBuf, &data->materials[mat_id].pbr_metallic_roughness.base_color_texture.texture->image->buffer_view->buffer->data[imageOffset], imageSize);
	materials[mat_id].tex = gaasIMAGELoadImageMipmapFromBuffer(tempImageBuf, imageSize, 1, miplevels);

	free(tempImageBuf);
}

void gaas_GLTF_internal_LoadNodes(int node_id) {
	//get current node name
	strncpy(nodes[node_id].name, data->nodes[node_id].name, 255);
	nodes[node_id].hasAnyTransforms = 0;

	//find mesh_id because cgltf is a cunt that won't tell it to me directly
	for(int i=0; i<TotalMeshes; i++) {
		if(strcmp(meshes[i].name, data->nodes[node_id].mesh->name)==0) {
			nodes[node_id].meshId=i;
			break;
		} else {
			nodes[node_id].meshId=-1;
		}
	}
	//printf("Mesh in Node %d: %d %s\n", node_id, nodes[node_id].meshId, nodes[node_id].name);

	//load the fucking translation shit
	if(data->nodes[node_id].has_translation) {
		nodes[node_id].hasAnyTransforms = 1;
		nodes[node_id].pos.x=data->nodes[node_id].translation[0];
		nodes[node_id].pos.y=data->nodes[node_id].translation[1];
		nodes[node_id].pos.z=data->nodes[node_id].translation[2];
	} else {
		nodes[node_id].pos.x=0.0f;
		nodes[node_id].pos.y=0.0f;
		nodes[node_id].pos.z=0.0f;
	}

	if(data->nodes[node_id].has_rotation) {
		nodes[node_id].hasAnyTransforms = 1;
		ScePspFVector4 q;
		q.x=data->nodes[node_id].rotation[0];
		q.y=data->nodes[node_id].rotation[1];
		q.z=data->nodes[node_id].rotation[2];
		q.w=data->nodes[node_id].rotation[3];

		double w2 = q.w*q.w;
    	double x2 = q.x*q.x;
    	double y2 = q.y*q.y;
    	double z2 = q.z*q.z;
    	double unitLength = w2 + x2 + y2 + z2;    // Normalised == 1, otherwise correction divisor.
    	double abcd = q.w*q.x + q.y*q.z;
    	double eps = 1e-7;    // TODO: pick from your math lib instead of hardcoding.
    	double pi = GU_PI;   // TODO: pick from your math lib instead of hardcoding.
    	if (abcd > (0.5-eps)*unitLength) {
    	    nodes[node_id].rot.y=2 * atan2f(q.y, q.w);//yaw
    	    nodes[node_id].rot.z = pi;//pitch
    	    nodes[node_id].rot.x=0; //roll
    	} else if (abcd < (-0.5+eps)*unitLength) {
    	    nodes[node_id].rot.y=-2 * atan2f(q.y, q.w);//yaw
    	    nodes[node_id].rot.z = -pi;//pitch
    	    nodes[node_id].rot.x= 0;//roll
    	} else {
    	    double adbc = q.w*q.z - q.x*q.y;
    	    double acbd = q.w*q.y - q.x*q.z;
    	    nodes[node_id].rot.y=atan2f(2*adbc, 1 - 2*(z2+x2));//yaw
    	    nodes[node_id].rot.z=asinf(2*abcd/unitLength);//pitch
    	    nodes[node_id].rot.x=atan2f(2*acbd, 1 - 2*(y2+x2));//roll
    	}

		//convert quaternion to euler, because fuck me we're not doing this at runtime, fuck that
		//also this isn't even vfpu optimized so doing this at runtime makes even less sense
		/* nodes[node_id].rot.z=atan2f(2.f*(quatAngles.w*quatAngles.x+quatAngles.y*quatAngles.z), 1.f-2.f*(quatAngles.x*quatAngles.x+quatAngles.y*quatAngles.y));//roll
		nodes[node_id].rot.x=asinf(2.f*(quatAngles.w*quatAngles.y+quatAngles.x*quatAngles.z));//pitch
		nodes[node_id].rot.y=atan2f(2.f*(quatAngles.w*quatAngles.z+quatAngles.x*quatAngles.y), 1.f-2.f*(quatAngles.y*quatAngles.y+quatAngles.z*quatAngles.z));//yaw */
		/* nodes[node_id].rot.x=0.0f;
		nodes[node_id].rot.y=0.0f;
		nodes[node_id].rot.z=0.0f; */
	} else {
		nodes[node_id].rot.x=0.0f;
		nodes[node_id].rot.y=0.0f;
		nodes[node_id].rot.z=0.0f;
	}

	if(data->nodes[node_id].has_scale) {
		nodes[node_id].hasAnyTransforms = 1;
		nodes[node_id].scl.x=data->nodes[node_id].scale[0];
		nodes[node_id].scl.y=data->nodes[node_id].scale[1];
		nodes[node_id].scl.z=data->nodes[node_id].scale[2];
	} else {
		nodes[node_id].scl.x=1.0f;
		nodes[node_id].scl.y=1.0f;
		nodes[node_id].scl.z=1.0f;
	}

	if(data->nodes[node_id].has_matrix) {
		nodes[node_id].hasAnyTransforms = 1;
		nodes[node_id].matrix.x.x=data->nodes[node_id].matrix[0];
		nodes[node_id].matrix.x.y=data->nodes[node_id].matrix[1];
		nodes[node_id].matrix.x.z=data->nodes[node_id].matrix[2];
		nodes[node_id].matrix.x.w=data->nodes[node_id].matrix[3];
		nodes[node_id].matrix.y.x=data->nodes[node_id].matrix[4];
		nodes[node_id].matrix.y.y=data->nodes[node_id].matrix[5];
		nodes[node_id].matrix.y.z=data->nodes[node_id].matrix[6];
		nodes[node_id].matrix.y.w=data->nodes[node_id].matrix[7];
		nodes[node_id].matrix.z.x=data->nodes[node_id].matrix[8];
		nodes[node_id].matrix.z.y=data->nodes[node_id].matrix[9];
		nodes[node_id].matrix.z.z=data->nodes[node_id].matrix[10];
		nodes[node_id].matrix.z.w=data->nodes[node_id].matrix[11];
		nodes[node_id].matrix.w.x=data->nodes[node_id].matrix[12];
		nodes[node_id].matrix.w.y=data->nodes[node_id].matrix[13];
		nodes[node_id].matrix.w.z=data->nodes[node_id].matrix[14];
		nodes[node_id].matrix.w.w=data->nodes[node_id].matrix[15];
	}
}

void gaasGLTFLoad(const char* file, int miplevels, int fileoffset, int filesize, int overwriteColor, unsigned int newColor) {
    //init cgltf
	printf("free memory: %d bytes, %d kb, %d mb\n", iGetFreeMemory(), iGetFreeMemory()/1000, iGetFreeMemory()/1000000);
    cgltf_options options = {0};
	cgltf_result result = cgltf_parse_file(&options, file, &data, fileoffset, filesize);
	if (result == cgltf_result_success) {
		//printf("cgltf_result_success on parse_file\n");
		result = cgltf_load_buffers(&options, data, file);
	}
	if (result == cgltf_result_success) {
		//printf("cgltf_result_success on load_buffers\n");
		result = cgltf_validate(data);
	}
	/* if (result == cgltf_result_success) {
		printf("cgltf_result_success on validate\n");
	} */

	TotalMeshes = (unsigned)data->meshes_count;
	TotalNodes = (unsigned)data->nodes_count;
	TotalMaterials = (unsigned)data->materials_count;

	//calculate how much data we need
	/* if (result == cgltf_result_success) {
		printf("Type: %u\n", data->file_type);
		printf("Nodes: %u\n", TotalNodes);
		printf("Meshes: %u\n", TotalMeshes);
		printf("Materials: %u\n", TotalMaterials);
		printf("Parent Nodes: %u\n", TotalParents);
	} */

	nodes=(struct Node*)calloc(sizeof(struct Node), TotalNodes);
	meshes=(struct Mesh*)calloc(sizeof(struct Mesh), TotalMeshes);//create mesh array
	materials=(struct Material*)calloc(sizeof(struct Material), TotalMaterials);
	//printf("free memory after mesh array: %d bytes, %d kb, %d mb\n", iGetFreeMemory(), iGetFreeMemory()/1000, iGetFreeMemory()/1000000);

	//load materials into overall array
	for(int i=0; i<TotalMaterials; i++) {
		gaas_GLTF_internal_LoadMaterials(i, miplevels);
	}
	//load meshes into overall array
	for(int i=0; i<TotalMeshes; i++) {
		gaas_GLTF_internal_LoadMeshes(i, overwriteColor, newColor);
	}
	//load all nodes, ignoring their hierarchy because lmao
	for(int i=0; i<TotalNodes; i++) {
		gaas_GLTF_internal_LoadNodes(i);
	}

    cgltf_free(data);
}

int TotRenderLists;
int RenderListValid=0;
int* RenderList[32];


//render lists are used for selective rendering
//when you pass a value into selectRender the renderer will only draw Nodes that are in that specific list
void gaasGLTFLoadRenderList(const char* file, int offset, int size) {
	FILE* fp;
	char line[256];
	int iup=0;

	fp = fopen(file, "rb");
	fseek(fp, offset, SEEK_SET);

	while(fgets(line,255,fp) && ftell(fp)<=offset+size) {
		line[255]=0;
		if(strchr(line,'\n')) strchr(line,'\n')[0]=0;
		if(strchr(line,'\r')) strchr(line,'\r')[0]=0;
		char *value=strchr(line,'=');

        if(strstr(line,"TotChunk =")) { 
			char* Bitch;
			Bitch = strdup(value+2);
			sscanf(Bitch, "%d", &TotRenderLists);
			RenderListValid=1;
		}
		//printf("TotalLists: %d\n", TotRenderLists);

		if(RenderListValid==1) {
			int Int1;
			int Int2;
			char Name[256];
			if(sscanf(line,"chunk %d %d",&Int1,&Int2)==2) {
				RenderList[Int1] = (int*)calloc(sizeof(int), Int2+1);
				RenderList[Int1][0] = Int2; //first member of a render list will always be total number of nodes
			}

			if(sscanf(line,"node %d %d %s",&Int1,&Int2,Name)==3) {
				int NodeID = 0;
				//printf("Node Name: %s %d %d\n", Name, Int1, Int2);
				for(int i=0; i<TotalNodes; i++) {
					if(strcmp(nodes[i].name, Name)==0) {
						NodeID=i;
						break;
					} else {
						NodeID=-1;
					}
				}
		
				if(NodeID==-1) {
					printf("Node not found\n");
					RenderListValid=0;
					return;
				}
						
				RenderList[Int1][Int2+1] = NodeID;

				RenderListValid=1;
			}
		}
	}

	fclose(fp);
}

char debugText[128];

void gaasGLTFRender(int selectRender, int selectCamera, int usebb, int debugNode) {
	int OverallNodesToRender=0;

	if(selectRender>=TotRenderLists) {
		selectRender=-1;
	}

    sceGuColor(0xffffffff);

	sceGumMatrixMode(GU_MODEL);
	{
		ScePspFVector3 pos = {0, 0, 0};
		ScePspFVector3 rot = {0, 0, 0};
		ScePspFVector3 scl = {1, 1, 1};

		sceGumLoadIdentity();
		sceGumTranslate(&pos);
		sceGumRotateXYZ(&rot);
		sceGumScale(&scl);
	}

	//needed for bounding boxes to work
	sceGumDrawArray(GU_TRIANGLES, GU_VERTEX_32BITF | GU_TRANSFORM_3D, 0, 0, 0);

	if(selectRender<0 || RenderListValid==0) {
		OverallNodesToRender=TotalNodes;
	} else {
		OverallNodesToRender=RenderList[selectRender][0];
	}
	//printf("total: %d\n", OverallNodesToRender);

    sceGuFrontFace(GU_CCW);
	for(int i=0; i<OverallNodesToRender; i++) {
		int shortcut=0;
		if(selectRender<0 || RenderListValid==0) {
			shortcut=i;
		} else {
			shortcut = RenderList[selectRender][i+1];
		}
		//printf("shortcut: %d\n", shortcut);
		int meshid = nodes[shortcut].meshId;
		int matid = meshes[meshid].matId;

		if(shortcut==debugNode) {
			sprintf(debugText, "Current Node: %s\n", nodes[shortcut].name);
			sceGuDisable(GU_TEXTURE_2D);
		}

		sceGumMatrixMode(GU_MODEL);
		{
			ScePspFVector3 pos = {nodes[shortcut].pos.x, nodes[shortcut].pos.y, nodes[shortcut].pos.z};
			ScePspFVector3 rot = {nodes[shortcut].rot.x, nodes[shortcut].rot.x, nodes[shortcut].rot.x};
			ScePspFVector3 scl = {nodes[shortcut].scl.x, nodes[shortcut].scl.y, nodes[shortcut].scl.z};

			sceGumLoadIdentity();
			sceGumTranslate(&pos);
			sceGumRotateXYZ(&rot);
			sceGumScale(&scl);
		}

		if(materials[matid].tex!=NULL) {
			if(materials[matid].doubleSided==1) {
				sceGuDisable(GU_CULL_FACE);
			}
			/* if(materials[matid].unlit==1) {
				sceGuDisable(GU_LIGHTING);
			} */

			gaasGFXTextureMip(materials[matid].tex, materials[matid].filter_min, materials[matid].filter_mag, materials[matid].repeat_u, materials[matid].repeat_v, GU_TFX_MODULATE, GU_TEXTURE_AUTO, 0.75);
		} else {
			printf("Invalid texture data in Material %d %s\n", matid, materials[matid].name);
		}

		if(usebb==1 && nodes[shortcut].hasAnyTransforms==0) {
			sceGuBeginObject(GU_VERTEX_32BITF, 8, 0, meshes[meshid].bbox);
		}

		switch (meshes[meshid].meshType) {
			case 0:
				sceGumDrawArray(meshes[meshid].prim, meshes[meshid].vtype, meshes[meshid].vertCount, meshes[meshid].indices, meshes[meshid].UVColorNormalMesh);
				break;
			
			case 1:
				sceGumDrawArray(meshes[meshid].prim, meshes[meshid].vtype, meshes[meshid].vertCount, meshes[meshid].indices, meshes[meshid].UVNormalMesh);
				break;
			
			case 2:
				sceGumDrawArray(meshes[meshid].prim, meshes[meshid].vtype, meshes[meshid].vertCount, meshes[meshid].indices, meshes[meshid].UVColorMesh);
				break;

			case 3:
				sceGumDrawArray(meshes[meshid].prim, meshes[meshid].vtype, meshes[meshid].vertCount, meshes[meshid].indices, meshes[meshid].UVMesh);
				break;

			default:
				break;
		}
		if(usebb==1  && nodes[shortcut].hasAnyTransforms==0) {
			sceGuEndObject();
		}
		//sceGuEnable(GU_LIGHTING);
		sceGuEnable(GU_CULL_FACE);
		if(shortcut==debugNode) {
			sceGuEnable(GU_TEXTURE_2D);
		}
	}
    sceGuFrontFace(GU_CW);
	if(debugNode>-1 && debugNode<TotalNodes) {
		gaasDEBUGDrawString(debugText, 0, 64, 0xFF00FF00, 0);
	}
}

void gaasGLTFFree() {
	//printf("free 0\n");
	if(RenderListValid==1) {
		for(int i=0; i<TotRenderLists; i++){
			free(RenderList[i]);
		}
	}
	RenderListValid=0;

	for(int i=0; i<TotalMeshes; i++) {
		free(meshes[i].indices);
		meshes[i].indices=0;
		//printf("free %d.1\n", i);
		if(meshes[i].UVColorNormalMesh) free(meshes[i].UVColorNormalMesh);
		if(meshes[i].UVNormalMesh) free(meshes[i].UVNormalMesh);
		if(meshes[i].UVColorMesh) free(meshes[i].UVColorMesh);
		if(meshes[i].UVMesh) free(meshes[i].UVMesh);
		meshes[i].UVColorNormalMesh=0;
		meshes[i].UVNormalMesh=0;
		meshes[i].UVColorMesh=0;
		meshes[i].UVMesh=0;
		//printf("free %d.2\n", i);
	}

	for(int i=0; i<TotalMaterials; i++) {
		gaasIMAGEFreeMipmap(materials[i].tex);
		materials[i].tex=0;
		//printf("free %d.3\n", i);
	}

	free(materials);
	materials=0;
	//printf("free 1\n");
	free(meshes);
	meshes=0;
	//printf("free 2\n");
	free(nodes);
	meshes=0;
	//printf("free 3\n");
}