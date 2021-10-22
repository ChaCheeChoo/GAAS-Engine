#ifndef GWD
#define GWD

#include <psptypes.h>
#include <stddef.h>

/**
 * How to load files from gwd
 * This applies to most asset loaders in the engine
 * If you plan to load a file from gwd you have to set the "usesoffset" variable in the loader to 1;
 * Set the filepath to be "./EBOOT.PBP"
 * then supply a "fileoffset" using gaasGWDGetOffsetFromName
 * and, if needed, supply a "filesize" using gaasGWDGetSizeFromName
 * when using either of those functions you have to remember to type out the full path and put ./ at the start, otherwise it will not work
**/

/**
 * Generates a GWD index by getting the gwd file embedded as a psar file in the eboot
 * eboot - Path to eboot file ("./EBOOT.PBP")
**/
void gaasGWDGetWADOffsets(const char* eboot);

/**
 * Gets offset of specific file in gwd index
 * filename - complete path to file in gwd
 *      when seeking for a file remember to type out the full path and put ./ at the start, otherwise it will not work
 * 
 * returns: offset of file
**/
int gaasGWDGetOffsetFromName(const char* filename);

/**
 * Gets size of specific file in gwd index
 * filename - complete path to file in gwd
 *      when seeking for a file remember to type out the full path and put ./ at the start, otherwise it will not work
 * 
 * returns: size of file
**/
int gaasGWDGetSizeFromName(const char* filename);

/**
 * Gets position of PSAR in EBOOT aka where the gwd is located
 * 
 * returns: PSAR offset
**/
int gaasGWDGetPSAROffset();

/**
 * Gets position of where the index data starts in gwd file
 * 
 * returns: gwd index offset
**/
int gaasGWDGetGWDIndexOffset();

#endif