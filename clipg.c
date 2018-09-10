#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "client.h"
#include "util/debug.h"
#include "util/util.h"
#include "rwini.h"
#include "pgDB/pgDB.h"
#include "pgDB/pgLocalDB.h"
#include "fpgaFunc.h"
#include "comStruct.h"
#include "pubtiming.h"
#include "recvcmd.h"
#include "module_cfg/comm_config.h"
#include "module_cfg/monitor_config.h"
#include "openshort/openshort.h"
#include "hi_unf_gpio.h"
#include "clipg.h"
#include "common.h"
#include "spi_sp6.h"
#include "pubPwrTask.h"
#include "fpgaCtrlAccess.h"

static int bSetTime = 0;
static power_cfg_info_t s_power_info = { 0 };
static timing_info_t s_timing_info = { 0 };
static vcom_info_t s_vcom_info = { 0 };
static comm_cfg_info_t s_comm_cfg_info = { 0 };
static monitor_cfg_info_t s_monitor_cfg_info = { 0 };
static int s_vcom_ptn_index = -1;
static gamma_cfg_t s_gamma_cfg = { 0 };

power_cfg_info_t* get_current_power_cfg()
{
	return &s_power_info;
}

monitor_cfg_info_t* get_current_monitor_cfg()
{
	return &s_monitor_cfg_info;
}

int get_module_res(int* p_width, int* p_height)
{
	if ( (p_width == NULL) || (p_height == NULL) )
	{
		printf("get_module_res: Invalid param!\n");
		return -1;
	}

	*p_width = s_timing_info.timhactivetime;
	*p_height = s_timing_info.timvactivetime;
	printf("get_module_res: %d x %d.\n", *p_width, *p_height);
	
	return 0;
}

void client_pg_setTime(int stampTime)
{
    DBG("set time %d",stampTime);
#if 1
    if(!stampTime)
        return;
    if(bSetTime == 0)
    {
        struct timeval tv_set;

        tv_set.tv_sec  = stampTime;
        tv_set.tv_usec = 0;
        if (settimeofday(&tv_set,NULL)<0) {
            perror("settimeofday");
        }
        bSetTime = 1;
    }
#endif
    //client_rebackHeart();
}

void client_pg_firmware(void *param)
{
    recv_file_info_set_t *recv_file_info_set = param;
    char dst_file[256];
    recv_file_info_t *recv_file_info = recv_file_info_set->fn_get_cur_file_info(recv_file_info_set);
    char *pstrNewFileName = recv_file_info->rfiName;
    sprintf(dst_file,"/home/updata/%s",pstrNewFileName);
    FILE  *local_file = fopen(dst_file, "wb+");
    fwrite(recv_file_info->pFileData, recv_file_info->rfiSize,1,local_file);
    fclose(local_file);
}

