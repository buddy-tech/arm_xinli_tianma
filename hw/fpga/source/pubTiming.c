#include "pubtiming.h"
#include "ini/iniparser.h"

int read_timging_config(char *timName,timing_info_t *pTiming_info)
{
    int i;
    char timpath[256];
    char section[256];
    dictionary	*ini;
    sprintf(timpath,"%s/%s",TIMING_INI_FILE_PATH,timName);
    ini = iniparser_load(timpath);
    if (ini==NULL)
    {
        printf("cannot parse tim.ini\n");
        return -1;
    }
    else
    {
        printf("read the timName %s\n",timpath);
    }

    sprintf(section,"timing:timsignalmode");
    pTiming_info->timsignalmode= iniparser_getint(ini, section, 1);
    sprintf(section,"timing:Signal");
    pTiming_info->Signal= iniparser_getint(ini, section, 0);
    sprintf(section,"timing:timbit");
    pTiming_info->timbit= iniparser_getint(ini, section, 0);
    sprintf(section,"timing:timvesajeda");
    pTiming_info->timvesajeda = iniparser_getint(ini, section, 0);
    sprintf(section,"timing:timvsyncpri");
    pTiming_info->timvsyncpri = iniparser_getint(ini, section, 0);
    sprintf(section,"timing:timhsyncpri");
    pTiming_info->timhsyncpri = iniparser_getint(ini, section, 0);
    sprintf(section,"timing:timde");
    pTiming_info->timde = iniparser_getint(ini, section, 0);
    sprintf(section,"timing:timvtotaltime");
    pTiming_info->timvtotaltime =  iniparser_getint(ini, section, 0);
    sprintf(section,"timing:timhactivetime");
    pTiming_info->timhactivetime = iniparser_getint(ini, section, 0);
    sprintf(section,"timing:timhtotaltime");
    pTiming_info->timhtotaltime = iniparser_getint(ini, section, 0);
    sprintf(section,"timing:timhbackporch");
    pTiming_info->timhbackporch = iniparser_getint(ini, section, 0);
    sprintf(section,"timing:timhfrontporch");
    pTiming_info->timhfrontporch = iniparser_getint(ini, section, 0);
    sprintf(section,"timing:timhsyncpulsewidth");
    pTiming_info->timhsyncpulsewidth = iniparser_getint(ini, section, 0);
    sprintf(section,"timing:timvactivetime");
    pTiming_info->timvactivetime = iniparser_getint(ini, section, 0);
    sprintf(section,"timing:timvbackporch");
    pTiming_info->timvbackporch = iniparser_getint(ini, section, 0);
    sprintf(section,"timing:timvsyncpulsewidth");
    pTiming_info->timvsyncpulsewidth = iniparser_getint(ini, section, 0);
    sprintf(section,"timing:timvfrontporch");
    pTiming_info->timvfrontporch = iniparser_getint(ini, section, 0);
    sprintf(section,"timing:timclockfreq");
    pTiming_info->timclockfreq = iniparser_getint(ini, section, 0);
    sprintf(section,"timing:timMainFreq");
    pTiming_info->timMainFreq = iniparser_getint(ini, section, 0);


    sprintf(section,"timing:Signal");
    pTiming_info->Signal = iniparser_getint(ini, section, 0);

    sprintf(section,"timing:timlink");
    char *plink = iniparser_getstring(ini, section, "12345678");
    int  linkCount = strlen(plink);
    printf("linkcount is %d\n",linkCount);
    for(i=0;i<linkCount;i++)
    {
        printf("plink:%d --%d\n",i,plink[i]-'0');
        pTiming_info->timlink[i] = plink[i]-'0';
    }

    sprintf(section,"timing:timsignaltype");
    pTiming_info->timsignaltype = iniparser_getint(ini, section, 0);

    sprintf(section,"timing:timmipilanenum");
    pTiming_info->timmipilanenum = iniparser_getint(ini, section, 1);

    sprintf(section,"timing:timmipisyncmode");
    pTiming_info->timmipisyncmode = iniparser_getint(ini, section, 0);

    sprintf(section,"timing:timmipicommandenable");
    pTiming_info->timmipicommandenable = iniparser_getint(ini, section, 0);

    sprintf(section,"timing:timmipiprd");
    pTiming_info->timmipiprd = iniparser_getint(ini, section, 0);

    sprintf(section,"timing:timmipivsdelay");
    pTiming_info->timmipivsdelay = iniparser_getint(ini, section, 0);

    sprintf(section,"timing:mdlmipidsiclock");
    pTiming_info->mdlmipidsiclock = iniparser_getint(ini, section, 0);
    pTiming_info->mdlmipidsiclock = pTiming_info->mdlmipidsiclock *1000;

    sprintf(section,"timing:timmipii2cdrivertype");
    pTiming_info->timmipii2cdrivertype = iniparser_getint(ini, section, 0);

    sprintf(section,"timing:timmipilinenumber");
    pTiming_info->timmipilinenumber = iniparser_getint(ini, section, 0);

    sprintf(section,"timing:timmipirefreshrate");
    pTiming_info->timmipirefreshrate = iniparser_getint(ini, section, 0);

    sprintf(section,"timing:timinitcodename");
    char *pInitCodeName = iniparser_getstring(ini, section, "");
    if(pInitCodeName)
    {
        strcpy(pTiming_info->timinitcodename,pInitCodeName);
    }

    sprintf(section,"timing:timMipiBurnCodeName");
    char *pMtpCodeName = iniparser_getstring(ini, section, "");
    if(pInitCodeName)
    {
        strcpy(pTiming_info->timMipiBurnCodeName,pMtpCodeName);
    }

    sprintf(section,"timing:timdpinterface");
    pTiming_info->timdpinterface = iniparser_getint(ini, section, 0);

    sprintf(section,"timing:timdpauxrate");
    pTiming_info->timdpauxrate = iniparser_getint(ini, section, 0);

    sprintf(section,"timing:timdphpddelay");
    pTiming_info->timdphpddelay = iniparser_getint(ini, section, 0);
	
	sprintf(section,"timing:tim_clock_pri");
	pTiming_info->tim_clk_pri = iniparser_getint(ini, section, 0);

    iniparser_freedict(ini);

	return 0;
}
