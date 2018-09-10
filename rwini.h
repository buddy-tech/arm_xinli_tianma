#ifndef RWINI_H
#define RWINI_H

#define  SYNC_INI_FILE_PATH   "./sync.ini"
#define  MODULE_FILE_INI_PATH "./module.ini"

typedef struct tag_sync_info_s
{
    int syncProcess;
    int timeStamp;
}sync_info_t;

void read_sync_config(sync_info_t *pSync_info);
void write_sync_config(sync_info_t *pSync_info);
int  read_cur_module_name(char *pModuleName);
void write_cur_module_name();
void set_cur_module_name(char *pModuleName);

#endif // RWINI_H
