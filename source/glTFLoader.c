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

//temporary buffers used before data is written to main vertex list
struct ScePspFVector2* temp_tex;
struct ScePspFVector3* temp_normal;
struct ScePspFVector3* temp_pos;
struct Vector4UnsignedShort* temp_color;

void* tempImageBuf;

static gaasGLTF* temp = NULL;

void gaas_GLTF_internal_LoadMeshes(int mesh_id, int overwriteColor, unsigned int ColorToOverwriteWith) {
	//decide what primitive type a mesh will use
	switch (data->meshes->primitives->type) {
	    case cgltf_primitive_type_points:
	    	temp->meshes[mesh_id].prim=GU_POINTS;
	    	break;
    
	    case cgltf_primitive_type_lines:
	    	temp->meshes[mesh_id].prim=GU_LINES;
	    	break;

	    case cgltf_primitive_type_line_strip:
	    	temp->meshes[mesh_id].prim=GU_LINE_STRIP;
	    	break;
    
	    case cgltf_primitive_type_triangles:
	    	temp->meshes[mesh_id].prim=GU_TRIANGLES;
	    	break;

	    case cgltf_primitive_type_triangle_strip:
	    	temp->meshes[mesh_id].prim=GU_TRIANGLE_STRIP;
	    	break;
    
	    case cgltf_primitive_type_triangle_fan:
	    	temp->meshes[mesh_id].prim=GU_TRIANGLE_FAN;
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
	temp->meshes[mesh_id].meshType = -1;//mesh types: 0 - UVColorNormalVertex; 1 - UVNormalVertex; 2 - UVColorVertex; 3 - UVVertex; -1 - invalid

	//determine mesh type
	if(index_pos!=-1 && index_tex!=-1 && index_norm!=-1 && index_color!=-1) {
		temp->meshes[mesh_id].meshType=0;//all data is present
	} else if(index_pos!=-1 && index_tex!=-1 && index_norm!=-1 && index_color==-1) {
		temp->meshes[mesh_id].meshType=1;//color data is missing
	} else if(index_pos!=-1 && index_tex!=-1 && index_norm==-1 && index_color!=-1) {
		temp->meshes[mesh_id].meshType=2;//normal data is missing
	} else if(index_pos!=-1 && index_tex!=-1 && index_norm==-1 && index_color==-1) {
		temp->meshes[mesh_id].meshType=3;//both color and normal data is missing
	} else {
		temp->meshes[mesh_id].meshType=-1;//invalid data
		printf("Invalid data, unsupported vertex type|mesh: %d\n", mesh_id);
	}

	//printf("meshType: %d\n", temp->meshes[mesh_id].meshType);

	//generate actual vtype data
	switch (temp->meshes[mesh_id].meshType) {
		case 0:
			temp->meshes[mesh_id].vtype = GU_TEXTURE_32BITF|GU_COLOR_8888|GU_NORMAL_32BITF|GU_VERTEX_32BITF|GU_INDEX_16BIT|GU_TRANSFORM_3D;
			break;
		
		case 1:
			temp->meshes[mesh_id].vtype = GU_VERTEX_32BITF|GU_NORMAL_32BITF|GU_TEXTURE_32BITF|GU_INDEX_16BIT|GU_TRANSFORM_3D;
			break;
		
		case 2:
			temp->meshes[mesh_id].vtype = GU_VERTEX_32BITF|GU_TEXTURE_32BITF|GU_COLOR_8888|GU_INDEX_16BIT|GU_TRANSFORM_3D;
			break;
		
		case 3:
			temp->meshes[mesh_id].vtype = GU_TEXTURE_32BITF|GU_VERTEX_32BITF|GU_INDEX_16BIT|GU_TRANSFORM_3D;
			break;

		default:
			break;
	}
	

    //gets number of vertices
	temp->meshes[mesh_id].vertCount = data->meshes[mesh_id].primitives->indices->count;
	//meshes[mesh_id].name=(char*)malloc(sizeof(data->meshes[mesh_id].name));
	strncpy(temp->meshes[mesh_id].name, data->meshes[mesh_id].name, 255);
	
	if(data->meshes[mesh_id].primitives->attributes->data->has_min) {
		temp->meshes[mesh_id].min[0] = data->meshes[mesh_id].primitives->attributes[index_pos].data->min[0];
		temp->meshes[mesh_id].min[1] = data->meshes[mesh_id].primitives->attributes[index_pos].data->min[1];
		temp->meshes[mesh_id].min[2] = data->meshes[mesh_id].primitives->attributes[index_pos].data->min[2];
	}
	if(data->meshes[mesh_id].primitives->attributes->data->has_max) {
		temp->meshes[mesh_id].max[0] = data->meshes[mesh_id].primitives->attributes[index_pos].data->max[0];
		temp->meshes[mesh_id].max[1] = data->meshes[mesh_id].primitives->attributes[index_pos].data->max[1];
		temp->meshes[mesh_id].max[2] = data->meshes[mesh_id].primitives->attributes[index_pos].data->max[2];
	}
	if(data->meshes[mesh_id].primitives->attributes->data->has_min && data->meshes[mesh_id].primitives->attributes->data->has_max) {
		temp->meshes[mesh_id].bbox[0].x = temp->meshes[mesh_id].min[0];
		temp->meshes[mesh_id].bbox[0].y = temp->meshes[mesh_id].min[1];
		temp->meshes[mesh_id].bbox[0].z = temp->meshes[mesh_id].min[2];

		temp->meshes[mesh_id].bbox[1].x = temp->meshes[mesh_id].max[0];
		temp->meshes[mesh_id].bbox[1].y = temp->meshes[mesh_id].min[1];
		temp->meshes[mesh_id].bbox[1].z = temp->meshes[mesh_id].min[2];

		temp->meshes[mesh_id].bbox[2].x = temp->meshes[mesh_id].min[0];
		temp->meshes[mesh_id].bbox[2].y = temp->meshes[mesh_id].min[1];
		temp->meshes[mesh_id].bbox[2].z = temp->meshes[mesh_id].max[2];

		temp->meshes[mesh_id].bbox[3].x = temp->meshes[mesh_id].max[0];
		temp->meshes[mesh_id].bbox[3].y = temp->meshes[mesh_id].min[1];
		temp->meshes[mesh_id].bbox[3].z = temp->meshes[mesh_id].max[2];


		temp->meshes[mesh_id].bbox[4].x = temp->meshes[mesh_id].min[0];
		temp->meshes[mesh_id].bbox[4].y = temp->meshes[mesh_id].max[1];
		temp->meshes[mesh_id].bbox[4].z = temp->meshes[mesh_id].min[2];

		temp->meshes[mesh_id].bbox[5].x = temp->meshes[mesh_id].max[0];
		temp->meshes[mesh_id].bbox[5].y = temp->meshes[mesh_id].max[1];
		temp->meshes[mesh_id].bbox[5].z = temp->meshes[mesh_id].min[2];

		temp->meshes[mesh_id].bbox[6].x = temp->meshes[mesh_id].min[0];
		temp->meshes[mesh_id].bbox[6].y = temp->meshes[mesh_id].max[1];
		temp->meshes[mesh_id].bbox[6].z = temp->meshes[mesh_id].max[2];

		temp->meshes[mesh_id].bbox[7].x = temp->meshes[mesh_id].max[0];
		temp->meshes[mesh_id].bbox[7].y = temp->meshes[mesh_id].max[1];
		temp->meshes[mesh_id].bbox[7].z = temp->meshes[mesh_id].max[2];
	}

	//find matId because cgltf is a cunt that won't tell it to me directly
	for(int i=0; i<temp->TotalMaterials; i++) {
		if(strcmp(temp->materials[i].name, data->meshes[mesh_id].primitives->material->name)==0) {
			temp->meshes[mesh_id].matId=i;
			break;
		} else {
			temp->meshes[mesh_id].matId=-1;
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
	switch (temp->meshes[mesh_id].meshType) {
		case 0:
			//printf("Test 1.1: Allocating memory to mesh %d\n", mesh_id);
			temp->meshes[mesh_id].UVColorNormalMesh=(struct UVColorNormalVertex*)calloc(sizeof(struct UVColorNormalVertex), count);
			temp->meshes[mesh_id].UVNormalMesh=NULL;
			temp->meshes[mesh_id].UVColorMesh=NULL;
			temp->meshes[mesh_id].UVMesh=NULL;
			//printf("Test 1.2\n");
			break;
		
		case 1:
			//printf("Test 1.1: Allocating memory to mesh %d\n", mesh_id);
			temp->meshes[mesh_id].UVColorNormalMesh=NULL;
			temp->meshes[mesh_id].UVNormalMesh=(struct UVNormalVertex*)calloc(sizeof(struct UVNormalVertex), count);
			temp->meshes[mesh_id].UVColorMesh=NULL;
			temp->meshes[mesh_id].UVMesh=NULL;
			//printf("Test 1.2\n");
			break;
		
		case 2:
			//printf("Test 1.1: Allocating memory to mesh %d\n", mesh_id);
			temp->meshes[mesh_id].UVColorNormalMesh=NULL;
			temp->meshes[mesh_id].UVNormalMesh=NULL;
			temp->meshes[mesh_id].UVColorMesh=(struct UVColorVertex*)calloc(sizeof(struct UVColorVertex), count);
			temp->meshes[mesh_id].UVMesh=NULL;
			//printf("Test 1.2\n");
			break;
		
		case 3:
			//printf("Test 1.1: Allocating memory to mesh %d\n", mesh_id);
			temp->meshes[mesh_id].UVColorNormalMesh=NULL;
			temp->meshes[mesh_id].UVNormalMesh=NULL;
			temp->meshes[mesh_id].UVColorMesh=NULL;
			temp->meshes[mesh_id].UVMesh=(struct UVVertex*)calloc(sizeof(struct UVVertex), count);
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


	switch (temp->meshes[mesh_id].meshType) {
		case 0:
			for(int i=0; i<count_tex; i++) {
				temp->meshes[mesh_id].UVColorNormalMesh[i].u = temp_tex[i].x;
				temp->meshes[mesh_id].UVColorNormalMesh[i].v = temp_tex[i].y;
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
					temp->meshes[mesh_id].UVColorNormalMesh[i].color = GU_RGBA((int)tempXfloat, (int)tempYfloat, (int)tempZfloat, (int)tempWfloat);
				} else {
					temp->meshes[mesh_id].UVColorNormalMesh[i].color = ColorToOverwriteWith;
				}
				//printf("writing color: %d %d %d %d | %f %f %f %f | %x\n", temp_color[i].x, temp_color[i].y, temp_color[i].z, temp_color[i].w,  tempXfloat, tempYfloat, tempZfloat, tempWfloat,  meshes[mesh_id].UVColorNormalMesh[i].color);
			}
			for(int i=0; i<count_normal; i++) {
				temp->meshes[mesh_id].UVColorNormalMesh[i].nx = temp_normal[i].x;
				temp->meshes[mesh_id].UVColorNormalMesh[i].ny = temp_normal[i].y;
				temp->meshes[mesh_id].UVColorNormalMesh[i].nz = temp_normal[i].z;
			}
			for(int i=0; i<count; i++) {
				temp->meshes[mesh_id].UVColorNormalMesh[i].x = temp_pos[i].x;
				temp->meshes[mesh_id].UVColorNormalMesh[i].y = temp_pos[i].y;
				temp->meshes[mesh_id].UVColorNormalMesh[i].z = temp_pos[i].z;
			}
			break;
		
		case 1:
			for(int i=0; i<count_tex; i++) {
				temp->meshes[mesh_id].UVNormalMesh[i].u = temp_tex[i].x;
				temp->meshes[mesh_id].UVNormalMesh[i].v = temp_tex[i].y;
			}
			for(int i=0; i<count_normal; i++) {
				temp->meshes[mesh_id].UVNormalMesh[i].nx = temp_normal[i].x;
				temp->meshes[mesh_id].UVNormalMesh[i].ny = temp_normal[i].y;
				temp->meshes[mesh_id].UVNormalMesh[i].nz = temp_normal[i].z;
			}
			for(int i=0; i<count; i++) {
				temp->meshes[mesh_id].UVNormalMesh[i].x = temp_pos[i].x;
				temp->meshes[mesh_id].UVNormalMesh[i].y = temp_pos[i].y;
				temp->meshes[mesh_id].UVNormalMesh[i].z = temp_pos[i].z;
			}
			break;
		
		case 2:
			for(int i=0; i<count_tex; i++) {
				temp->meshes[mesh_id].UVColorMesh[i].u = temp_tex[i].x;
				temp->meshes[mesh_id].UVColorMesh[i].v = temp_tex[i].y;
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
					temp->meshes[mesh_id].UVColorMesh[i].color = GU_RGBA((int)tempXfloat, (int)tempYfloat, (int)tempZfloat, (int)tempWfloat);
				} else {
					temp->meshes[mesh_id].UVColorMesh[i].color = ColorToOverwriteWith;
				}
			}
			for(int i=0; i<count; i++) {
				temp->meshes[mesh_id].UVColorMesh[i].x = temp_pos[i].x;
				temp->meshes[mesh_id].UVColorMesh[i].y = temp_pos[i].y;
				temp->meshes[mesh_id].UVColorMesh[i].z = temp_pos[i].z;
			}
			break;
		
		case 3:
			for(int i=0; i<count_tex; i++) {
				temp->meshes[mesh_id].UVMesh[i].u = temp_tex[i].x;
				temp->meshes[mesh_id].UVMesh[i].v = temp_tex[i].y;
			}
			for(int i=0; i<count; i++) {
				temp->meshes[mesh_id].UVMesh[i].x = temp_pos[i].x;
				temp->meshes[mesh_id].UVMesh[i].y = temp_pos[i].y;
				temp->meshes[mesh_id].UVMesh[i].z = temp_pos[i].z;
			}
			break;

		default:
			break;
	}

    int buf_size_indices = data->meshes[mesh_id].primitives->indices->buffer_view->size;
    int buf_offset_indices = data->meshes[mesh_id].primitives->indices->buffer_view->offset;
    temp->meshes[mesh_id].indices=(unsigned short*)malloc(buf_size_indices);//allocate space for indices
    memcpy(temp->meshes[mesh_id].indices, &data->meshes[mesh_id].primitives->indices->buffer_view->buffer->data[buf_offset_indices], buf_size_indices);//copy buffer data directly into indices buffer

	if(temp_pos!=NULL)   free(temp_pos);
	if(temp_tex!=NULL)   free(temp_tex);
	if(temp_normal!=NULL)free(temp_normal);
	if(temp_color!=NULL) free(temp_color);
}

void gaas_GLTF_internal_LoadMaterials(int mat_id, int miplevels) {
	//basic material info
	strncpy(temp->materials[mat_id].name, data->materials[mat_id].name, 255);
	temp->materials[mat_id].doubleSided = data->materials[mat_id].double_sided;
	temp->materials[mat_id].unlit = data->materials[mat_id].unlit;
	temp->materials[mat_id].alphaMode = data->materials[mat_id].alpha_mode;
	//printf("%d  %s %d %d %d\n", mat_id, materials[mat_id].name, materials[mat_id].doubleSided, materials[mat_id].unlit, materials[mat_id].alphaMode);

	//printf("Mat %d tex %s\n", mat_id, data->materials[mat_id].pbr_metallic_roughness.base_color_texture.texture->image->name);

	//set up filtering and wrapping info
	switch(data->materials[mat_id].pbr_metallic_roughness.base_color_texture.texture->sampler->wrap_s) {
		case 33071:
			temp->materials[mat_id].repeat_u=GU_CLAMP;
			break;
		case 33648:
			temp->materials[mat_id].repeat_u=GU_CLAMP;
			break;
		case 10497:
			temp->materials[mat_id].repeat_u=GU_REPEAT;
			break;
		default:
			break;
	}
	switch(data->materials[mat_id].pbr_metallic_roughness.base_color_texture.texture->sampler->wrap_t) {
		case 33071:
			temp->materials[mat_id].repeat_v=GU_CLAMP;
			break;
		case 33648:
			temp->materials[mat_id].repeat_v=GU_CLAMP;
			break;
		case 10497:
			temp->materials[mat_id].repeat_v=GU_REPEAT;
			break;
		default:
			break;
	}
	switch(data->materials[mat_id].pbr_metallic_roughness.base_color_texture.texture->sampler->mag_filter) {
		case 9728:
			temp->materials[mat_id].filter_mag=GU_NEAREST;
			break;
		case 9729:
			temp->materials[mat_id].filter_mag=GU_LINEAR;
			break;
		default:
			break;
	}
	switch(data->materials[mat_id].pbr_metallic_roughness.base_color_texture.texture->sampler->min_filter) {
		case 9728:
			if(miplevels>0) {
				temp->materials[mat_id].filter_min=GU_NEAREST_MIPMAP_LINEAR;
			} else {
				temp->materials[mat_id].filter_min=GU_NEAREST;
			}
			break;
		case 9729:
			if(miplevels>0) {
				temp->materials[mat_id].filter_min=GU_LINEAR_MIPMAP_LINEAR;
			} else {
				temp->materials[mat_id].filter_min=GU_LINEAR;
			}
			break;
		case 9984:
			temp->materials[mat_id].filter_min=GU_NEAREST_MIPMAP_NEAREST;
			break;
		case 9985:
			temp->materials[mat_id].filter_min=GU_LINEAR_MIPMAP_NEAREST;
			break;
		case 9986:
			temp->materials[mat_id].filter_min=GU_NEAREST_MIPMAP_LINEAR;
			break;
		case 9987:
			temp->materials[mat_id].filter_min=GU_LINEAR_MIPMAP_LINEAR;
			break;
		default:
			break;
	}

	//load texture data
	int imageOffset = data->materials[mat_id].pbr_metallic_roughness.base_color_texture.texture->image->buffer_view->offset;
	int imageSize = data->materials[mat_id].pbr_metallic_roughness.base_color_texture.texture->image->buffer_view->size;
	tempImageBuf=(char*)malloc(imageSize);
	memcpy(tempImageBuf, &data->materials[mat_id].pbr_metallic_roughness.base_color_texture.texture->image->buffer_view->buffer->data[imageOffset], imageSize);
	temp->materials[mat_id].tex = gaasIMAGELoadImageMipmapFromBuffer(tempImageBuf, imageSize, 1, miplevels);
	if(temp->materials[mat_id].tex==NULL) {
		printf("failed to load texture for %s\n", temp->materials[mat_id].name);
	}
	gaasIMAGEMoveMipmapToVram(temp->materials[mat_id].tex);

	free(tempImageBuf);
}

ScePspFVector3 ConvertTheMessOfFuckThatIsQuaternionsToEulerAngles(ScePspFVector4 q) {
	ScePspFVector3 euler;

	float unit = (q.x * q.x) + (q.y * q.y) + (q.z * q.z) + (q.w * q.w);

	// this will have a magnitude of 0.5 or greater if and only if this is a singularity case
	float test = q.x * q.w - q.y * q.z;

    if (test > 0.4995f * unit) { // singularity at north pole
        euler.x = GU_PI / 2;
        euler.y = 2.0f * atan2(q.y, q.x);
        euler.z = 0;
    } else if (test < -0.4995f * unit) { // singularity at south pole
        euler.x = GU_PI / 2;
        euler.y = -2.0f * atan2(q.y, q.x);
        euler.z = 0;
    } else { // no singularity - this is the majority of cases
        euler.x = asin(2.0f * (q.w * q.x - q.y * q.z));
        euler.y = atan2(2.0f * q.w * q.y + 2.0f * q.z * q.x, 1 - 2.0f * (q.x * q.x + q.y * q.y));
        euler.z = atan2(2.0f * q.w * q.z + 2.0f * q.x * q.y, 1 - 2.0f * (q.z * q.z + q.x * q.x));
    }

	return euler;
}

void gaas_GLTF_internal_LoadNodes(int node_id) {
	//get current node name
	strncpy(temp->nodes[node_id].name, data->nodes[node_id].name, 255);
	temp->nodes[node_id].hasAnyTransforms = 0;

	temp->nodes[node_id].animId = -1;
	temp->nodes[node_id].totChannels = 0;

	//find mesh_id because cgltf is a cunt that won't tell it to me directly
	for(int i=0; i<temp->TotalMeshes; i++) {
		if(strcmp(temp->meshes[i].name, data->nodes[node_id].mesh->name)==0) {
			temp->nodes[node_id].meshId=i;
			break;
		} else {
			temp->nodes[node_id].meshId=-1;
		}
	}
	//printf("Mesh in Node %d: %d %s\n", node_id, nodes[node_id].meshId, nodes[node_id].name);

	//load the fucking translation shit
	if(data->nodes[node_id].has_translation) {
		temp->nodes[node_id].hasAnyTransforms = 1;
		temp->nodes[node_id].pos.x=data->nodes[node_id].translation[0];
		temp->nodes[node_id].pos.y=data->nodes[node_id].translation[1];
		temp->nodes[node_id].pos.z=data->nodes[node_id].translation[2];
	} else {
		temp->nodes[node_id].pos.x=0.0f;
		temp->nodes[node_id].pos.y=0.0f;
		temp->nodes[node_id].pos.z=0.0f;
	}

	//quaternions were a mistake
	if(data->nodes[node_id].has_rotation) { //fucking wack
		temp->nodes[node_id].hasAnyTransforms = 1;

		ScePspFVector4 q;
		q.x = data->nodes[node_id].rotation[0];
		q.y = data->nodes[node_id].rotation[1];
		q.z = data->nodes[node_id].rotation[2];
		q.w = data->nodes[node_id].rotation[3];

		temp->nodes[node_id].rot = ConvertTheMessOfFuckThatIsQuaternionsToEulerAngles(q);

		//printf("rotate my balls %f %f %f\n", nodes[node_id].rot.x, nodes[node_id].rot.y, nodes[node_id].rot.z);
	} else {
		temp->nodes[node_id].rot.x=0.0f;
		temp->nodes[node_id].rot.y=0.0f;
		temp->nodes[node_id].rot.z=0.0f;
	}

	if(data->nodes[node_id].has_scale) {
		temp->nodes[node_id].hasAnyTransforms = 1;
		temp->nodes[node_id].scl.x=data->nodes[node_id].scale[0];
		temp->nodes[node_id].scl.y=data->nodes[node_id].scale[1];
		temp->nodes[node_id].scl.z=data->nodes[node_id].scale[2];
	} else {
		temp->nodes[node_id].scl.x=1.0f;
		temp->nodes[node_id].scl.y=1.0f;
		temp->nodes[node_id].scl.z=1.0f;
	}

	if(data->nodes[node_id].has_matrix) {
		temp->nodes[node_id].hasAnyTransforms = 1;
		temp->nodes[node_id].matrix.x.x=data->nodes[node_id].matrix[0];
		temp->nodes[node_id].matrix.x.y=data->nodes[node_id].matrix[1];
		temp->nodes[node_id].matrix.x.z=data->nodes[node_id].matrix[2];
		temp->nodes[node_id].matrix.x.w=data->nodes[node_id].matrix[3];
		temp->nodes[node_id].matrix.y.x=data->nodes[node_id].matrix[4];
		temp->nodes[node_id].matrix.y.y=data->nodes[node_id].matrix[5];
		temp->nodes[node_id].matrix.y.z=data->nodes[node_id].matrix[6];
		temp->nodes[node_id].matrix.y.w=data->nodes[node_id].matrix[7];
		temp->nodes[node_id].matrix.z.x=data->nodes[node_id].matrix[8];
		temp->nodes[node_id].matrix.z.y=data->nodes[node_id].matrix[9];
		temp->nodes[node_id].matrix.z.z=data->nodes[node_id].matrix[10];
		temp->nodes[node_id].matrix.z.w=data->nodes[node_id].matrix[11];
		temp->nodes[node_id].matrix.w.x=data->nodes[node_id].matrix[12];
		temp->nodes[node_id].matrix.w.y=data->nodes[node_id].matrix[13];
		temp->nodes[node_id].matrix.w.z=data->nodes[node_id].matrix[14];
		temp->nodes[node_id].matrix.w.w=data->nodes[node_id].matrix[15];
	}
}

void gaas_GLTF_internal_LoadAnims(int anim_id) {
	temp->animations[anim_id].totalChan = data->animations[anim_id].channels_count;

	temp->animations[anim_id].channels=(struct AnimChannel*)calloc(sizeof(struct AnimChannel), temp->animations[anim_id].totalChan);

	for(int i=0; i<temp->animations[anim_id].totalChan; i++) {
		//load misc. animation data
		temp->animations[anim_id].channels[i].interpolationType = data->animations[anim_id].channels[i].sampler->interpolation;
		temp->animations[anim_id].channels[i].animType = data->animations[anim_id].channels[i].target_path;
		for(int y=0; y<temp->TotalNodes; y++) {
			if(strcmp(temp->nodes[y].name, data->animations[anim_id].channels[i].target_node->name)==0) {
				temp->animations[anim_id].channels[i].nodeID=y;
				temp->nodes[temp->animations[anim_id].channels[i].nodeID].animId = anim_id;
				temp->nodes[temp->animations[anim_id].channels[i].nodeID].totChannels++;
				break;
			} else {
				temp->animations[anim_id].channels[i].nodeID=-1;
			}
		}

		//input/output count, 1 for every blender time value in animation
		temp->animations[anim_id].channels[i].inputCount = data->animations[anim_id].channels[i].sampler->input->count;
		temp->animations[anim_id].channels[i].outputCount = data->animations[anim_id].channels[i].sampler->output->count;

		//printf("out: %d  in: %d\n", animations[anim_id].channels[i].outputCount, animations[anim_id].channels[i].inputCount);

		//allocate buffers for animation data
		temp->animations[anim_id].channels[i].input=(float*)calloc(sizeof(float), temp->animations[anim_id].channels[i].inputCount);
		temp->animations[anim_id].channels[i].output=(struct ScePspFVector3*)calloc(sizeof(struct ScePspFVector3), temp->animations[anim_id].channels[i].outputCount);

		struct ScePspFVector4* temp_rot_buf = (struct ScePspFVector4*)calloc(sizeof(struct ScePspFVector4), temp->animations[anim_id].channels[i].outputCount);

		//copy input buffer into memory
		int buf_size_input = data->animations[anim_id].channels[i].sampler->input->buffer_view->size;
    	int buf_offset_input = data->animations[anim_id].channels[i].sampler->input->buffer_view->offset;
    	memcpy(temp->animations[anim_id].channels[i].input, &data->animations[anim_id].channels[i].sampler->input->buffer_view->buffer->data[buf_offset_input], buf_size_input);//copy buffer data into input buffer

		//copy output buffer into memory
		if(temp->animations[anim_id].channels[i].animType!=animation_path_type_rotation) { //if not rotation load directly into buffer
			int buf_size_output = data->animations[anim_id].channels[i].sampler->output->buffer_view->size;
    		int buf_offset_output = data->animations[anim_id].channels[i].sampler->output->buffer_view->offset;
    		memcpy(temp->animations[anim_id].channels[i].output, &data->animations[anim_id].channels[i].sampler->output->buffer_view->buffer->data[buf_offset_output], buf_size_output);//copy buffer data into input buffer
		} else { //else convert to euler first
			int buf_size_output = data->animations[anim_id].channels[i].sampler->output->buffer_view->size;
    		int buf_offset_output = data->animations[anim_id].channels[i].sampler->output->buffer_view->offset;
    		memcpy(temp_rot_buf, &data->animations[anim_id].channels[i].sampler->output->buffer_view->buffer->data[buf_offset_output], buf_size_output);//copy buffer data into input buffer

			for(int h=0; h<temp->animations[anim_id].channels[i].outputCount; h++) {
				temp->animations[anim_id].channels[i].output[h] = ConvertTheMessOfFuckThatIsQuaternionsToEulerAngles(temp_rot_buf[h]);
			}
		}

		free(temp_rot_buf);

		/* for(int s=0; s<animations[anim_id].channels[i].inputCount; s++) {
			printf("%d: %f\n", s, animations[anim_id].channels[i].input[s]);
		} */
		//printf("channel %d: %d %d %d %d %d\n", anim_id, animations[anim_id].channels[i].input, animations[anim_id].channels[i].output, animations[anim_id].channels[i].interpolationType, animations[anim_id].channels[i].animType, animations[anim_id].channels[i].nodeID);
	}
}

gaasGLTF* gaasGLTFLoad(const char* file, int miplevels, int fileoffset, int filesize, int overwriteColor, unsigned int newColor) {
	temp = malloc(sizeof(struct gaasGLTF));

    //reset animations
	temp->currentTime = 0.0f;
	temp->currentFrame=0;
	
	//init cgltf
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

	temp->TotalMeshes = (unsigned)data->meshes_count;
	temp->TotalNodes = (unsigned)data->nodes_count;
	temp->TotalMaterials = (unsigned)data->materials_count;
	temp->TotalAnims = (unsigned)data->animations_count;

	//calculate how much data we need
	/* if (result == cgltf_result_success) {
		printf("Type: %u\n", data->file_type);
		printf("Nodes: %u\n", temp->TotalMeshes);
		printf("Meshes: %u\n", temp->TotalMeshes);
		printf("Materials: %u\n", temp->TotalMaterials);
	} */

	temp->nodes=(struct Node*)calloc(sizeof(struct Node), temp->TotalNodes);
	temp->meshes=(struct Mesh*)calloc(sizeof(struct Mesh), temp->TotalMeshes);//create mesh array
	temp->materials=(struct Material*)calloc(sizeof(struct Material), temp->TotalMaterials);
	temp->animations=(struct Animation*)calloc(sizeof(struct Animation), temp->TotalAnims);

	//load materials into overall array
	for(int i=0; i<temp->TotalMaterials; i++) {
		gaas_GLTF_internal_LoadMaterials(i, miplevels);
	}
	//load meshes into overall array
	for(int i=0; i<temp->TotalMeshes; i++) {
		gaas_GLTF_internal_LoadMeshes(i, overwriteColor, newColor);
	}
	//load all nodes, ignoring their hierarchy because lmao
	for(int i=0; i<temp->TotalNodes; i++) {
		gaas_GLTF_internal_LoadNodes(i);
	}
	//load all animations
	for(int i=0; i<temp->TotalAnims; i++) {
		gaas_GLTF_internal_LoadAnims(i);
	}

    cgltf_free(data);
	return temp;
}

//render lists are used for selective rendering
//when you pass a value into selectRender the renderer will only draw Nodes that are in that specific list
void gaasGLTFLoadRenderList(gaasGLTF* scene, const char* file, int offset, int size) {
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
			sscanf(Bitch, "%d", &scene->TotRenderLists);
			scene->RenderListValid=1;
		}
		//printf("TotalLists: %d\n", TotRenderLists);

		if(scene->RenderListValid==1) {
			int Int1;
			int Int2;
			char Name[256];
			if(sscanf(line,"chunk %d %d",&Int1,&Int2)==2) {
				scene->RenderList[Int1] = (int*)calloc(sizeof(int), Int2+1);
				scene->RenderList[Int1][0] = Int2; //first member of a render list will always be total number of nodes
			}

			if(sscanf(line,"node %d %d %s",&Int1,&Int2,Name)==3) {
				int NodeID = 0;
				//printf("Node Name: %s %d %d\n", Name, Int1, Int2);
				for(int i=0; i<scene->TotalNodes; i++) {
					if(strcmp(scene->nodes[i].name, Name)==0) {
						NodeID=i;
						break;
					} else {
						NodeID=-1;
					}
				}
		
				if(NodeID==-1) {
					printf("Node not found\n");
					scene->RenderListValid=0;
					return;
				}
						
				scene->RenderList[Int1][Int2+1] = NodeID;

				scene->RenderListValid=1;
			}
		}
	}

	fclose(fp);
}

