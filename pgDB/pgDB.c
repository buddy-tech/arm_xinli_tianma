
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <dirent.h>
#include "pgDB.h"
#include "util/debug.h"
#include "sqlite3.h"
#include "mypath.h"


#define MDDB_PATTERN_TABLE  "pattern_config"
#define MDDB_GLOBAL_TABLE   "global_config"

static module_set_t module_set;
static module_self_set_t  module_self_set;

#define MAX_PATTERN_NUM_IN_MODULE 200
#define MAX_MODULE_NUM_IN_PG  200

#define PIC_DEFAULT_LOCK_SECOND			(100)

static int dbpattern_configLoad(sqlite3 *dbMdl,module_set_t *pModule_set);
static int dbglobal_configLoad(sqlite3 *dbMdl,module_set_t *pModule_set);

typedef struct tag_db_modle_init_info_s
{
    char dbName[128];
    int  bInit;
}db_modle_init_info_t;

static db_modle_init_info_t  db_modle_init_info;

int dbModuleInit(char *pName)
{
    //if(db_modle_init_info.bInit)
    {
        dbModuleClose();
        db_modle_init_info.bInit = 0;
    }
	
    if(dbModuleOpen(pName, &module_set) == 0)
    {
        db_modle_init_info.bInit = 1;
        strcpy(db_modle_init_info.dbName,pName);
        return 0;
    }
	
    return -1;
}

int isdbMoudlelInit()
{
    return db_modle_init_info.bInit;
}

static int _stringtoint(const char *pString,int s32Default)
{
    if(pString)
        return strtol(pString,0,0);
    else
        return s32Default;
}

static void delPattern(void *pPattern)
{
    free(pPattern);
}

static int searchPattern(const module_info_t *pMoudleInfo, const int *ptnId)
{
    if(!pMoudleInfo || !ptnId)
    {
        return 0;
    }
    if(pMoudleInfo->ptdindex == *ptnId)
    {
        return 1;
    }
    return 0;
}

int dbModuleOpen(char *dbName, module_set_t *pModule_set)
{
    sqlite3 *dbMdl = NULL;
    DBG("DBName %s", dbName);
	
	if (NULL == dbName)
	{
		printf("dbModuleOpen error: DBName is NULL!\n");
		return -1;
	}
	
    if (SQLITE_OK != sqlite3_open(dbName, &dbMdl))
	{
        DBG("Open database %s fail.\r\n", dbName);
		return -2;
	}
	
    memset(pModule_set,0,sizeof(module_set_t));
    pModule_set->max_conns = MAX_PATTERN_NUM_IN_MODULE;
    pModule_set->cur_conns = 0;
	
    loop_create(&pModule_set->loop_conns, pModule_set->max_conns, delPattern);
	
    dbpattern_configLoad(dbMdl, pModule_set);
	
    dbglobal_configLoad(dbMdl, pModule_set);
    if (NULL != dbMdl)
    {
        sqlite3_close(dbMdl);
        dbMdl = NULL;
    }
	
    return 0;
}

int dbModuleClose()
{
    loop_cleanup(&module_set.loop_conns);
	return 0;
}