void client_pg_syncFile(void *param) //param = recv_file_info_set_t
{
    recv_file_info_set_t *recv_file_info_set = param;
    char dst_file[256] = "";
    char dst_path[256] = "";	
    recv_file_info_t *recv_file_info = NULL;
    char *pstrNewFileName = NULL;

	recv_file_info = recv_file_info_set->fn_get_cur_file_info(recv_file_info_set);
	if (recv_file_info == NULL)
	{
		printf("client_pg_syncFile error: recv_file_info = NULL.\n");
		
		if(recv_file_info_set->param)
        {
            free(recv_file_info_set->param);
			recv_file_info_set->param = NULL;
        }

		recv_file_list_del(recv_file_info_set);
        free(recv_file_info_set);
		recv_file_info_set = NULL;
        
        //client_syncFinish();
	}
	
	pstrNewFileName = recv_file_info->rfiName;
    sprintf(dst_file, "./%s", pstrNewFileName);
	
    int  pathBytes = strrchr(dst_file,'/') - dst_file;
    memset(dst_path,0,sizeof(dst_path));
    memcpy(dst_path,dst_file,pathBytes);
	
    int result = mkdir_recursive(dst_path);
    int  local_file = 0;
    local_file = open(dst_file, O_CREAT|O_TRUNC|O_WRONLY);
    write(local_file, recv_file_info->pFileData, recv_file_info->rfiSize);
    close(local_file);
	
    system("sync");
    if(strstr(dst_file, "imageOrigin"))  //save to bmp to ptn,with same name
    {
        saveBmpToPtn(dst_file);
    }
    DBG("%s  result %d %s",dst_path,result,dst_file);

    local_file_info_t *pLocalFile = 0;
    if(pLocalFile = localDBGetRec(recv_file_info->rfiName))
    {
        pLocalFile->fileSize = recv_file_info->rfiSize;
        pLocalFile->fileTime = recv_file_info->rfiTime;
        localDBUpdate(recv_file_info->rfiName, recv_file_info->rfiTime, recv_file_info->rfiSize);
    }
    else
    {
        localDBAdd(recv_file_info->rfiName, recv_file_info->rfiTime, recv_file_info->rfiSize);
    }

    sync_info_t sync_info;
    sync_info.syncProcess = (recv_file_info_set->curNo + 1) * 100 / recv_file_info_set->totalFilesNums;
    sync_info.timeStamp = cur_time();
    //DBG("SYNC %d timeStamp %d\n",sync_info.syncProcess,sync_info.timeStamp);
    write_sync_config(&sync_info); //写./sync.ini

    client_syncStatu();

    recv_file_info_set->fn_add_file_index(recv_file_info_set);

    if(client_getFileFromSet(recv_file_info_set) != 0)
    {
    	printf("client_pg_syncFile: sync end!\n");
        if(recv_file_info_set->param)
        {
            free(recv_file_info_set->param);
			recv_file_info_set->param = NULL;
        }

		recv_file_list_del(recv_file_info_set);
        free(recv_file_info_set);
		recv_file_info_set = NULL;
        
        client_syncFinish();

		// box state reset.
		printf("=========================   SYNC END 3===================\n");
		pg_do_box_home_proc();
		
    }
}

void client_pg_downFile(void *param) //param = recv_file_info_set_t
{
    DBG("down file success!");
    recv_file_info_set_t *recv_file_info_set = param;
    recv_file_info_t *recv_file_info = recv_file_info_set->fn_get_cur_file_info(recv_file_info_set);
    dwn_file_info_t  *dwn_file_info  = (dwn_file_info_t*)recv_file_info_set->param;
    if(dwn_file_info)
    {
        char dst_file[256];
        int  local_file;
        strcpy(dst_file,dwn_file_info->dstFileName);
        local_file = open(dst_file, O_CREAT|O_TRUNC|O_WRONLY);
        write(local_file, recv_file_info->pFileData, recv_file_info->actRecvSize);
        close(local_file);
    }
}

int load_current_vcom_info(vcom_info_t* p_vcom_info)
{
	if (p_vcom_info == NULL)
		return -1;
	
	module_info_t *p_vcom_Module_info =  db_get_vcom_module_info();
	if (p_vcom_Module_info)
	{
		printf("VCOM cfg: %s. ptdindex = %d.\n", p_vcom_Module_info->vcomfile, p_vcom_Module_info->ptdindex);
		read_vcom_config(p_vcom_info, p_vcom_Module_info->vcomfile);
		printf("init vcom: %d, min vcom: %d, max vcom: %d, min_valid_flick: %f,"
				"max_valid_flick: %f, ok_flick: %f.\n", 
				p_vcom_Module_info->vcomfile, p_vcom_info->initVcom, p_vcom_info->minvalue, 
				p_vcom_info->maxvalue, p_vcom_info->f_min_valid_flick, p_vcom_info->f_max_valid_flick,
				p_vcom_info->f_ok_flick);

		s_vcom_ptn_index = p_vcom_Module_info->ptdindex;
	}
	else
	{
		printf("*** find vcom pattern failed! ***\n");
		s_vcom_ptn_index = -1;
	}
	
	return 0;
}

