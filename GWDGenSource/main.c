/**
 * This tool is used to generate G@AS Wad Files (gwd)
 * simply run the program in a command line and give it your asset directory
 * "GWDFileGen data ./Data"
**/

#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;

int o=0;
int e=0;

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

struct Path {
    char path[512];
};

struct header head;
struct ToCblock *fileindex;
struct Path *filepath;

unsigned int crc32(const char* s) {
    unsigned int crc = 0xffffffff;
    unsigned int i = 0;
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

int countfiles(char *dir, int depth) {
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;
    char shit[256];
    if((dp = opendir(dir)) == NULL) {
        o++;
        return -1;
    }
    
    while((entry = readdir(dp)) != NULL) {
        lstat(entry->d_name,&statbuf);
        if(strcmp(".",entry->d_name) == 0 || strcmp("..",entry->d_name) == 0) {
            continue;
        }
        strcpy(shit, dir);
        strcat(shit, "/");
        strcat(shit, entry->d_name);
        countfiles(shit, depth);
    }

    closedir(dp);
    return o;
}

void genindex(char *dir, int depth) {
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;
    char shit[256];
    if((dp = opendir(dir)) == NULL) {
        fileindex[e].name_hash = crc32(dir);
        strcpy(filepath[e].path, dir);
        e++;
        return;
    }
    while((entry = readdir(dp)) != NULL) {
        lstat(entry->d_name,&statbuf);
        if(strcmp(".",entry->d_name) == 0 || strcmp("..",entry->d_name) == 0) {
            continue;
        }
        strcpy(shit, dir);
        strcat(shit, "/");
        strcat(shit, entry->d_name);
        genindex(shit, depth);
    }
    closedir(dp);
}

int main(int argc, char *argv[]) { 
    FILE* fp;
    FILE* copy;
    char out[256];
    sprintf(out, "%s.gwd", argv[1]);

    //setup header
    head.signature[0] = 'G';
    head.signature[1] = 'W';
    head.signature[2] = 'D';
    head.signature[3] = '\0';

    head.version[0] = 1;
    head.version[1] = 0;

    head.ToCblockSize = sizeof(struct ToCblock);

    head.ToCentries = countfiles(argv[2], 0); //counts number of files to write
    head.ToCtotalSize = sizeof(struct ToCblock)*head.ToCentries;
    fileindex = (struct ToCblock*)malloc(head.ToCtotalSize); //allocates space for file index
    filepath = (struct Path*)malloc(sizeof(struct Path)*head.ToCentries); //allocates space for paths

    //generate file index
    genindex(argv[2], 0);

    //start file operation
    fp = fopen(out, "wb");

    fseek(fp, sizeof(struct header)+head.ToCtotalSize, SEEK_SET); //skips writing header and ToC until they're ready

    //prepare for file copy
    void* buffer;

    head.filesOffset = ftell(fp);

    for(int i=0; i<head.ToCentries; i++) { 
        copy = fopen(filepath[i].path, "rb"); //open files

        fileindex[i].entry = i;
        fileindex[i].offset = ftell(fp); //get file offset

        //get file size
        fseek(copy, 0L, SEEK_END);
        fileindex[i].filesize = ftell(copy); 
        fseek(copy, 0L, SEEK_SET);

        fseek(fp, fileindex[i].offset, SEEK_SET);

        buffer=malloc(fileindex[i].filesize);
        fread(buffer, fileindex[i].filesize, 1, copy);
        fwrite(buffer, fileindex[i].filesize, 1, fp);

        free(buffer);
        fclose(copy);
    }

    fseek(fp, sizeof(struct header), SEEK_SET); //seek to after header to write ToC

    //write file index
    head.ToCoffset = ftell(fp);
    for(int i=0; i<head.ToCentries; i++) {
        fwrite(&fileindex[i], head.ToCblockSize, 1, fp);
        printf("file %d %s 0x%x %d %d\n", i, filepath[i].path, fileindex[i].name_hash, fileindex[i].offset, fileindex[i].filesize);
    }

    //write header
    fseek(fp, 0L, SEEK_SET); //seek to start of file to write header
    fwrite(&head, sizeof(struct header), 1, fp);

    free(fileindex);
    free(filepath);
    fclose(fp);
    return 0;
}