// load pattern_config table data into module_set.
int dbpattern_configLoad(sqlite3 *dbMdl, module_set_t *pModule_set)
{
    char *errcode = 0;
    int i = 0;
	int index = 0;
	int ret = 0;
	int row = 0;
	int column = 0;
    char **result = NULL;
    char sqls[256] = "";
	
    sprintf(sqls,"select * from %s", MDDB_PATTERN_TABLE);
    ret = sqlite3_get_table(dbMdl, sqls, &result, &row, &column, &errcode);
    if(errcode)
    {
        printf("dbpattern_configLoad error: %s.\n", errcode);
        sqlite3_free(errcode);
        sqlite3_free_table(result);
        return -1;
    }
	
    if(row > 0)
    {
        for(i = 0; i < row; i ++)
        {
            module_info_t *pMoudleItemInfo = malloc(sizeof(module_info_t));
            memset(pMoudleItemInfo, 0, sizeof(module_info_t));
			
            index = (i+1)*column;

			// index.
            pMoudleItemInfo->ptdindex = _stringtoint(result[index++], 0);

			// name.
            if(result[index])
            {          
                strcpy(pMoudleItemInfo->ptdpatternname, result[index++]);
            }

			// image_type.
            if(result[index])
            {
                //bmp
                strcpy(pMoudleItemInfo->ptdpatterntype ,result[index++]);
            }

			// bit_nums.
            pMoudleItemInfo->ptdpatternbit = _stringtoint(result[index++], 0);

			// resolution.
            if(result[index])
            {
                strcpy(pMoudleItemInfo->displayX ,result[index++]);
            }

			// lock_time.
            pMoudleItemInfo->ptddisptime = _stringtoint(result[index++], PIC_DEFAULT_LOCK_SECOND);

			// freq.
            pMoudleItemInfo->timclockfreq = _stringtoint(result[index++],0);

			// pic_move.
            if(result[index])
            {
                strcpy(pMoudleItemInfo->movefile, result[index++]);
            }

			// edid.
            if(result[index])
            {            	
                strcpy(pMoudleItemInfo->edidfile, result[index++]);
            }

			// init_code.
            if(result[index])
            {
                strcpy(pMoudleItemInfo->initcodefile, result[index++]);
				//printf("init code file: %s.\n", pMoudleItemInfo->initcodefile);
            }

			// 3d.
            if(result[index])
            {
                strcpy(pMoudleItemInfo->i3dfile ,result[index++]);
            }

			// video
            if(result[index])
            {
                strcpy(pMoudleItemInfo->videofile ,result[index++]);
            }

			// vcom_config
            if(result[index])
            {
            	//printf("=== ptn index: %d. vcom file: %s. index = %d. len = %d.====\n", 
				//	pMoudleItemInfo->ptdindex, result[index], index, strlen(result[index]));
				if (strlen(result[index]) > 0)
				{
					strcpy(pMoudleItemInfo->vcomfile ,result[index]);
					//printf("get vcom file: %s.\n", pMoudleItemInfo->vcomfile);
				}

				index++;
            }

			// power_test.
            if(result[index])
            {
                strcpy(pMoudleItemInfo->powertestfile ,result[index++]);
            }

			// dynamic_pic.
            if(result[index])
            {
                strcpy(pMoudleItemInfo->dynamicpattern ,result[index++]);
            }

			// gamma_config.
            if(result[index])
            {
                strcpy(pMoudleItemInfo->signaltest ,result[index++]);
            }
			
            //p1
            if(result[index])
            {
                strcpy(pMoudleItemInfo->timing_mode ,result[index]);
            }
            index++;
			
			
            //p2
            index++;
			
            //p3
            index++;
			
            //p4
            index++;
			
            if(pModule_set->cur_conns < pModule_set->max_conns)
            {
                loop_push_to_tail(&pModule_set->loop_conns, pMoudleItemInfo);
                pModule_set->cur_conns ++;
            }
            else
            {
                free(pMoudleItemInfo);
                break;
            }
        }
    }
	
    sqlite3_free_table(result);
	
    return 0;
}

// load global_config table data into module_set.
int dbglobal_configLoad(sqlite3 *dbMdl, module_set_t *pModule_set)
{
    char *errcode = 0;
    int i = 0;
	int index = 0;
	int ret = 0;
	int row = 0;
	int column = 0;
    char **result = NULL;
    char  sqls[256] = "";
	
    sprintf(sqls,"select * from %s", MDDB_GLOBAL_TABLE);
    ret = sqlite3_get_table(dbMdl, sqls, &result, &row, &column, &errcode);
    if(errcode)
    {
        printf("dbglobal_configLoad error: %s.\n", errcode);
        sqlite3_free(errcode);
        sqlite3_free_table(result);
        return -1;
    }
	
    if( row > 0)
    {
        for(i = 0; i < row; i ++)
        {
            index = (i+1) * column;

			// power config file.
            if(result[index])
            {
                strcpy(pModule_set->powermainfile, result[index]);
            }
			index ++;

			// timing config file.
            if(result[index])
            {
            	printf("timing config file: %s.\n", result[index]);
                strcpy(pModule_set->timingmainfile, result[index]);
            }
			index ++;

			// ageing time.
            pModule_set->agingtime = _stringtoint(result[index], 0);
			index ++;
			
            // resered1
            if(result[index])
        	{
        		strcpy(pModule_set->str_ic_name, result[index]);
        	}
            index++;
			
            // resered2	// comm config
            if(result[index])
        	{
        		strcpy(pModule_set->str_comm_cfg_name, result[index]);
        	}
            index++;
			
            // resered3	// monitor config
            if(result[index])
        	{
        		strcpy(pModule_set->str_monitor_cfg_name, result[index]);
        	}
            index++;
			
            // resered4
            index++;
        }
    }
    sqlite3_free_table(result);
    return 0;
}

