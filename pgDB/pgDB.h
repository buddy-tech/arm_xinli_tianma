
#ifndef _PG_DB_H
#define _PG_DB_H

#include "loop/loop.h"

#define MAX_FILE_NAME_LEN 64

typedef struct tag_module_info_s
{
    int  ptdindex;
    char ptdpatternname[MAX_FILE_NAME_LEN];
    char ptdpatterntype[MAX_FILE_NAME_LEN/2];
    int  ptdpatternbit;
    char displayX[MAX_FILE_NAME_LEN/2];		// resolution.
    int  ptddisptime;						// lock time(second).
    int  timclockfreq;						// frequency.
    char movefile[MAX_FILE_NAME_LEN];
    char edidfile[MAX_FILE_NAME_LEN];
    char initcodefile[MAX_FILE_NAME_LEN];
    char i3dfile[MAX_FILE_NAME_LEN];
    char videofile[MAX_FILE_NAME_LEN];
    char vcomfile[MAX_FILE_NAME_LEN];		// vcom config.
    char powertestfile[MAX_FILE_NAME_LEN];
    char dynamicpattern[MAX_FILE_NAME_LEN];
    char signaltest[MAX_FILE_NAME_LEN];		// gamma config.
    // res1;
    char timing_mode[MAX_FILE_NAME_LEN];	// timing mode.
    // res2;
    // res3;
    // res4;
}module_info_t;

typedef struct tag_module_set_s
{
    char            powermainfile[MAX_FILE_NAME_LEN];
    char            timingmainfile[MAX_FILE_NAME_LEN];
    int             agingtime;
	char			str_ic_name[MAX_FILE_NAME_LEN];	
	// res2;
	// res3;
	// res4;
	char			str_comm_cfg_name[MAX_FILE_NAME_LEN];
	char			str_monitor_cfg_name[MAX_FILE_NAME_LEN];
	
    loop_list_t		loop_conns;
    unsigned short	max_conns;
    unsigned short	cur_conns;
}module_set_t;


typedef struct tag_module_self_s
{
    int  index;
    int  totalPattern;
    char moduleselfName[MAX_FILE_NAME_LEN/2];
    char modulePowerName[MAX_FILE_NAME_LEN/2];
    char patternFirstName[MAX_FILE_NAME_LEN/2];
}module_self_t;

typedef struct tag_module_self_set_s
{
    loop_list_t			loop_conns;
    unsigned short	max_conns;
    unsigned short	cur_conns;
}module_self_set_t;

int dbModuleInit(char *pName);
int isdbMoudlelInit();
int dbModuleOpen(char *dbName, module_set_t *pModule_set);
int dbModuleClose();

int dbModuleTstPtnNum();
module_info_t *dbModuleGetPatternByLoopId(int loopId);
module_info_t *dbModuleGetPatternById(int ptnId);

int dbModuleGetPwr(char *pPwrName);
int dbModuleGetTim(char *pTimName);
int dbModuleGetAgingTime(int *pAgingTime);


int dbInitModuleSelf();
module_self_t *dbModuleSelfGetByName(char *pName);
module_self_t *dbModuleSelfGetByIndex(int index);

#endif

