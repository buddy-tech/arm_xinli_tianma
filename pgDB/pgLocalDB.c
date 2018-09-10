#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "pgLocalDB.h"
#include "util/debug.h"
#include "util/util.h"
#include "sqlite3.h"

#define  LOCAL_DB_PATH        "local.db"
#define  FILE_INFO_TABLE      "FileInfoTable"
#define  MAX_FILES_NUM        1000

static sqlite3 *gDbLocal    = NULL;

static local_file_info_set_t local_file_set;

static int _stringtoint(const char *pString,int s32Default)
{
    if(pString)
        return strtol(pString,0,0);
    else
        return s32Default;
}

static void delRecord(void *plocalRec)
{
    free(plocalRec);
}

// load local db record to local_file_set list.
int localDBInit()
{
    char *errcode = 0;
    char sqls[256] = "";
    int  i = 0;
	int index = 0;
	int ret = 0;
	int row = 0;
	int column = 0;
    char **result = NULL;

    memset(&local_file_set, 0, sizeof(local_file_info_set_t));
    if (SQLITE_OK != sqlite3_open(LOCAL_DB_PATH, &gDbLocal))
    {
        DBG("Open database %s fail.\r\n", LOCAL_DB_PATH);
        return -1;
    }
	
    sprintf(sqls,"%s","CREATE TABLE IF NOT EXISTS FileInfoTable (name TEXT, time , size , status , F1 , F2 )");
    ret = sqlite3_exec(gDbLocal, sqls, NULL, NULL, &errcode);
    if(ret<0||errcode)
    {
        DBG("[error] :%s \n",errcode);
        sqlite3_free(errcode);
        return -2;
    }
	
    sprintf(sqls, "select * from %s", FILE_INFO_TABLE);
    ret = sqlite3_get_table(gDbLocal, sqls, &result, &row, &column, &errcode);
    if(ret<0||errcode)
    {
        DBG("[error] :%s \n",errcode);
        sqlite3_free(errcode);
        sqlite3_free_table(result);
        return -3;
    }
	
    loop_create(&local_file_set.loop_conns, MAX_FILES_NUM, delRecord);
    local_file_set.cur_conns = 0;
	
    if(row > 0)
    {
        for(i= 0; i < row; i ++)
        {
            index = (i + 1) * column;
			
            local_file_info_t *pLocalFileInfo = (local_file_info_t*)malloc(sizeof(local_file_info_t));
            memset(pLocalFileInfo, 0, sizeof(local_file_info_t));
			
            if(result[index])
            {
                strcpy(pLocalFileInfo->fileName ,result[index++]);
            }
			
            if(result[index])
            {
                pLocalFileInfo->fileTime = _stringtoint(result[index++],0);
            }
			
            if(result[index])
            {
                pLocalFileInfo->fileSize = _stringtoint(result[index++],0);
            }
			
            if(result[index])
            {
                pLocalFileInfo->fileStatus = _stringtoint(result[index++],0);
            }
			
            //f1
            index++;
			
            //f2
            index++;

            loop_push_to_tail(&local_file_set.loop_conns, pLocalFileInfo);
            local_file_set.cur_conns ++;
        }
    }
	
    sqlite3_free_table(result);
}

int localDBAdd(char *pName, int time, int size)
{
    int  row = 0, column = 0,ret = 0;
    char **result = NULL;
    char *errcode = 0;
    char  sqls[256] = "";

    sprintf(sqls, "SELECT * FROM '%s' where name='%s' ", FILE_INFO_TABLE, pName);
    sqlite3_get_table(gDbLocal, sqls, &result, &row, &column, &errcode);
    if(errcode)
    {
        fprintf(stderr,"[%s error] :%s  @ line:%d\n", __FUNCTION__, errcode, __LINE__);
        sqlite3_free(errcode);
        sqlite3_free_table(result);
        return 0;
    }

	// if the record is exist, update the record data.
    if(row > 0)
    {
        sprintf(sqls,"UPDATE %s SET time=%d,size=%d where name='%s'", FILE_INFO_TABLE, time, size, pName);
        ret = sqlite3_exec(gDbLocal, sqls, 0, 0, &errcode);
        if(ret && errcode)
        {
            DBG("[error] :%s with %s\n", errcode, sqls);
            sqlite3_free(errcode);
            return 0;
        }
    }
    else
    {
    	// add new record to db.
        sprintf(sqls, "INSERT INTO %s VALUES ('%s', %d, %d, %d, 0, 0 )", FILE_INFO_TABLE, pName, time, size, 0);
        ret = sqlite3_exec(gDbLocal, sqls, NULL, NULL, &errcode);
        if(ret < 0 || errcode)
        {
            DBG("[error] :%s with %s\n", errcode, sqls);
            sqlite3_free(errcode);
            return -1;
        }
		
        local_file_info_t *pLocalFileInfo = (local_file_info_t*)malloc(sizeof(local_file_info_t));
        memset(pLocalFileInfo, 0, sizeof(local_file_info_t));
        strcpy(pLocalFileInfo->fileName, pName);
        pLocalFileInfo->fileTime = time;
        pLocalFileInfo->fileSize = size;
        pLocalFileInfo->fileStatus = 0; //normal;
        loop_push_to_tail(&local_file_set.loop_conns, pLocalFileInfo);
        local_file_set.cur_conns++;
    }
	
    return 0;
}

int localDBUpdate(char *pName, int time, int size)
{
    char *errcode=0;
    char  sqls[256] = "";
    int  ret = 0;
	
    sprintf(sqls, "UPDATE %s SET time=%d,size=%d where name='%s'", FILE_INFO_TABLE, time, size, pName);
    ret = sqlite3_exec(gDbLocal, sqls, NULL, NULL, &errcode);
    if(ret < 0 || errcode)
    {
        DBG("[error] :%s \n", errcode);
        sqlite3_free(errcode);
        return -1;
    }

	//return 0;
}

static int searchParternName(const void *p, const void *q)
{
    local_file_info_t *pLocal = p;
	
    if(strcmp(pLocal->fileName, q)==0)
    {
        return 1;
    }
	
    return 0;
}

int localDBDelete(char *pName)
{
    loop_list_t *l = &local_file_set.loop_conns;
	
    if(loop_search(l, pName, searchParternName))
    {
        return 0;
    }
	
    return -1;
}

// delete all records, clear local_file_set list.
int localDBDelAll()
{
    char *errcode = NULL;
    int	ret = 0;
    char sql[128] = "";
	
    sprintf(sql, "DELETE FROM '%s'", FILE_INFO_TABLE);
    ret = sqlite3_exec(gDbLocal, sql, 0, 0, &errcode);
    if(ret && errcode)
    {
        fprintf(stderr,"[%s error] :%s  @ line:%d\n", __FUNCTION__, errcode, __LINE__);
        sqlite3_free(errcode);
        return -1;
    }
	
    loop_list_t *l = &local_file_set.loop_conns;
    loop_clear(l);
	
    return 0;
}

local_file_info_t *localDBGetRec(char *pName)
{
    local_file_info_t *pLocalFileInfo = NULL;
    loop_list_t *l = &local_file_set.loop_conns;
	
    if( (pLocalFileInfo = loop_search(l, pName, searchParternName)) != NULL)
    {
        return pLocalFileInfo;
    }
	
    return NULL;
}