int get_current_vcom_ptn_index()
{
	return s_vcom_ptn_index;
}

vcom_info_t* get_current_vcom_info()
{
	return &s_vcom_info;
}

int load_current_gamma_info(gamma_cfg_t* p_gamma_cfg)
{
	if (p_gamma_cfg == NULL)
	{
		printf("load_current_gamma_info: Null gamma config.\n");
		return -1;
	}
	
	module_info_t *p_gamma_Module_info =  db_get_gamma_module_info();
	if (p_gamma_Module_info)
	{
		printf("Gamma cfg: %s.\n", p_gamma_Module_info->signaltest);		
		load_gamma_config(p_gamma_cfg, p_gamma_Module_info->signaltest);
	}
	else
	{
		printf("*** find gamma pattern failed!  %s. ***\n", p_gamma_cfg);
	}
	
	return 0;
}

gamma_cfg_t* get_current_gamma_cfg()
{
	return &s_gamma_cfg;
}

void show_signal_type_info(int signal_type)
{
	switch(signal_type)
	{
		case E_SIGNAL_TYPE_LVDS:
			printf("Signal Type: LVDS.\n");
			break;

		case E_SIGNAL_TYPE_EDP:
			printf("Signal Type: eDP.\n");
			break;

		case E_SIGNAL_TYPE_MIPI:
			printf("Signal Type: MIPI.\n");
			break;

		case E_SIGNAL_TYPE_V_BY_ONE:
			printf("Signal Type: V-By-One.\n");
			break;

		case E_SIGNAL_TYPE_TTL:
			printf("Signal Type: TTL.\n");
			break;

		default:
			printf("Signal Type: Unknow, %d.\n", signal_type);
			break;
	}
}

static void initFpgaOutputPictureTiming(fpgaSeqInfo_t* pFpgaSeq)
{
	pFpgaSeq->T0 = (s_timing_info.timhbackporch+s_timing_info.timhsyncpulsewidth+\
                  s_timing_info.timhactivetime)/s_timing_info.timsignalmode;
	pFpgaSeq->T1 = s_timing_info.timhsyncpulsewidth/s_timing_info.timsignalmode;
	pFpgaSeq->T2 = (s_timing_info.timhsyncpulsewidth+s_timing_info.timhbackporch)/s_timing_info.timsignalmode;
	pFpgaSeq->T3 = (s_timing_info.timhbackporch+s_timing_info.timhsyncpulsewidth+\
                  s_timing_info.timhfrontporch+s_timing_info.timhactivetime)/s_timing_info.timsignalmode;
	pFpgaSeq->T4 = s_timing_info.timvbackporch+s_timing_info.timvsyncpulsewidth+\
                    s_timing_info.timvactivetime;
	pFpgaSeq->T5 = s_timing_info.timvsyncpulsewidth;
	pFpgaSeq->T6 = s_timing_info.timvbackporch+s_timing_info.timvsyncpulsewidth;
	pFpgaSeq->T7 = s_timing_info.timvbackporch+s_timing_info.timvsyncpulsewidth+\
                   s_timing_info.timvfrontporch+s_timing_info.timvactivetime;
}

