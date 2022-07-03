#ifndef GWDHEADER
#define GWDHEADER

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

struct GWDheader { //size: 28 bytes
    uint8_t signature[4]; //signature/magic: GWD\0
    uint16_t version[2]; //version number: 0 - major version; 1 - minor version
    unsigned int filesOffset;
    unsigned int ToCentries; //number of files
    unsigned int ToCtotalSize;
    unsigned int ToCblockSize;
    unsigned int ToCoffset;
};

struct GWDToCblock { //size: 16 bytes
    unsigned int entry;
    unsigned int name_hash;
    unsigned int offset;
    unsigned int filesize;
};

struct PBPheader {
	uint8_t signature[4];
	uint16_t version[2];
	unsigned int offset[8];
};

typedef struct gaasGWD {
	int PSAROffset;
	struct GWDheader header;
	struct GWDToCblock *ToC;
} gaasGWD;

/**
 * How to load files from gwd
 * This applies to most asset loaders in the engine
 * If you plan to load a file from gwd you have to set the "usesoffset" variable in most loaders to 1;
 * Set the filepath to be "./EBOOT.PBP" or "./path-to-gwd", depending on what you're using
 * then supply a "fileoffset" using gaasGWDGetOffsetFromName
 * and, if needed, supply a "filesize" using gaasGWDGetSizeFromName
 * when using either of those functions you have to remember to type out the full path and put ./ at the start, otherwise it will not work
 * to generate a .gwd file simply run the GWDFileGen program, located in the root directory of the engine, in a terminal and give it your gwd name and asset directory
 * "GWDFileGen data ./Data"
 * NOTE: this program is compiled for Linux by default, you'll have to manually recompile it for other platforms using the code located in the GWDGenSource directory
**/

/**
 * Loads .gwd file
 * 
 * returns: gaasGWD structure
**/
gaasGWD* gaasGWDLoad(const char* file);

/**
 * Loads .gwd file that's located in the PSAR section of an eboot
 * 
 * returns: gaasGWD structure
**/
gaasGWD* gaasGWDLoadFromEBOOT(const char* eboot);

/**
 * Prints info about the supplied .gwd file
 * source - gaasGWD structure
**/
void gaasGWDTest(gaasGWD* source);

/**
 * Gets offset of specific file in gwd index
 * source - gaasGWD structure you're loading assets from
 * filename - complete path to file in gwd
 *      when seeking for a file remember to type out the full path and put ./ at the start, otherwise it will not work
 * 
 * returns: offset of file
**/
int gaasGWDGetOffsetFromName(gaasGWD* source, const char* filename); 

/**
 * Gets size of specific file in gwd index
 * source - gaasGWD structure you're loading assets from
 * filename - complete path to file in gwd
 *      when seeking for a file remember to type out the full path and put ./ at the start, otherwise it will not work
 * 
 * returns: size of file
**/
int gaasGWDGetSizeFromName(gaasGWD* source, const char* filename);

#endif