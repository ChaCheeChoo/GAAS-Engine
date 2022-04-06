#ifndef GLTFLOADER
#define GLTFLOADER

/**
 * Loads glTF scene
 * file - file to load, either .gltf/.glb or your game eboot if using gwd
 * miplevels - how many mipmap levels to use, 0 for none
 * fileoffset - if using gwd set using gaasGWDGetOffsetFromName, else set to 0
 * filesize - if using gwd set using gaasGWDGetSizeFromName, else set to 0
 * overwriteColor - flag to overwrite vertex color in scene, if 1 then newColor will be used
**/
void gaasGLTFLoad(const char* file, int miplevels, int fileoffset, int filesize, int overwriteColor, unsigned int newColor);

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
**/
void gaasGLTFLoadRenderList(const char* file, int offset, int size);

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
void gaasGLTFRender(int selectRender, int selectCamera, int usebb, int debugNode);

/**
 * Deletes scene from memory
**/
void gaasGLTFFree();

#endif