int load_cur_module()
{
    int isDir = 0;
	int ret = 0;
    unsigned int u32QueueId = 0;
    char modelName[256] = "";
    char reallyModelName[256] = "";
    char pwrCfgName[256] = "";
    char timCfgName[256] = "";
	char chip_name[64] = "";
	char str_comm_cfg_name[256] = "";
	char str_monitor_cfg_name[256] = "";

	printf("********************load_cur_module.********************\n");

    ret = read_cur_module_name(modelName);
    if(ret!=0)
    {
    	printf("load_cur_module: read_cur_module_name failed!\n");
        return ret;
    }

    sprintf(reallyModelName, "%s/%s", TST_PATH, modelName);
    DBG("PG sync with model %s %s\r\n", modelName, reallyModelName);
    if(file_exists(reallyModelName, &isDir))
    {
        if (dbModuleInit(reallyModelName) != 0)
		{
			printf("load_cur_module: dbModuleInit failed!\n");
			return -1;
		}

		// select chip module.
		if (db_module_get_chip_name(chip_name) != 0)
		{
			printf("load_cur_module error: db_module_get_chip_name failed!\n");
			strcpy(chip_name, "Unknow");
		}

		ic_mgr_set_current_chip_id(chip_name);
		
        if (db_module_get_power_cfg_file_name(pwrCfgName)!= 0)
		{
			printf("load_cur_module: db_module_get_power_cfg_file_name failed!\n");
			return -1;
		}
		
        if (db_module_get_timing_cfg_file_name(timCfgName) != 0)
        {
			printf("load_cur_module: db_module_get_timing_cfg_file_name failed!\n");
			return -1;
		}

		if (db_module_get_comm_cfg_name(str_comm_cfg_name) != 0)
        {
			printf("load_cur_module: db_module_get_comm_cfg_name failed!\n");
			//return -1;
		}

		if (db_module_get_monitor_cfg_name(str_monitor_cfg_name) != 0)
        {
			printf("load_cur_module: db_module_get_monitor_cfg_name failed!\n");
			//return -1;
		}

		if (read_power_config(pwrCfgName, &s_power_info))
        {
			printf("load_cur_module: read_pwr_config failed!\n");
			return -1;
		}

		printf("****** VCOM Setting:  *****\n");
		printf("vcom set: %d.\n", s_power_info.vcom_set_volt);
		printf("vcom v max: %d.\n", s_power_info.vcom_max_volt);
		printf("vcom v min: %d.\n", s_power_info.vcom_min_volt);
		printf("vcom current max: %d.\n", s_power_info.vcom_max_current);

		if ( (s_power_info.power_module.LEDChannel != 0) 
				 && (s_power_info.power_module.LEDNumber != 0) )
		{
			// CC Mode.
			s_power_info.bl_power_mode = E_BL_MODE_CONST_CURRENT;
			printf("Back Light Mode: Const Current!\n");
		}
		else
		{
			// CV Mode.
			s_power_info.bl_power_mode = E_BL_MODE_CONST_VOLT;
			printf("Back Light Mode: Const Volt!\n");
		}
		
        if (read_timging_config(timCfgName, &s_timing_info) != 0)
        {
			printf("load_cur_module: read_timging_config failed!\n");
			return -1;
		}

		if (comm_config_read_cfg(str_comm_cfg_name, &s_comm_cfg_info) != 0)
        {
			printf("load_cur_module: comm_config_read_cfg failed!\n");
			//return -1;
		}

		if (monitor_config_read_cfg(str_monitor_cfg_name, &s_monitor_cfg_info) != 0)
        {
			printf("load_cur_module: monitor_config_read_cfg failed!\n");
			//return -1;
		}

        pwrSetCfg(&s_power_info.power_module);
        fpgaSeqInfo_t  fpgaSeq;
        initFpgaOutputPictureTiming(&fpgaSeq);
        unsigned int clock = fpgaSeq.T3*fpgaSeq.T7*s_timing_info.timclockfreq/1000;

        unsigned short value = fpga_read(0x60);
        if (s_timing_info.timMainFreq <= 19*1000)
        {
    		value |= 0x1;
        	clock *= 4;	//four times primary frequency.
        }
        else
        	value &= (~0x1);

		fpga_write(0x60, value);
        //5338
        clock_5338_setFreq(clock);
        fpgaReset();
        fpgaSeqSet(&fpgaSeq);
        // FPGA REG1 寄存器说明
        // bit0~bit1:图形量化位宽(01:6bit；10:8bit；11:10bit)。只写不能读。
        // bit2~bit3:link数选择(00:单link；01:双link；11:4link)
        // bit4 DE信号电平
        // bit5 行同步信号电平
        // bit6 场同步电平
        // bit7 0:vasa 1:jeida
        // bit8~bit15 控制输出信号连接模式，每个link均可以配置为0、1、2、3中的一种;
        // 其中link0由bit9~bit8控制，link1由bit11~bit10控制，link2由bit13~bit12控制，link3由bit15~bit14控制；
        fpgaBitSet(s_timing_info.timbit);

        printf("set module link num\n");
        fpgaLinkSet(s_timing_info.timsignalmode);

        printf("set singal polar sync_pri_h:%d sync_pri_v:%d sync_pri_de:%d\n",\
               s_timing_info.timhsyncpri,s_timing_info.timvsyncpri,s_timing_info.timde);

			   
        syncSingalLevelInfo_t syncSingalLevel = {0};
        syncSingalLevel.de = s_timing_info.timde;
        syncSingalLevel.sync_pri_h = s_timing_info.timhsyncpri;
        syncSingalLevel.sync_pri_v = s_timing_info.timvsyncpri;
        fpgaSignalSyncLevel(&syncSingalLevel);
		
        printf("set jeda vesa info %d\n",s_timing_info.timvesajeda);
        vesaJedaSwitchInfo_t vesaJedaSwitchInfo = {0};
        vesaJedaSwitchInfo.lvdsChange = s_timing_info.timvesajeda;
        fpgaVesaJedaSwitch(&vesaJedaSwitchInfo);

		// set bit and vesa jidea mode for sp6.
		sp6_set_lcd_bit_and_vesa_jeida(s_timing_info.timbit, s_timing_info.timvesajeda);
		
        printf("set link num %d\n",s_timing_info.timsignalmode);
        lvdsSignalModuleStr lvdsSignalModule = {0};
        lvdsSignalModule.linkCount = s_timing_info.timsignalmode;
        lvdsSignalModule.module[0] = s_timing_info.timlink[0];
        lvdsSignalModule.module[1] = s_timing_info.timlink[1];
        lvdsSignalModule.module[2] = s_timing_info.timlink[2];
        lvdsSignalModule.module[3] = s_timing_info.timlink[3];
        setFpgaSignalMode(&lvdsSignalModule);

		if(strlen(s_timing_info.timinitcodename) > 0)
			icMgrSetInitCodeName(s_timing_info.timinitcodename);

		refresh_lcd_msg_pos();
    }
    else
    {
        client_rebackError(SYNC_FILE_ERROR);
        DBG("file is not exist %s",reallyModelName);
        return -1;
    }
	
    printf("*********** load module end ***************\n");
    return 0;
}

