#include "rwini.h"
#include "ini/iniparser.h"
#include "util/debug.h"

void read_sync_config(sync_info_t *pSync_info)
{
    char section[256];
    dictionary	*ini;
    ini = iniparser_load(SYNC_INI_FILE_PATH);
    if (ini==NULL)
    {
        DBG("cannot parse sync.ini\n");
        pSync_info->syncProcess = 0;
        pSync_info->timeStamp = 0;
        return;
    }
	
    sprintf(section,"sync:syncprocess");
    pSync_info->syncProcess = iniparser_getint(ini, section, 0);
    sprintf(section,"sync:timestamp");
    pSync_info->timeStamp = iniparser_getint(ini, section, 0);
    iniparser_freedict(ini);
    //DBG("SYNC %d timeStamp %d\n",pSync_info->syncProcess,pSync_info->timeStamp);
}

void write_sync_config(sync_info_t *pSync_info)
{
    int ret;
    char section[256];
    char value[256];
    dictionary	*ini;
    ini = iniparser_load(SYNC_INI_FILE_PATH);
    if(ini)
    {
        sprintf(section,"sync:syncprocess");
        sprintf(value,"%d",pSync_info->syncProcess);
        ret = iniparser_set(ini,section,value);

        sprintf(section,"sync:timestamp");
        sprintf(value,"%d",pSync_info->timeStamp);
        iniparser_set(ini,section,value);

        iniparser_dump_ini(ini,SYNC_INI_FILE_PATH);
        iniparser_freedict(ini);
    }
    else
    {
        FILE	*inifile;
        inifile = fopen(SYNC_INI_FILE_PATH, "w");
        if(!inifile)
        {
            return;
        }
        else
        {
            fprintf(inifile,
            "[sync]\n"
            "syncprocess=%d\n"
            "timestamp=%d\n",
            pSync_info->syncProcess,
            pSync_info->timeStamp
            );
            fclose(inifile);
        }
    }
}

static  char curModuleName[256];

int read_cur_module_name(char *pCurModuleName)
{
    char section[256] = "";
    char *pModuleName = NULL;
    dictionary	*ini = NULL;
	
    ini = iniparser_load(MODULE_FILE_INI_PATH);
    if (ini==NULL)
    {
        printf("read_cur_module_name error: cannot parse module.ini!\n");
        return -1;
    }
	
    sprintf(section,"curModule:name");
    pModuleName = iniparser_getstring(ini, section, "");
    if(!pModuleName)
    {
    	printf("read_cur_module_name error: pModuleName is NULL!\n");
        iniparser_freedict(ini);
        return -2;
    }
	
    strcpy(pCurModuleName, pModuleName);
    iniparser_freedict(ini);
	
    return 0;
}

void write_cur_module_name()
{
    int  ret;
    char section[256];
    char value[256];
    dictionary	*ini;
    ini = iniparser_load(MODULE_FILE_INI_PATH);
    if(ini)
    {
        sprintf(section,"curModule:name");
        sprintf(value,"%s",curModuleName);
        ret = iniparser_set(ini,section,value);

        iniparser_dump_ini(ini,MODULE_FILE_INI_PATH);
        iniparser_freedict(ini);
    }
    else
    {
        FILE	*inifile;
        inifile = fopen(MODULE_FILE_INI_PATH, "w");
        if(!inifile)
        {
            return;
        }
        else
        {
            fprintf(inifile,"[curModule]\nname=%s\n",curModuleName);
            fclose(inifile);
        }
    }
}

void set_cur_module_name(char *pModuleName)
{
    memset(curModuleName,0,sizeof(curModuleName));
    if(pModuleName)
    {
        strcpy(curModuleName,pModuleName);
    }
}
