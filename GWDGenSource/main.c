/**
 * This tool is used to generate G@AS Wad Files (gwd)
 * simply run the program in a command line and give it your asset directory
 * "GWDFileGen ./Data"
**/

#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

char paths[512][256];

int o=0;
int TotalFiles = 0;

struct wadindex {
    int id;
    char path[512];
    int offset;
    int filesize;
};

struct wadindex fileindex[512]; 
int IndexOffset = 0;

void printdir(char *dir, int depth) {
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;
    char shit[256];
    if((dp = opendir(dir)) == NULL) {
        strcpy(paths[o], dir);
        o++;
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
        printdir(shit, depth);
    }

    TotalFiles = o;
    closedir(dp);
}

int main(int argc, char *argv[]) { 
    FILE* fp;
    FILE* copy;
    char out[256];
    sprintf(out, "%s.gwd", argv[1]);
    fp = fopen(out, "wb");

    fputs("GWD\n", fp);
    printdir(argv[2], 0);

    size_t n, m; 
    unsigned char buff[8192];

    for(int i=0; i<TotalFiles; i++) { 
        copy = fopen(paths[i], "rb");
        fileindex[i].id = i;
        strcpy(fileindex[i].path, paths[i]);
        fileindex[i].offset = ftell(fp);
        fseek(copy, 0L, SEEK_END);
        fileindex[i].filesize = ftell(copy); 
        fseek(copy, 0L, SEEK_SET);
        do {
            n = fread(buff, 1, sizeof buff, copy); 
            if (n) m = fwrite(buff, 1, n, fp);
            else   m = 0;
        } while ((n > 0) && (n == m));
        fputs("\n\n\n", fp);
        fclose(copy);
    }

    fputs("\n\n", fp);
    IndexOffset = ftell(fp);

    for(int i=0; i<TotalFiles; i++) {
        char shitenburg[512];
        sprintf(shitenburg, "filewad %s %d %d\n", fileindex[i].path, fileindex[i].offset, fileindex[i].filesize);
        printf("filewad %d %s %d %d\n", fileindex[i].id, fileindex[i].path, fileindex[i].offset, fileindex[i].filesize);
        fputs(shitenburg, fp);
    }

    char fuckenburg[128];
    sprintf(fuckenburg, "IndexOffset = %d", IndexOffset);
    fputs(fuckenburg, fp);

    fclose(fp);
    return 0;
}