char debugText[272];

void gaasGLTFRender(gaasGLTF* scene, int selectRender, int selectCamera, int usebb, int debugNode) {
	int OverallNodesToRender=0;

	if(selectRender>=scene->TotRenderLists) {
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

	if(selectRender<0 || scene->RenderListValid==0) {
		OverallNodesToRender=scene->TotalNodes;
	} else {
		OverallNodesToRender=scene->RenderList[selectRender][0];
	}
	//printf("total: %d\n", OverallNodesToRender);

    sceGuFrontFace(GU_CCW);
	for(int i=0; i<OverallNodesToRender; i++) {
		int shortcut=0;
		if(selectRender<0 || scene->RenderListValid==0) {
			shortcut=i;
		} else {
			shortcut = scene->RenderList[selectRender][i+1];
		}
		//printf("shortcut: %d\n", shortcut);
		int meshid = scene->nodes[shortcut].meshId;
		int matid = scene->meshes[meshid].matId;
		int animid = scene->nodes[shortcut].animId;

		if(shortcut==debugNode) {
			sprintf(debugText, "Current Node: %s\n", scene->nodes[shortcut].name);
			sceGuDisable(GU_TEXTURE_2D);
		}

		if(animid>-1) {
			scene->currentTime+=0.01; //tC

			//time between two frames | tD
			float difference = scene->animations[animid].channels[0].input[1]-scene->animations[animid].channels[0].input[0]; //for time channel doesn't matter

			//when currentTime elapses the difference, increment currentFrame
			if(scene->currentTime>difference) {
				scene->currentFrame++;
				scene->currentTime=0;
			}

			//reset animation when end is reached
			if(scene->currentFrame>=scene->animations[animid].channels[0].inputCount) {
				scene->currentFrame=0;
			}
			
			float tK = scene->animations[animid].channels[0].input[scene->currentFrame];

			float T = (scene->currentTime-tK)/difference;

			int previousFrame = 0;
			int nextFrame = 2;

			ScePspFVector3 pos;
			ScePspFVector3 rot;
			ScePspFVector3 scl;

			switch (scene->animations[animid].channels[0].interpolationType) {
				case interpolation_type_step:
					pos = scene->animations[animid].channels[0].output[scene->currentFrame];
					rot = scene->animations[animid].channels[1].output[scene->currentFrame];
					scl = scene->animations[animid].channels[2].output[scene->currentFrame];
					break;
				
				case interpolation_type_linear:
					pos.x = (1.0-T)*scene->animations[animid].channels[0].output[scene->currentFrame].x+T*scene->animations[animid].channels[0].output[scene->currentFrame+1].x;
					rot.x = (1.0-T)*scene->animations[animid].channels[1].output[scene->currentFrame].x+T*scene->animations[animid].channels[1].output[scene->currentFrame+1].x;
					scl.x = (1.0-T)*scene->animations[animid].channels[2].output[scene->currentFrame].x+T*scene->animations[animid].channels[2].output[scene->currentFrame+1].x;

					pos.y = (1.0-T)*scene->animations[animid].channels[0].output[scene->currentFrame].y+T*scene->animations[animid].channels[0].output[scene->currentFrame+1].y;
					rot.y = (1.0-T)*scene->animations[animid].channels[1].output[scene->currentFrame].y+T*scene->animations[animid].channels[1].output[scene->currentFrame+1].y;
					scl.y = (1.0-T)*scene->animations[animid].channels[2].output[scene->currentFrame].y+T*scene->animations[animid].channels[2].output[scene->currentFrame+1].y;

					pos.z = (1.0-T)*scene->animations[animid].channels[0].output[scene->currentFrame].z+T*scene->animations[animid].channels[0].output[scene->currentFrame+1].z;
					rot.z = (1.0-T)*scene->animations[animid].channels[1].output[scene->currentFrame].z+T*scene->animations[animid].channels[1].output[scene->currentFrame+1].z;
					scl.z = (1.0-T)*scene->animations[animid].channels[2].output[scene->currentFrame].z+T*scene->animations[animid].channels[2].output[scene->currentFrame+1].z;
					break;
				
				case interpolation_type_cubic_spline:
					pos = scene->animations[animid].channels[0].output[scene->currentFrame];
					rot = scene->animations[animid].channels[1].output[scene->currentFrame];
					scl = scene->animations[animid].channels[2].output[scene->currentFrame];
					break;

				default:
					break;
			}

			sceGumMatrixMode(GU_MODEL);
			{
				sceGumLoadIdentity();
				sceGumTranslate(&pos);
				sceGumRotateXYZ(&rot);
				sceGumScale(&scl);
			}
		} else {
			sceGumMatrixMode(GU_MODEL);
			{
				ScePspFVector3 pos = {scene->nodes[shortcut].pos.x, scene->nodes[shortcut].pos.y, scene->nodes[shortcut].pos.z};
				ScePspFVector3 rot = {scene->nodes[shortcut].rot.x, scene->nodes[shortcut].rot.y, scene->nodes[shortcut].rot.z};
				ScePspFVector3 scl = {scene->nodes[shortcut].scl.x, scene->nodes[shortcut].scl.y, scene->nodes[shortcut].scl.z};

				sceGumLoadIdentity();
				sceGumTranslate(&pos);
				sceGumRotateXYZ(&rot);
				sceGumScale(&scl);
			}
		}

		if(scene->materials[matid].tex!=NULL) {
			if(scene->materials[matid].doubleSided==1) {
				sceGuDisable(GU_CULL_FACE);
			}
			/* if(materials[matid].unlit==1) {
				sceGuDisable(GU_LIGHTING);
			} */
			
			gaasGFXTextureMip(scene->materials[matid].tex, scene->materials[matid].filter_min, scene->materials[matid].filter_mag, scene->materials[matid].repeat_u, scene->materials[matid].repeat_v, GU_TFX_MODULATE, GU_TEXTURE_AUTO, 0.75);
		} else {
			printf("Invalid texture data in Material %d %s\n", matid, scene->materials[matid].name);
		}

		if(usebb==1 && scene->nodes[shortcut].hasAnyTransforms==0) {
			//sceGumDrawArray(GU_POINTS, GU_VERTEX_32BITF | GU_TRANSFORM_3D, 8, 0, meshes[meshid].bbox);
			sceGuBeginObject(GU_VERTEX_32BITF, 8, 0, scene->meshes[meshid].bbox);
		}

		switch (scene->meshes[meshid].meshType) {
			case 0:
				sceGumDrawArray(scene->meshes[meshid].prim, scene->meshes[meshid].vtype, scene->meshes[meshid].vertCount, scene->meshes[meshid].indices, scene->meshes[meshid].UVColorNormalMesh);
				break;
			
			case 1:
				sceGumDrawArray(scene->meshes[meshid].prim, scene->meshes[meshid].vtype, scene->meshes[meshid].vertCount, scene->meshes[meshid].indices, scene->meshes[meshid].UVNormalMesh);
				break;
			
			case 2:
				sceGumDrawArray(scene->meshes[meshid].prim, scene->meshes[meshid].vtype, scene->meshes[meshid].vertCount, scene->meshes[meshid].indices, scene->meshes[meshid].UVColorMesh);
				break;

			case 3:
				sceGumDrawArray(scene->meshes[meshid].prim, scene->meshes[meshid].vtype, scene->meshes[meshid].vertCount, scene->meshes[meshid].indices, scene->meshes[meshid].UVMesh);
				break;

			default:
				break;
		}
		if(usebb==1 && scene->nodes[shortcut].hasAnyTransforms==0) {
			sceGuEndObject();
		}
		//sceGuEnable(GU_LIGHTING);
		sceGuEnable(GU_CULL_FACE);
		if(shortcut==debugNode) {
			sceGuEnable(GU_TEXTURE_2D);
		}
	}
    sceGuFrontFace(GU_CW);
	if(debugNode>-1 && debugNode<scene->TotalNodes) {
		gaasDEBUGDrawString(debugText, 0, 64, 0xFF00FF00, 0);
	}
}

void gaasGLTFFree(gaasGLTF* scene) {
	//printf("free 0\n");
	if(scene->RenderListValid==1) {
		for(int i=0; i<scene->TotRenderLists; i++){
			free(scene->RenderList[i]);
		}
	}
	scene->RenderListValid=0;

	for(int i=0; i<scene->TotalMeshes; i++) {
		free(scene->meshes[i].indices);
		scene->meshes[i].indices=0;
		//printf("free %d.1\n", i);
		if(scene->meshes[i].UVColorNormalMesh) free(scene->meshes[i].UVColorNormalMesh);
		if(scene->meshes[i].UVNormalMesh) free(scene->meshes[i].UVNormalMesh);
		if(scene->meshes[i].UVColorMesh) free(scene->meshes[i].UVColorMesh);
		if(scene->meshes[i].UVMesh) free(scene->meshes[i].UVMesh);
		scene->meshes[i].UVColorNormalMesh=0;
		scene->meshes[i].UVNormalMesh=0;
		scene->meshes[i].UVColorMesh=0;
		scene->meshes[i].UVMesh=0;
		//printf("free %d.2\n", i);
	}

	for(int i=0; i<scene->TotalMaterials; i++) {
		gaasIMAGEFreeMipmap(scene->materials[i].tex);
		scene->materials[i].tex=0;
		//printf("free %d.3\n", i);
	}

	for(int i=0; i<scene->TotalAnims; i++) {
		for (int j=0; j<scene->animations[i].totalChan; j++) {
			free(scene->animations[i].channels[j].input);
			scene->animations[i].channels[j].input=0;
			free(scene->animations[i].channels[j].output);
			scene->animations[i].channels[j].output=0;
		}
		free(scene->animations[i].channels);
		scene->animations[i].channels=0;
	}

	free(scene->animations);
	scene->animations=0;
	free(scene->materials);
	scene->materials=0;
	//printf("free 1\n");
	free(scene->meshes);
	scene->meshes=0;
	//printf("free 2\n");
	free(scene->nodes);
	scene->meshes=0;
	//printf("free 3\n");
}