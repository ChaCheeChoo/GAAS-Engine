#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "gwdloader.h"

unsigned int crc32(const char* s) {
    unsigned int crc = 0xffffffff;
    size_t i = 0;
    while (s[i] != '\0') {
        uint8_t byte = s[i];
        crc = crc ^ byte;
        for (uint8_t j = 8; j > 0; --j) {
            crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
        }

        i++;
    }
    return crc ^ 0xffffffff;
}

gaasGWD* gaasGWDLoad(const char* file) { 
	FILE* fp = fopen(file, "rb");
	gaasGWD* temp = malloc(sizeof(struct gaasGWD));

	//read header
	fread(&temp->header, sizeof(struct GWDheader), 1, fp);
	temp->PSAROffset=0;

	//check file integrity
	if(temp->header.signature[0] == 'G' && temp->header.signature[1] == 'W' && temp->header.signature[2] == 'D' && temp->header.signature[3] == '\0') {
		if(temp->header.version[0] == 1 && temp->header.version[1] == 0) {
			temp->ToC = malloc(temp->header.ToCtotalSize);//allocate table of contents array

			for(int i=0; i<temp->header.ToCentries; i++) {//read each ToC block
				fseek(fp, temp->header.ToCoffset + temp->header.ToCblockSize*i, SEEK_SET);
				fread(&temp->ToC[i], temp->header.ToCblockSize, 1, fp);
			}
		}
	}

	fclose(fp);
	return temp;
}

gaasGWD* gaasGWDLoadFromEBOOT(const char* eboot) {
	FILE* fp = fopen(eboot, "rb");
	gaasGWD* temp = malloc(sizeof(struct gaasGWD));
	int PsarOffset = 0; 

	struct PBPheader header;
	fread(&header, sizeof(struct PBPheader), 1, fp);

	if(header.signature[0] == '\0' && header.signature[1] == 'P' && header.signature[2] == 'B' && header.signature[3] == 'P') {
		PsarOffset = header.offset[7];
	}

	fseek(fp, PsarOffset, SEEK_SET); 

	fread(&temp->header, sizeof(struct GWDheader), 1, fp);
	temp->PSAROffset = PsarOffset;

	if(temp->header.signature[0] == 'G' && temp->header.signature[1] == 'W' && temp->header.signature[2] == 'D' && temp->header.signature[3] == '\0') {
		if(temp->header.version[0] == 1 && temp->header.version[1] == 0) {
			temp->ToC = malloc(temp->header.ToCtotalSize);

			for(int i=0; i<temp->header.ToCentries; i++) {
				fseek(fp, temp->PSAROffset + temp->header.ToCoffset + temp->header.ToCblockSize*i, SEEK_SET);
				fread(&temp->ToC[i], sizeof(struct GWDToCblock), 1, fp);
			}
		}
	}

	fclose(fp);
	return temp;
}

void gaasGWDTest(gaasGWD* source) {
	printf("\n***************Testing GWD***************\n");
	printf("Files Offset: %d\nNumber Of Files: %d\nToC Offset: %d\n", source->header.filesOffset, source->header.ToCentries, source->header.ToCoffset);
	printf("Table Of Contents:\n");
	for(int i=0; i<source->header.ToCentries; i++) {
		printf("ID: %d | size: %d | offset: %d | hash: 0x%x\n", source->ToC[i].entry, source->ToC[i].filesize, source->ToC[i].offset, source->ToC[i].name_hash);
	}
	printf("***************Testing GWD***************\n\n");
}

int gaasGWDGetOffsetFromName(gaasGWD* source, const char* filename) { 
	unsigned int fileHash = crc32(filename);
    for(int i=0; i<source->header.ToCentries; i++) {
    	if(fileHash == source->ToC[i].name_hash) { 
    		return source->ToC[i].offset+source->PSAROffset;
    	}
    }
	return -1;
}

int gaasGWDGetSizeFromName(gaasGWD* source, const char* filename) { 
	unsigned int fileHash = crc32(filename);
    for(int i=0; i<source->header.ToCentries; i++) {
    	if(fileHash == source->ToC[i].name_hash) { 
    		return source->ToC[i].filesize;
    	}
    }
	return -1;
}