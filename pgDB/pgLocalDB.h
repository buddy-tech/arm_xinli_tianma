#ifndef _PGLOCALDB_H
#define _PGLOCALDB_H

#include "loop/loop.h"

#define LOCAL_FILE_NAME_LEN 128

typedef struct tag_local_file_info_s
{
    char  fileName[LOCAL_FILE_NAME_LEN];
    int   fileTime;
    int   fileSize;
    int   fileStatus;  //normal,updating,delete.
}local_file_info_t;


typedef struct tag_local_file_info_set_s
{
    loop_list_t		loop_conns;
    unsigned short	cur_conns;
}local_file_info_set_t;

int localDBInit();
int localDBAdd(char *pName,int time,int size);
int localDBUpdate(char *pName,int time,int size);
int localDBDelete(char *pName);
local_file_info_t *localDBGetRec(char *pName);
int localDBDelAll();

#endif
