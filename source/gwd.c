#include <string.h>
#include <stdio.h>
#include "gwd.h"

int PsarOffset = 0; 
int GWDIndexOffset = 0; 
 
struct PBPheader {
	uint8_t signature[4];
	uint16_t version[2];
	uint32_t offset[8];
};

struct gwd {
    char path[256]; 
    int fileoffset;
    int filesize;
};

struct gwd gwdindex[512];
int TotalNumOfFiles = 0;

//get the psar and gwdindex offsets 
void gaasGWDGetWADOffsets(const char* eboot) {
	char line[256];
	FILE* fp = fopen(eboot, "rb");
    int i=0;

	struct PBPheader header;
	fread(&header, sizeof(struct PBPheader), 1, fp);

	if(header.signature[0] == '\0' && header.signature[1] == 'P' && header.signature[2] == 'B' && header.signature[3] == 'P') {
		PsarOffset = header.offset[7];
	}

    fseek(fp, -512, SEEK_END);

	while(fgets(line,255,fp)) {
		line[255]=0;
		if(strchr(line,'\n')) strchr(line,'\n')[0]=0;
		if(strchr(line,'\r')) strchr(line,'\r')[0]=0;
		char *value=strchr(line,'=');

        if(strstr(line,"IndexOffset =")) { 
			char* Bitch;
			Bitch = strdup(value+2);
			sscanf(Bitch, "%d", &GWDIndexOffset);
		}
	}

	fseek(fp, GWDIndexOffset, SEEK_SET);

	while(fgets(line,255,fp)) {
        int Int1;
        int Int2;
        char Path[256];

        if(sscanf(line,"filewad %s %d %d",Path,&Int1,&Int2)==3) {
            strcpy(gwdindex[i].path, Path);
            gwdindex[i].fileoffset = Int1;
            gwdindex[i].filesize = Int2;
            i++;
        } 
	}
    TotalNumOfFiles = i;
	fclose(fp);
}

int gaasGWDGetOffsetFromName(const char* filename) { 
    for(int i=0; i<TotalNumOfFiles; i++) {
    	if(strstr(gwdindex[i].path, filename) != NULL) { 
    		return gwdindex[i].fileoffset+PsarOffset;
    	}
    }
	return -1;
}

int gaasGWDGetSizeFromName(const char* filename) { 
    for(int i=0; i<TotalNumOfFiles; i++) {
    	if(strstr(gwdindex[i].path, filename) != NULL) {
    		return gwdindex[i].filesize;
    	}
    }
	return -1;
}

int gaasGWDGetPSAROffset() {
	return PsarOffset;
}

int gaasGWDGetGWDIndexOffset() {
	return GWDIndexOffset;
}