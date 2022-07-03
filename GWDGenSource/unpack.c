#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;

struct header { //size: 28 bytes
    uint8_t signature[4]; //signature/magic: GWD\0
    uint16_t version[2]; //version number: 0 - major version; 1 - minor version
    unsigned int filesOffset; //offset where all files start
    unsigned int ToCentries; //number of files
    unsigned int ToCtotalSize; //how much space the Table of Contents takes up in bytes
    unsigned int ToCblockSize; //size of a single ToC block in bytes
    unsigned int ToCoffset; //offset where the Table of Contents starts
};

struct ToCblock { //size: 16 bytes
    unsigned int entry; //ToC entry id
    unsigned int name_hash; //crc 32 hash of a file path
    unsigned int offset; //where the file is located in the gwd
    unsigned int filesize; //size of a file in bytes
};

struct header head;
struct ToCblock *fileindex;

int main(int argc, char *argv[]) {
    FILE* paste;
    FILE* fp = fopen(argv[1], "rb");

    fread(&head, sizeof(struct header), 1, fp);

	if(head.signature[0] == 'G' && head.signature[1] == 'W' && head.signature[2] == 'D' && head.signature[3] == '\0') {
		if(head.version[0] == 1 && head.version[1] == 0) {
			fileindex = malloc(head.ToCtotalSize);

			for(int i=0; i<head.ToCentries; i++) {
				fseek(fp, head.ToCoffset + head.ToCblockSize*i, SEEK_SET);
				fread(&fileindex[i], head.ToCblockSize, 1, fp);
			}
		}
	}

    void *buffer;
    char filepath[512];

    for(int i=0; i<head.ToCentries; i++) { 
        sprintf(filepath, "%s/%x", argv[2], fileindex[i].name_hash);
        paste = fopen(filepath, "wb"); //open files

        fseek(fp, fileindex[i].offset, SEEK_SET);

        buffer=malloc(fileindex[i].filesize);
        fread(buffer, fileindex[i].filesize, 1, fp);
        fwrite(buffer, fileindex[i].filesize, 1, paste);

        free(buffer);
        fclose(paste);
    }

    free(fileindex);
    fclose(fp);

    return 0;
}