module_info_t *dbModuleGetPatternById(int ptnId)
{
    int searchPtnId = ptnId;
    module_info_t *pMoudleItemInfo = loop_search( &module_set.loop_conns, &searchPtnId, searchPattern );
    return pMoudleItemInfo;
}

static int search_vcom_pattern(const module_info_t *pMoudleInfo, const int *ptnId)
{
    if(!pMoudleInfo)
    {
    	printf("search_vcom_pattern: NULL.\n");
        return 0;
    }
	
    if(strlen(pMoudleInfo->vcomfile) > 0)
    {
    	//printf("search_vcom_pattern: ptn index = %d.\n", pMoudleInfo->ptdindex);
        return 1;
    }
	
    return 0;
}

module_info_t *db_get_vcom_module_info()
{
	module_info_t* p_vcom_module = NULL;

	p_vcom_module = loop_search( &module_set.loop_conns, NULL, search_vcom_pattern );
	
	return p_vcom_module;
}

static int search_gamma_pattern(const module_info_t *pMoudleInfo, const int *ptnId)
{
    if(!pMoudleInfo)
    {
    	printf("search_gamma_pattern: NULL.\n");
        return 0;
    }
	
    if(strlen(pMoudleInfo->signaltest) > 0)
    {
        return 1;
    }
	
    return 0;
}

module_info_t *db_get_gamma_module_info()
{
	module_info_t* p_gamma_module = NULL;

	p_gamma_module = loop_search( &module_set.loop_conns, NULL, search_gamma_pattern );
	
	return p_gamma_module;
}



int dbModuleTstPtnNum()
{
    return module_set.cur_conns;
}

module_info_t *dbModuleGetPatternByLoopId(int loopId)
{
    return loop_get_from_head(&module_set.loop_conns,loopId);
}

int db_module_get_power_cfg_file_name(char *pPwrName)
{
    if(!pPwrName || (strlen(module_set.powermainfile) == 0))
        return -1;
	
    strcpy(pPwrName, module_set.powermainfile);
    return 0;
}

int db_module_get_timing_cfg_file_name(char *pTimName)
{
    if(!pTimName || (strlen(module_set.timingmainfile)==0))
        return -1;
	
    strcpy(pTimName, module_set.timingmainfile);
    return 0;
}

int db_module_get_chip_name(char *p_ic_name)
{
    if(!p_ic_name || (strlen(module_set.str_ic_name)==0))
        return -1;
	
    strcpy(p_ic_name, module_set.str_ic_name);
	
    return 0;
}

int db_module_get_comm_cfg_name(char *p_comm_cfg_name)
{
    if( !p_comm_cfg_name || (strlen(module_set.str_comm_cfg_name) == 0) )
        return -1;
	
    strcpy(p_comm_cfg_name, module_set.str_comm_cfg_name);
	
    return 0;
}

int db_module_get_monitor_cfg_name(char *p_monitor_cfg_name)
{
    if( !p_monitor_cfg_name || (strlen(module_set.str_monitor_cfg_name) == 0) )
        return -1;
	
    strcpy(p_monitor_cfg_name, module_set.str_monitor_cfg_name);
	
    return 0;
}


#if 0
int db_module_get_aging_time(int *pAgingTime)
{
    *pAgingTime = module_set.agingtime;
    return 0;
}
#endif

