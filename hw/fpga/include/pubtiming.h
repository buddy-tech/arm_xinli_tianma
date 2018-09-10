#ifndef PUBTIMING_H
#define PUBTIMING_H

#define TIMING_INI_FILE_PATH  "cfg/timing/"

enum {
	E_SIGNAL_TYPE_LVDS = 0,
	E_SIGNAL_TYPE_EDP,
	E_SIGNAL_TYPE_MIPI,
	E_SIGNAL_TYPE_V_BY_ONE,
	E_SIGNAL_TYPE_TTL,
	E_MAX_SIGNAL_TYPE_NUMS
};

typedef struct tag_timing_info_s
{
    int  timsignalmode; //1,2,4,8 linkcount
    int  Signal;
    int  timbit;      //6,8,10,12
    int  timvesajeda; //Vesa,Jeda
    int  timvsyncpri; //0:+ 1:-
    int  timhsyncpri;
	int  tim_clk_pri; // 0: +; 1: -
    int  timde;
    int  timvtotaltime;
    int  timhactivetime;
    int  timhtotaltime;
    int  timhbackporch;
    int  timhfrontporch;
    int  timhsyncpulsewidth;
    int  timvactivetime;
    int  timvbackporch;
    int  timvsyncpulsewidth;
    int  timvfrontporch;
    int  timclockfreq;
    int  timMainFreq;
    char timlink[8];
    int  timsignaltype; //0:lvds 1:edp 2:mipi 3:vbyone
    //mipi
    int  timmipilanenum;
    int  timmipisyncmode;  // 0:pulse mode 1: event mode
    int  timmipicommandenable;
    int  timmipiprd;
    int  timmipivsdelay;
    int  mdlmipidsiclock;
    int  timmipii2cdrivertype;
    int  timmipilinenumber;
    int  timmipirefreshrate;
    char timinitcodename[64];
    char timMipiBurnCodeName[64];
    //dp
    int  timdpinterface;
    int  timdpauxrate;
    int  timdphpddelay;
}timing_info_t;

int read_timging_config(char *timName,timing_info_t *pTiming_info);

#endif // PUBTIMING_H