static int pg_pwr_status = 0;

int  client_pg_pwr_status_get()
{
    return pg_pwr_status;
}

void  client_pg_pwr_status_set(int enable)
{
    pg_pwr_status = enable;
}

static void set_vcom_volta(unsigned int maxLimit, unsigned int volta)
{
	int enable = 0;
	if (volta > 0)
		enable = 1;

	mcu_set_vcom_max_volt(maxLimit);
	_msleep(10);
	mcu_set_vcom_volt(volta);
	_msleep(10);
	mcu_set_vcom_enable(enable);
}

static void doIcNonePowerTiming(bool bOn)
{
	_msleep(55);	//It is for waiting the power controller activing.

	s1103PwrCfgDb* pPwrCfg = &s_power_info.power_module;
	if (bOn)
	{
		/* left */
		gpio_set_output_value(CHANNEL_ENABLE_GPIO_L, 0);

		gpio_set_output_value(GPIO_PIN_UD_L, 0);
		gpio_set_output_value(GPIO_PIN_LR_L, 0);
		gpio_set_output_value(GPIO_PIN_MODE_L, 0);
		gpio_set_output_value(GPIO_PIN_STBY_L, 0);
		gpio_set_output_value(GPIO_PIN_RST_L, 0);

		/* right */
		gpio_set_output_value(CHANNEL_ENABLE_GPIO_R, 0);

		gpio_set_output_value(GPIO_PIN_UD_R, 0);
		gpio_set_output_value(GPIO_PIN_LR_R, 0);
		gpio_set_output_value(GPIO_PIN_MODE_R, 0);
		gpio_set_output_value(GPIO_PIN_STBY_R, 0);
		gpio_set_output_value(GPIO_PIN_RST_R, 0);
	}

	int deadLine = 0;
	int udTime = 0,lrTime = 0,modeTime = 0,stbyTime = 0,rstTime = 0;

	if (bOn)
	{
		udTime = pPwrCfg->udOpenDelay;
		lrTime = pPwrCfg->lrOpenDelay;
		modeTime = pPwrCfg->modeOpenDelay;
		stbyTime = pPwrCfg->stbyOpenDelay;
		rstTime = pPwrCfg->rstOpenDelay;
	}
	else
	{
		udTime = pPwrCfg->udCloseDelay;
		lrTime = pPwrCfg->lrCloseDelay;
		modeTime = pPwrCfg->modeCloseDelay;
		stbyTime = pPwrCfg->stbyCloseDelay;
		rstTime = pPwrCfg->rstCloseDelay;
	}

	while (1)
	{
		if (udTime == deadLine)
		{
			gpio_set_output_value(GPIO_PIN_UD_L, bOn);
			gpio_set_output_value(GPIO_PIN_UD_R, bOn);
		}

		if (lrTime == deadLine)
		{
			gpio_set_output_value(GPIO_PIN_LR_L, bOn);
			gpio_set_output_value(GPIO_PIN_LR_R, bOn);
		}

		if (modeTime == deadLine)
		{
			gpio_set_output_value(GPIO_PIN_MODE_L, bOn);
			gpio_set_output_value(GPIO_PIN_MODE_R, bOn);
		}

		if (stbyTime == deadLine)
		{
			gpio_set_output_value(GPIO_PIN_STBY_L, bOn);
			gpio_set_output_value(GPIO_PIN_STBY_R, bOn);
		}

		if (rstTime == deadLine)
		{
			gpio_set_output_value(GPIO_PIN_RST_L, bOn);
			gpio_set_output_value(GPIO_PIN_RST_R, bOn);
		}

		deadLine++;
		_msleep(1);

		if (udTime < deadLine && lrTime < deadLine
			&& modeTime < deadLine && stbyTime < deadLine
			&& rstTime < deadLine)
			break;
	}

	if (!bOn)
	{
		/* left */
		gpio_set_output_value(CHANNEL_ENABLE_GPIO_L, 1);
		/* right */
		gpio_set_output_value(CHANNEL_ENABLE_GPIO_R, 1);
	}
}