int dbInitModuleSelf()
{
    DIR *p_dir = NULL;
    struct dirent *p_dirent = NULL;
	
    memset(&module_self_set, 0, sizeof(module_self_set));
    module_self_set.max_conns = MAX_MODULE_NUM_IN_PG;
    module_self_set.cur_conns = 0;
    loop_create(&module_self_set.loop_conns, module_self_set.max_conns, 0);
	
    if((p_dir = opendir(TST_PATH))==NULL)
    {
        printf("dbInitModuleSelf error: ---->can not open path: %s.\n", TST_PATH);
        return -1;
    }
	
    while( p_dirent=readdir(p_dir))
    {
        DBG("%s", p_dirent->d_name);
        if((strcmp(p_dirent->d_name, ".") == 0)
			|| (strcmp(p_dirent->d_name, "..") == 0))
        {
            continue;
        }
			
        module_self_t *pModule_Self = malloc(sizeof(module_self_t));
        memset(pModule_Self,0,sizeof(module_self_t));
        strcpy(pModule_Self->moduleselfName,p_dirent->d_name);
        pModule_Self->index = module_self_set.cur_conns;
		
        loop_push_to_tail(&module_self_set.loop_conns, pModule_Self);
        module_self_set.cur_conns++;
    }
	
    closedir(p_dir);
	
    return module_self_set.cur_conns;
}

static int searchModuleName(const module_self_t *pMoudleSelf, const char *pName)
{
    if(!pMoudleSelf || !pName)
    {
        return 0;
    }
	
    if(strcmp(pMoudleSelf->moduleselfName,pName)==0)
    {
        return 1;
    }
	
    return 0;
}


static int searchModuleIndex(const module_self_t *pMoudleSelf, const int *index)
{
    if(!pMoudleSelf || !index)
    {
        return 0;
    }
	
    if(pMoudleSelf->index == *index)
    {
        return 1;
    }
	
    return 0;
}

module_self_t *dbModuleSelfGetByName(char *pName)
{
    module_self_t *pMoudleSelfInfo = loop_search( &module_self_set.loop_conns, pName, searchModuleName );
    if(pMoudleSelfInfo)
    {
        module_set_t tmp_module_set;
        char moduleName[256];
        sprintf(moduleName,"%s/%s",TST_PATH,pName);
		
        if(dbModuleOpen(moduleName, &tmp_module_set) == 0)
        {
           strncpy(pMoudleSelfInfo->modulePowerName, tmp_module_set.powermainfile, 31);
           module_info_t *ptmp_model_info  = loop_get_from_head(&tmp_module_set.loop_conns,0);
           if(ptmp_model_info)
           {
               strncpy(pMoudleSelfInfo->patternFirstName, ptmp_model_info->ptdpatternname,31);
           }
		   
           pMoudleSelfInfo->totalPattern = tmp_module_set.cur_conns;
           loop_cleanup(&tmp_module_set.loop_conns);
           DBG("module pwr name %s %s", pMoudleSelfInfo->modulePowerName, pMoudleSelfInfo->patternFirstName);
        }
        else
        {
           DBG("open module failed");
        }
    }
	
    return pMoudleSelfInfo;
}

module_self_t *dbModuleSelfGetByIndex(int index)
{
    int actIndex = index;
    module_self_t *pMoudleSelfInfo = loop_search( &module_self_set.loop_conns, &actIndex, searchModuleIndex );
    if(pMoudleSelfInfo)
    {
        module_set_t tmp_module_set;

        char moduleName[256];
        sprintf(moduleName,"%s/%s",TST_PATH,pMoudleSelfInfo->moduleselfName);

        if(dbModuleOpen(moduleName,&tmp_module_set) == 0)
        {
           strncpy(pMoudleSelfInfo->modulePowerName,tmp_module_set.powermainfile, 31);
           module_info_t *ptmp_model_info  = loop_get_from_head(&tmp_module_set.loop_conns,0);
           if(ptmp_model_info)
           {
               strncpy(pMoudleSelfInfo->patternFirstName,ptmp_model_info->ptdpatternname, 31);
           }
		   
           pMoudleSelfInfo->totalPattern = tmp_module_set.cur_conns;
           loop_cleanup(&tmp_module_set.loop_conns);
        }
    }
	
    return pMoudleSelfInfo;
}