static void initTtlOrLvdsFpgaTiming()
{
	unsigned char mode = sp6_read_8bit_data(0x29);

	if (s_timing_info.timMainFreq > 19*1000)
	{
		mode &= ~(0x1 << 5);
		sp6_write_8bit_data(0x29, mode);
		return;
	}

	mode |= (0x1 << 5);
	sp6_write_8bit_data(0x29, mode);

	fpgaSeqInfo_t  fpgaSeq = {0};
	initFpgaOutputPictureTiming(&fpgaSeq);

	sp6_write_8bit_data(0x2b, fpgaSeq.T0&0xff);
	sp6_write_8bit_data(0x2c, (fpgaSeq.T0 >> 8)&0xff);

	sp6_write_8bit_data(0x2d, fpgaSeq.T1&0xff);
	sp6_write_8bit_data(0x2e, (fpgaSeq.T1 >> 8)&0xff);

	sp6_write_8bit_data(0x2f, fpgaSeq.T2&0xff);
	sp6_write_8bit_data(0x30, (fpgaSeq.T2 >> 8)&0xff);

	sp6_write_8bit_data(0x31, fpgaSeq.T3&0xff);
	sp6_write_8bit_data(0x32, (fpgaSeq.T3 >> 8)&0xff);

	sp6_write_8bit_data(0x33, fpgaSeq.T4&0xff);
	sp6_write_8bit_data(0x34, (fpgaSeq.T4 >> 8)&0xff);

	sp6_write_8bit_data(0x35, fpgaSeq.T5&0xff);
	sp6_write_8bit_data(0x36, (fpgaSeq.T5 >> 8)&0xff);

	sp6_write_8bit_data(0x37, fpgaSeq.T6&0xff);
	sp6_write_8bit_data(0x38, (fpgaSeq.T6 >> 8)&0xff);

	sp6_write_8bit_data(0x39, fpgaSeq.T7&0xff);
	sp6_write_8bit_data(0x3a, (fpgaSeq.T7 >> 8)&0xff);
}

static void dp_signal_interface_init()
{
}

static void mipi_siganl_interface_init()
{
	gpio_set_output_value(SUPPLY_POWER_2828_GPIO, 1);

	gpio_set_output_value(RST_2828_GPIO, 0);
	_msleep(10);
	gpio_set_output_value(RST_2828_GPIO, 1);

	fpga_write(0xa9, 0x12);
	_msleep(2);
	fpga_write(0xa9, 0x10);

	fpga_write(0x56, 0); // 数据时钟输出
}

static void lvds_signal_interface_init()
{
	initTtlOrLvdsFpgaTiming();

	fpga_write(0xa9, 0x14);
	_msleep(2);
	fpga_write(0xa9, 0x10);

	fpga_write(0x56, 0); // 数据时钟输出
}

static void ttl_signal_interface_init()
{
	lvds_signal_interface_init();
}

int client_pg_shutON(int enable, char *pModelName, int channel, int ptn_id)
{
    char reallyModelName[256] = "";
    char pwrCfgName[256] = "";
    char timCfgName[256] = "";
	
    if(enable == 1)
    {
        int isDir;
        sprintf(reallyModelName, "%s/%s", TST_PATH, pModelName);
        DBG("PG shut on with model %s %s", pModelName, reallyModelName);

        if(file_exists(reallyModelName, &isDir))
        { 	
        	box_show_being_power_on_msg();
        	show_signal_type_info(s_timing_info.timsignaltype);
			
        	int channel = PWR_CHANNEL_ALL;
        	if (s_timing_info.timsignaltype == E_SIGNAL_TYPE_TTL)
        	{
    			// Open short for TTL only.
				if (s_monitor_cfg_info.open_short_enable)
				{
					channel = open_short_test_singal_and_io(&s_monitor_cfg_info);
					if (channel <= 0)
					{
						box_notify_openshort_error_down();
						return -1;
					}
				}
        	}

    		printf("*** do power on ***\n");
//    		client_pg_showPtn(ptn_id);
    		pwrSetState(true, channel);
    		doIcNonePowerTiming(true);
            client_pg_showPtn(ptn_id);

            if(s_timing_info.timsignaltype == E_SIGNAL_TYPE_EDP) //edp
            {
            	dp_signal_interface_init();
            	fpga_write_16bits(FPGA_CLOCK_REGA8, 1); //set output signal type.

			    icMgrSndInitCode(DEV_CHANNEL_0);
			    icMgrSndInitCode(DEV_CHANNEL_1);
			    icMgrSndInitCode(DEV_CHANNEL_2);
			    icMgrSndInitCode(DEV_CHANNEL_3);
            }
			else if (s_timing_info.timsignaltype == E_SIGNAL_TYPE_MIPI)
			{
				mipi_siganl_interface_init();
				mipi2828Open(&s_timing_info, channel);
				fpga_write_16bits(FPGA_CLOCK_REGA8, 2); //set output signal type.

			    icMgrSndInitCode(DEV_CHANNEL_0);
			    icMgrSndInitCode(DEV_CHANNEL_1);
			    icMgrSndInitCode(DEV_CHANNEL_2);
			    icMgrSndInitCode(DEV_CHANNEL_3);
			}
			else if (s_timing_info.timsignaltype == E_SIGNAL_TYPE_LVDS)
			{	
				sp6_open_lvds_signal();
				lvds_signal_interface_init();
				set_vcom_volta(s_power_info.vcom_max_volt, s_power_info.vcom_set_volt);
				fpga_write_16bits(FPGA_CLOCK_REGA8, 4); //set output signal type.

			    icMgrSndInitCode(DEV_CHANNEL_0);
			    icMgrSndInitCode(DEV_CHANNEL_1);
			}
			else if (s_timing_info.timsignaltype == E_SIGNAL_TYPE_TTL)
			{
				ttl_signal_interface_init();
				sp6_open_ttl_signal();

				if (s_monitor_cfg_info.ntc_enable)
					setNtcCheckPause(false);

				set_vcom_volta(s_power_info.vcom_max_volt, s_power_info.vcom_set_volt);
				fpga_write_16bits(FPGA_CLOCK_REGA8, 4); //set output signal type.

			    icMgrSndInitCode(DEV_CHANNEL_0);
			    icMgrSndInitCode(DEV_CHANNEL_1);
			}
			else
			{
				printf("unknown signal type:%d\r\n", s_timing_info.timsignaltype);
				return;
			}

            client_rebackPower(1);
            pg_pwr_status = 1;
        }
        else
        {
            printf("client_pg_shutON error: file is not exist %s", reallyModelName);
			// notify box and PC.
        }

		client_notify_msg(0, "开电成功!");
		box_notify_power_on_end();
		printf("Power On OK!\n");
    }
    else
    {
		printf("*** do power off ***\n");
		pwrSetState(false, PWR_CHANNEL_ALL);
		doIcNonePowerTiming(false);
    	if (s_monitor_cfg_info.ntc_enable)
    		setNtcCheckPause(true);

		if (s_timing_info.timsignaltype == E_SIGNAL_TYPE_LVDS)
		{
			sp6_close_lvds_signal();
			mcu_set_vcom_enable(0);
		}
		else if (s_timing_info.timsignaltype == E_SIGNAL_TYPE_MIPI)
		{
			mipi2828Close();
			fpga_write(0xa9, 0);
		}
		else if (s_timing_info.timsignaltype == E_SIGNAL_TYPE_TTL)
		{
			sp6_close_ttl_signal();
			mcu_set_vcom_enable(0);
		}

		fpga_write_16bits(FPGA_CLOCK_REGA8, 0);
		fpga_write(0x56, 1);

        client_rebackPower(0);
		box_notify_power_down();
		client_notify_msg(0, "设备关电!");
		
        pg_pwr_status = 0;
        printf("===========   Power Down! pg_pwr_status = %d. \n", pg_pwr_status);
    }

	return NOTIFY_ERROR_OK;
}

static int pg_cur_ptnId = 0;

int  client_pg_ptnId_get()
{
    return pg_cur_ptnId;
}

void  client_pg_ptnId_set(int ptnId)
{
    pg_cur_ptnId = ptnId;
}

static void doPicConfig(int ptnId)
{
	printf("%s: do nothing\r\n", __FUNCTION__);
}

void client_pg_showPtn(int ptnId)
{
	doPicConfig(ptnId);
	setCurrentPicture(ptnId);
    client_pg_ptnId_set(ptnId);
}

void client_pg_showCrossCur(pointer_xy_t *pPointer_xy,int num)
{
    int i;

    for(i=0;i<num;i++)
    {
       crossCursorInfo_t  info;
       info.enable = 1;
       info.x = pPointer_xy[i].x;
       info.y = pPointer_xy[i].y;
       info.wordColor = 0xFFFFFF;
       info.crossCursorColor = 0;
       info.RGBchang = 0;
       info.HVflag = 0;
       info.startCoordinate = 0;
       fpgaCrossCursorSet(&info);
    }
}

void client_pg_hideCrossCur()
{
    crossCursorInfo_t info = {0};
    info.enable = 0;
    info.x = 0;
    info.y = 0;
    info.wordColor = 0xFFFFFF;
    info.crossCursorColor = 0;
    info.RGBchang = 0;
    info.HVflag = 0;
    info.startCoordinate = 0;
    fpgaCrossCursorSet(&info);
}

void client_pg_upgradeFile(char *pSrcFile,char *pFileType,char *pDstFile)
{
}
