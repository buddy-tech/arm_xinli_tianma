#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "hi_unf_spi.h"
#include "vcom.h"
#include "pgDB/pgDB.h"
#include "common.h"
#include "comUart.h"
#include "../box/box.h"

#include "ca310.h"
#include "clipg.h"
#include "ic_manager.h"
#include "pubLcdTask.h"
#include "mipiflick.h"
#include "client.h"

static pthread_mutex_t flickLock;

typedef struct tag_auto_flick_seg_s
{
    int  vcomIndex;
    /*unsigned*/ int  flickValue;
    char *autoFlickCode;
}auto_flick_seg_t;

typedef struct tag_auto_flick_s
{
    char pageCode[32];
    int  curFlickIndex;
    int  totalFlickNum;
    int  curCurve;
    auto_flick_seg_t *auto_flick_seg;
}auto_flick_t;

static auto_flick_t auto_flick;

typedef struct tag_vcom_value_info_s
{
    int minvcom_index;
    int maxvcom_index;
    int width;
    int flickJudgeValue;
    int isBurner;
    int isVcomCheck;

	int idBurner; 
	int vcomBurner;
	int gammaBurner; 
	int pwrOnDelay;

}vcom_value_info_t;

static vcom_value_info_t vcom_value_info;

void vcom_minmax_init(int min,int max,int flickJudgeValue,int isBurner,int isVcomCheck,int idBurner,int vcomBurner,int gammaBurner)
{
    vcom_value_info.minvcom_index = min;
    vcom_value_info.maxvcom_index = max;
    vcom_value_info.width = max-min+1;
    vcom_value_info.flickJudgeValue = flickJudgeValue;
    vcom_value_info.isBurner = isBurner;
    vcom_value_info.isVcomCheck = isVcomCheck;

	vcom_value_info.idBurner = idBurner;
	vcom_value_info.vcomBurner = vcomBurner;
	vcom_value_info.gammaBurner = gammaBurner;

}

static pthread_t flickthread = 0;
static int  stopFlickFlag = 0;
void autoFlickTask(void *param);
static int  curmode;
static auto_flick_t m_channel_auto_flick[2];
static pthread_t m_channel_flick_thread[2];
sem_t m_channel_flick_thread_start_sem[2];
sem_t m_channel_flick_thread_sem[2]; 
int m_channel_flick_thread_init = 0;
void channelFlickTask(void *param);
void box_channelFlickTask(void *param);

void flick_auto(int enable,int mode) //0:network;1:uart
{
    curmode = mode;
    if(enable)
    {
        int ret = 0;
        stopFlickFlag = 0;
        pthread_create(&flickthread, NULL, (void *) autoFlickTask, &curmode);
        if (ret != 0)
        {
            printf("can't create thread autoFlickTask: %s.\r\n", strerror(ret));
            return ;
        }
    }
    else
    {
        stopFlickFlag = 1;
        if(flickthread!=0)
        {
            pthread_join(flickthread,NULL);
            flickthread = 0;
        }
    }
}

#if 1
int m_channel_1 = 1;
int m_channel_2 = 4;
int m_channel_3 = 2; 
int m_channel_4 = 3; 

int channel_list[4] = {0, 0, 0, 0};
int lcd_channel_list[4] = {0, 0, 0, 0};
int vcom_list[4] = {0, 0, 0, 0};
int flick_list[4] = {0, 0, 0, 0};

int auto_flick_return_code = 0;
int auto_flick_return_code_2 = 0;
int auto_flick_return_code_3 = 0;
int auto_flick_return_code_4 = 0;

int auto_flick_thread_init = 0;
pthread_t auto_flick_thread[2];
sem_t auto_flick_thread_start_sem[2];
sem_t auto_flick_thread_sem[2];

int auto_flick_initVcom = 0;
int auto_flick_maxVcom = 0;
int auto_flick_readDelay = 0;

void autoFlickTask(void *param)
{
	printf("====  autoFlickTask run ...\n");

	// Load Module Data.
	int curPtnId = client_pg_ptnId_get();
	if(curPtnId == -1) 
		return;
	
	module_info_t *pModule_info = dbModuleGetPatternById(curPtnId);
	if(!pModule_info) 
		return;
	
	vcom_info_t vcom_info;

	vcom_info_t* p_vcom_info = get_current_vcom_info();
	if (p_vcom_info)
		vcom_info = *p_vcom_info;
	
	printf("!!!!!!!!!!!!vcom::%d %d %f-%d %d\n",vcom_info.minvalue,vcom_info.maxvalue,
			vcom_info.f_max_valid_flick, vcom_info.isBurner,vcom_info.isVcomCheck);

	// set vcom init data.
	auto_flick_initVcom = vcom_info.initVcom;
	auto_flick_readDelay = vcom_info.readDelay;
	auto_flick_maxVcom = vcom_info.maxvalue;

	// Only enable channel 1 now.
	// If enable channel 1 and channel 4, just set auto_flick_channel_nums to 2;
	int auto_flick_channel_nums = 1;

	if(auto_flick_thread_init == 0)
	{
		int i;
		for(i=0; i < auto_flick_channel_nums; i++)
		{
			sem_init(&auto_flick_thread_start_sem[i], 0, 0);
			sem_init(&auto_flick_thread_sem[i], 0, 0);
			pthread_create(&auto_flick_thread[i], NULL, (void *) channelFlickTask, (void*)i);
		}
		auto_flick_thread_init = 1;
	}

	//
	int need_check = 1;
	int need_check_2 = 0;

	if (auto_flick_channel_nums == 2)
		need_check_2 = 1;

	// channel 1 check module.
	if(need_check)
	{
		int check_error = ic_mgr_check_chip_ok(m_channel_1);
		printf("ic_mgr_check_chip_ok: channel %d, error: %d.\n", m_channel_1, check_error);
		
		if (check_error)
			need_check = 0;
	}

	// channel 2 check module.
	if(need_check_2)
	{
		int check_error = ic_mgr_check_chip_ok(m_channel_2);
		printf("ic_mgr_check_chip_ok: channel %d, error: %d.\n", m_channel_2, check_error);
		if (check_error)
			need_check_2 = 0;
	}

	// channel 1 check CA310.
	if(need_check)
	{
		if(lcd_getStatus(1) == 0)
		{		
			printf("lcd_getStatus: channel %d, ca310 error!\n", m_channel_1);
			need_check = 0;
		}
	}

	// channel 2 check CA310.
	if(need_check_2)
	{
		if(lcd_getStatus(2) == 0)
		{
			printf("lcd_getStatus: channel %d, ca310 error!\n", m_channel_2);
			need_check_2 = 0;
		}
	}

#if 1
	// start auto flick ...
	if(need_check)
	{
		printf("start channel 1 flick ...!\n");
		sem_post(&auto_flick_thread_start_sem[0]);
	}
	if(need_check_2)
	{	
		printf("start channel 2 flick ...!\n");
		sem_post(&auto_flick_thread_start_sem[1]);
	}

	// auto flick end.
	if(need_check)
	{
		sem_wait(&auto_flick_thread_sem[0]);
	}
	if(need_check_2)
	{
		sem_wait(&auto_flick_thread_sem[1]);
	}
#endif
	// 
	int vcom = 0;
	int flick = 0;
	mipi_channel_get_vcom_and_flick(m_channel_1, &vcom, &flick);
	client_rebackFlickOver(m_channel_1, vcom, flick);
			
	printf("autoFlickTask over\n");
}


#if 1
int z_flick_test(int mipi_channel, int porbe_channel, int read_delay, int vcom, int* p_flick_value)
{
	int retval = 0;

	if (p_flick_value == NULL)
	{
		printf("z_flick_test error: Invalid param!\n");
		return PROBE_STATU_OTHER_ERROR;
	}
	
	ic_mgr_write_vcom(mipi_channel, vcom);
	
	if(read_delay > 0) 
		usleep(1000 * read_delay); 

	float f_flick = 0.00;
	retval = ca310_capture_flick_data(porbe_channel, &f_flick);
	if (retval == PROBE_STATU_OK)
	{
		*p_flick_value = f_flick * 100;
		return PROBE_STATU_OK;
	}
	else if (retval == PROBE_STATU_OUT_RANGE)
	{
		printf("z_flick_test: flick value out range!\n");
		*p_flick_value = INVALID_FLICK_VALUE;
		return PROBE_STATU_OUT_RANGE;
	}
	else
	{
		printf("z_flick_test: flick value out range!\n");
		*p_flick_value = INVALID_FLICK_VALUE;
		return PROBE_STATU_OTHER_ERROR;
	}
}


#define Z_MIN_VCOM 		(0)
#define Z_MAX_VCOM 		(255)

void flick_notify_pc(int mipi_channel, int vcom, int flick)
{
	if(mipi_channel == m_channel_1)
	{
		if (flick < 0)
			flick = INVALID_FLICK_VALUE * 100;
		client_rebackautoFlick(mipi_channel, vcom, flick);
	}
}

static int change_direct(int direction)
{
	if (direction == -1)
	{
		direction = 1;
	}
	else
	{
		direction = -1;
	}

	printf("--- change dirction: ==> %d.\n", direction);

	return direction;
}

static int s_max_error_cnt = 10;
static int s_max_change_direct_cnt = 5;
// find_direction: -1: left; 0: none; 1: right;
int z_auto_flick_2(int mipi_channel, int probe_channel, int read_delay, int min_vcom, int max_vcom, 
					int init_vcom, int *p_find_vcom, int* p_find_flick, int send_net_notify, int last_vcom,
					int last_flick, int find_direction, int min_ok_flick, int flick_error_cnt,
					int max_ok_flick, int left_find_min_flick, int right_find_min_vcom,
					int right_find_min_flick, int change_direction_cnt)
{
	int vcom = 0;
	int flick = 0;
	int step = 0;

	printf("====================================================================================\n");
	
	if (last_flick < 0)
	{
		// flick is too large to get right flick value.
		flick_error_cnt ++;
		step = 10;

		if (find_direction == -1)
		{
			// ==> left
			if (last_vcom > min_vcom + step && last_vcom <= max_vcom)
			{
				vcom = last_vcom - step;
				printf("channel: %d, left 10: last=%d, now=%d.\n", mipi_channel, last_vcom, vcom);
			}
			else
			{
				printf("channel: %d, z_auto_flick_2: *** find to left end! 10 ***\n", mipi_channel);
				find_direction = change_direct(find_direction);
				vcom = min_vcom + step;
				change_direction_cnt ++;
			}
		}
		else
		{
			// ==> right
			if (last_vcom >= min_vcom && last_vcom < max_vcom - step)
			{
				vcom = last_vcom + step;
				printf("channel: %d, right 10: last=%d, now=%d.\n", mipi_channel, last_vcom, vcom);
			}
			else
			{
				printf("channel: %d, z_auto_flick_2: *** find to right end! 10 ***\n", mipi_channel);
				find_direction = change_direct(find_direction);
				vcom = max_vcom - step;
				change_direction_cnt ++;
			}
		}
		
		
	}
	else if (last_flick >= (int)INVALID_FLICK_VALUE)
	{
		// flick is too large to get right flick value.
		flick_error_cnt ++;
		step = 10;

		if (find_direction == -1)
		{
			// ==> left
			if (last_vcom > min_vcom + step && last_vcom <= max_vcom)
			{
				vcom = last_vcom - step;
				printf("channel: %d, left 10: last=%d, now=%d.\n", mipi_channel, last_vcom, vcom);
			}
			else
			{
				printf("channel: %d, z_auto_flick_2: *** find to left end! 10 ***\n", mipi_channel);
				find_direction = change_direct(find_direction);
				vcom = min_vcom + step;
				change_direction_cnt ++;
			}
		}
		else
		{
			// ==> right
			if (last_vcom >= min_vcom && last_vcom < max_vcom - step)
			{
				vcom = last_vcom + step;
				printf("channel: %d, right 10: last=%d, now=%d.\n", mipi_channel, last_vcom, vcom);
			}
			else
			{
				printf("channel: %d, z_auto_flick_2: *** find to right end! 10 ***\n", mipi_channel);
				find_direction = change_direct(find_direction);
				vcom = max_vcom - step;
				change_direction_cnt ++;
			}
		}
		
		
	}
	else if (last_flick > 50 * 100)
	{
		step = 5;

		if (find_direction == -1)
		{
			// ==> left
			if (last_vcom > min_vcom + step && last_vcom < max_vcom)
			{
				vcom = last_vcom - step;
				printf("channel: %d, left 5: last=%d, now=%d.\n", mipi_channel, last_vcom, vcom);
			}
			else
			{
				printf("channel: %d, z_auto_flick_2: *** find to left end 5! ***\n", mipi_channel);
				find_direction = change_direct(find_direction);
				vcom = min_vcom + step;
				change_direction_cnt ++;
			}
		}
		else
		{
			// ==> right
			if (last_vcom > min_vcom && last_vcom < max_vcom - step)
			{
				vcom = last_vcom + step;
				printf("channel: %d, right 5: last=%d, now=%d.\n", mipi_channel, last_vcom, vcom);
			}
			else
			{
				printf("channel: %d, z_auto_flick_2: *** find to right end 5! ***\n", mipi_channel);
				find_direction = change_direct(find_direction);
				vcom = max_vcom - step;
				change_direction_cnt ++; 
			}
		
		}
	}
	else if (last_flick > 20 * 100)
	{
		step = 3;
		
		if (find_direction == -1)
		{
			// ==> left
			if (last_vcom > min_vcom + step && last_vcom < max_vcom)
			{
				vcom = last_vcom - step;
				printf("channel: %d, left 3: last=%d, now=%d.\n", mipi_channel, last_vcom, vcom);
			}
			else
			{
				printf("channel: %d, z_auto_flick_2: *** find to left end 3! ***\n", mipi_channel);
				find_direction = change_direct(find_direction);
				vcom = min_vcom + step;
				change_direction_cnt ++;
			}
		}
		else
		{
			// ==> right
			if (last_vcom > min_vcom && last_vcom < max_vcom - step)
			{
				vcom = last_vcom + step;
				printf("channel: %d, right 3: last=%d, now=%d.\n", mipi_channel, last_vcom, vcom);
			}
			else
			{
				printf("channel: %d, z_auto_flick_2: *** find to right end 3! ***\n", mipi_channel);
				find_direction = change_direct(find_direction);
				vcom = max_vcom - step;
				change_direction_cnt ++;
			}
		
		}
	}
	else
	{
		step = 1;

		if (find_direction == -1)
		{
			// ==> left
			if (last_vcom > min_vcom + step && last_vcom < max_vcom)
			{
				vcom = last_vcom - step;
				printf("channel: %d, left 1: last=%d, now=%d.\n", mipi_channel, last_vcom, vcom);
			}
			else
			{
				printf("channel: %d, z_auto_flick_2: *** find to left end 1! ***\n", mipi_channel);
				find_direction = change_direct(find_direction);
				vcom = min_vcom + step;
				change_direction_cnt ++;
			}
		}
		else
		{
			// ==> right
			if (last_vcom > min_vcom && last_vcom < max_vcom - step)
			{
				vcom = last_vcom + step;
				printf("channel: %d, right 1: last=%d, now=%d.\n", mipi_channel, last_vcom, vcom);
			}
			else
			{
				printf("channel: %d, z_auto_flick_2: *** find to right end 1! ***\n", mipi_channel);
				find_direction = change_direct(find_direction);
				vcom = max_vcom - step;
				change_direction_cnt ++;
			}
		
		}
	}

	int ret = PROBE_STATU_OK;
	ret = z_flick_test(mipi_channel, probe_channel, read_delay, vcom, &flick);
	if (ret == PROBE_STATU_OUT_RANGE)
	{
	
	}
	else if (ret == PROBE_STATU_OTHER_ERROR)
	{
		
	}
	
	printf("channel: %d, last vcom: %d, flick: %d. ==> now: VCOM: %d. flick: %d. \n", mipi_channel, last_vcom, last_flick, vcom, flick);
	
	if (send_net_notify)
		flick_notify_pc(mipi_channel, vcom, flick);

	int show_vcom = 1;
	int show_ok = 0;
	int ok1 = 0;
	int ok2 = 0;
	show_lcd_msg(mipi_channel, show_vcom, show_ok, vcom, flick, ok1, ok2);

	if (flick > 0)
	{
		if (last_flick > 0)
		{
			if (last_flick < flick)
			{
				if (step == 1 && change_direction_cnt >= 1)
				{
					// find ok.
					printf("channel: %d, *** step 1: find min value: vcom = %d, flick = %d, min_ok = %d. ***\n",
							mipi_channel, last_vcom, last_flick, min_ok_flick);
					*p_find_vcom = last_vcom;
					*p_find_flick = last_flick;
					return 0;
				}
				
				if (find_direction == 0)
				{
					// detect direction
					find_direction = -1;
				}
				else
				{
					find_direction = -find_direction;
				}

				change_direction_cnt ++;
				// change direction.
				//if (find_direction == -1)
				//	find_direction = 1;
			}
		}

	}

	if (flick > 0 )
	{
		if (flick < min_ok_flick )
		{
			printf("channel: %d, *** find end: flick = %d, min_ok = %d. ***\n", mipi_channel, flick, min_ok_flick);
			*p_find_vcom = vcom;
			*p_find_flick = flick;
			return 0;
		}
		else if (flick < max_ok_flick && change_direction_cnt >= s_max_change_direct_cnt)
		{
			// find right flick value.
			if (last_flick < flick)
			{
				*p_find_vcom = last_vcom;
				*p_find_flick = last_flick;				
			}
			else
			{
				*p_find_vcom = vcom;
				*p_find_flick = flick;
			}
			
			printf("channel: %d, ====== find end. find min vcom: vcom = %d, flick = %d, min_ok = %d. ***\n",
					mipi_channel, *p_find_vcom, *p_find_flick, min_ok_flick);
			
			return 0;
		}
	}

	if (flick_error_cnt >= s_max_error_cnt && flick >= (int)INVALID_FLICK_VALUE)
	{
		printf("channel: %d, *** find error: flick = %d. try error cnt: %d. ***\n", mipi_channel, flick, flick_error_cnt);
		return 0;
	}

	if (change_direction_cnt >= s_max_change_direct_cnt)
	{
		printf("channel: %d, *** find error: change direction times = %d.\n", change_direction_cnt);
		return -1;
	}

	last_vcom = vcom;
	last_flick = flick;
	return z_auto_flick_2(mipi_channel, probe_channel, read_delay, min_vcom, max_vcom, 
					init_vcom, p_find_vcom, p_find_flick, send_net_notify, last_vcom,
					last_flick, find_direction, min_ok_flick, flick_error_cnt, max_ok_flick, 0, 0, 0,
					change_direction_cnt);
}

#endif

#define MAX_MIPI_CHANNEL_NUMS	(4)
int s_find_vcom[MAX_MIPI_CHANNEL_NUMS] = { 0 };
int s_find_flick[MAX_MIPI_CHANNEL_NUMS] = { 0 };

int mipi_channel_get_vcom_and_flick(int mipi_channel, int *p_vcom, int *p_flick)
{
	if (p_vcom == NULL)
		return -1;

	if (p_flick == NULL)
		return -1;
	
	if (mipi_channel > 0 && mipi_channel <= MAX_MIPI_CHANNEL_NUMS)
	{
		*p_vcom = s_find_vcom[mipi_channel];
		*p_flick = s_find_flick[mipi_channel];
		return 0;
	}
	
	return -1;
}

int mipi_channel_set_vcom_and_flick(int mipi_channel, int vcom, int flick)
{
	if (mipi_channel > 0 && mipi_channel <= MAX_MIPI_CHANNEL_NUMS)
	{
		s_find_vcom[mipi_channel] = vcom;
		s_find_flick[mipi_channel] = flick;
		return 0;
	}
	
	return -1;
}


void channelFlickTask(void *param)
{
	int index = (int)param;
	int channel;
	int lcd_channel;
	
	if(index == 0)
	{
		channel = m_channel_1;
		lcd_channel = 1;
		printf("channel 1 channelFlickTask run ...\n");
	}
	else
	{
		channel = m_channel_2;
		lcd_channel = 2;
		printf("channel 2 channelFlickTask run ...\n");
	}
	
	while(1)
	{
		sem_wait(&auto_flick_thread_start_sem[index]);

		//
		int vcom = auto_flick_initVcom;
		int last_flick = 0;
		int last_x = 0;
		int min_vcom = vcom;
		int min_vcom_flick = 0;
		int retVal = 0;
		int ok_vcom = 0;
		int ok_flick = 0;

		ca310_stop_flick_mode(lcd_channel);
		ca310_start_flick_mode(lcd_channel);
		
		int find_vcom = 0;
		int find_vcom2 = 0;
		int find_flick = 0;

		struct timeval tv1 = { 0 };
		struct timeval tv2 = { 0 };
		gettimeofday(&tv1, NULL);
		
		// read vcom config.
		vcom_info_t* p_vcom_info = get_current_vcom_info();
		printf("VCOM Info: \n");
		printf("min_vcom: %d.\n", p_vcom_info->minvalue);
		printf("max_vcom: %d.\n", p_vcom_info->maxvalue);
		printf("init_vcom: %d.\n", p_vcom_info->initVcom);
		printf("read delay: %d.\n", p_vcom_info->readDelay);

		{
			int min_ok_flick = p_vcom_info->f_ok_flick * 100;
			int max_ok_flick = p_vcom_info->f_max_valid_flick * 100;
			//int max_ok_flick = p_vcom_info->max_check_value * 100;

			int flick = 0;
			printf("min_ok_flick: %d, max_ok_flick: %d.\n", min_ok_flick, max_ok_flick);
			z_flick_test(channel, lcd_channel, p_vcom_info->pwrOnDelay, vcom, &flick);				

			float f_flick = 0.00;
			ca310_capture_flick_data(lcd_channel, &f_flick);
			flick = f_flick * 100;

			printf("init vcom: %d, flick: %d.\n", vcom, flick);

			int flick_erro = z_auto_flick_2(channel, lcd_channel, p_vcom_info->readDelay, 
											p_vcom_info->minvalue, p_vcom_info->maxvalue, 
											p_vcom_info->initVcom, &find_vcom, &find_flick, 1,
											vcom, flick, 0, min_ok_flick, 0, max_ok_flick, 0, 0, 0, 0);

			printf("z_auto_flick_2: end. find: vcom = %d, flick = %d.\n", find_vcom, find_flick);
			mipi_channel_set_vcom_and_flick(channel, find_vcom, find_flick);

			if (flick_erro == 0)
			{
			#ifdef USED_IC_MANAGER
			printf("*** write ok vcom: %d, %x ***\n", find_vcom, find_vcom);
			ic_mgr_write_vcom(channel, find_vcom);
			#else
			printf("*** NULL Flick process ***!\n");
			#endif
			}
		}
		
		gettimeofday(&tv2, NULL);
		int diff = ((tv2.tv_sec - tv1.tv_sec) * 1000 * 1000 + tv2.tv_usec - tv1.tv_usec) / 1000;
		printf("=== Auto Flick time: %dms.  tv1: %d.%d, tv2: %d.%d.	===\n", diff, tv1.tv_sec, tv1.tv_usec, tv2.tv_sec, tv2.tv_sec);

		ca310_stop_flick_mode(lcd_channel);
		sem_post(&auto_flick_thread_sem[index]);
	}
}
#endif


int box_autoFlick_initVcom = 0;
int box_autoFlick_maxVcom = 0;
int box_autoFlick_readDelay = 0;
int box_autoFlick_started[4] = {0, 0, 0, 0};

void box_autoFlick(vcom_info_t* vcom_info, int* vcomVcomValue1, int* vcomFlickValue1, int* vcomVcomValue2, int* vcomFlickValue2, char* otp1_text, char* otp2_text)
{
	box_autoFlick_initVcom = vcom_info->initVcom;
	box_autoFlick_readDelay = vcom_info->readDelay;
	box_autoFlick_maxVcom = vcom_info->maxvalue;

	channel_list[0] = m_channel_1;
	channel_list[1] = m_channel_2;

	//
	lcd_channel_list[0] = 1;
	lcd_channel_list[1] = 2;

	if(m_channel_flick_thread_init == 0)
	{
		int i;
		for(i=0; i < 2; i++)
		{
			sem_init(&m_channel_flick_thread_start_sem[i], 0, /*u32InitCnt*/0);
			sem_init(&m_channel_flick_thread_sem[i], 0, /*u32InitCnt*/0);
//			pthread_create(&m_channel_flick_thread[i], NULL, (void *) box_channelFlickTask, (void*)i);
		}

		m_channel_flick_thread_init = 1;
	}
	
	vcom_list[0] = 0;
	flick_list[0] = 0;
	vcom_list[1] = 0;
	flick_list[1] = 0;

	// start auto flick.
	int vcom_return_code = auto_flick_return_code;
	int vcom_return_code_2 = auto_flick_return_code_2;
	if(vcom_return_code == FLICKER_OK)
	{
		box_autoFlick_started[0] = 1;
		sem_post(&m_channel_flick_thread_start_sem[0]);
	}
	
	if(vcom_return_code_2 == FLICKER_OK)
	{
		box_autoFlick_started[1] = 1;
		sem_post(&m_channel_flick_thread_start_sem[1]);
	}

	// Wait auto flick end.
	if(vcom_return_code == FLICKER_OK)
	{
		sem_wait(&m_channel_flick_thread_sem[0]);
		box_autoFlick_started[0] = 0;
	}
	
	if(vcom_return_code_2 == FLICKER_OK)
	{
		sem_wait(&m_channel_flick_thread_sem[1]);
		box_autoFlick_started[1] = 0;
	}

	// check channel 1 flick value.
	if(auto_flick_return_code == FLICKER_OK)
	{
		int vcom = vcom_list[0];
		int flick = flick_list[0];
		printf("\nauto flick: channel=%d  vcom=%d  flick=%d\n", m_channel_1, vcom/*minIndex*/, flick/*minFlickValue*/);

		if(flick <= 0)
		{	
			// flick value error!
			// Can not find flick device!
			auto_flick_return_code = FLICKER_NG_2;
			sprintf(otp1_text, "NG FLIC�豸����,�븴λ");
			printf("line: %d. mipi_channel %d, %s\n", __LINE__, m_channel_1, otp1_text);
		}
		else
		{
			*vcomVcomValue1 = vcom;
			*vcomFlickValue1 = flick;
					
			if(vcom_info->f_max_valid_flick*100 > flick)
			{
				if(flick <= 70)
				{
					// NG, FLICK is xx, please reburn.
					auto_flick_return_code = FLICKER_NG_2;
					sprintf(otp1_text, "NG FLICK��%.1f�쳣,������!", ((float)flick)/100.0);
					printf("line: %d. %d, %s\n", __LINE__, m_channel_1, otp1_text);
				}
			}
			else //������¼
			{
				auto_flick_return_code = FLICKER_NG_2;
				sprintf(otp1_text, "NG FLICK��%.1f,������!", ((float)flick)/100.0);
				printf("line: %d. %d: check value: %f. %s\n", __LINE__, m_channel_1, 
						vcom_info->f_max_valid_flick, otp1_text);
			}
		}
	}

	// check channel 2 flick value.
	//��2·
	if(auto_flick_return_code_2 == FLICKER_OK)
	{
		int vcom = vcom_list[1];
		int flick = flick_list[1];
		printf("\nauto flick: channel=%d  vcom=%d  flick=%d\n", m_channel_2, vcom/*minIndex*/, flick/*minFlickValue*/);

		//���FLICKֵʧ��
		if(flick <= 0)
		{
			auto_flick_return_code_2 = FLICKER_NG_2;
			sprintf(otp2_text, "NG FLIC�豸����,�븴λ");
			//printf("%d--------------------------------- %s\n", m_channel_2, otp2_text);
			printf("line: %d. mipi_channel %d, %s\n", __LINE__, m_channel_2, otp2_text);
		}
		else
		{
			*vcomVcomValue2 = vcom;
			*vcomFlickValue2 = flick;
					
			if(vcom_info->f_max_valid_flick*100 > flick)
			{
				if(flick <= 70)
				{
					auto_flick_return_code_2 = FLICKER_NG_2;
					sprintf(otp2_text, "NG FLICK��%.1f�쳣,������!", ((float)flick)/100.0);
					printf("mipi channel %d: %s\n", m_channel_2, otp2_text);
					
				}
			}
			else //������¼
			{
				auto_flick_return_code_2 = FLICKER_NG_2;
				sprintf(otp2_text, "NG FLICK��%.1f,������!", ((float)flick)/100.0);
				printf("mipi channel %d: %s\n", m_channel_2, otp2_text);
			}
		}
	}

	printf("box_autoFlick over\n");
}

#define MIN_VCOM_FLICK_VALUE  200
void box_channelFlickTask(void *param)
{
#if 0
	int index = (int)param;
	int channel = channel_list[index];
	int lcd_channel = lcd_channel_list[index];
	int enable_read_vcom = 0;
	int enable_left = 0;
	int enable_right = 0;
	
	while(1)
	{
		sem_wait(&m_channel_flick_thread_start_sem[index]);
		if(box_autoFlick_started[index] == 0) 
			continue;
		printf("**********************************box_channelFlickTask***********[channel=%d][lcd=%d]\n", channel, lcd_channel);

		int retVal = 0;
		if (chip_is_helitai_8019() == 0)
		{
			ca310_stop_flick_mode(lcd_channel);
			usleep(100*1000);

			// check init vcom
			float f_flick = 0.00;
			retVal = ca310_start_flick_mode(lcd_channel);
			
			#if 1
			int find_vcom = 0;
			int find_vcom2 = 0;
			int find_flick = 0;
			
			struct timeval tv1 = { 0 };
			struct timeval tv2 = { 0 };
			gettimeofday(&tv1, NULL);
			
			// read vcom config.
			vcom_info_t* p_vcom_info = get_current_vcom_info();
			printf("VCOM Info: \n");
			printf("min_vcom: %d.\n", p_vcom_info->minvalue);
			printf("max_vcom: %d.\n", p_vcom_info->maxvalue);
			printf("init_vcom: %d.\n", p_vcom_info->initVcom);
			printf("read delay: %d.\n", p_vcom_info->readDelay);

			{
				//int min_ok_flick = 200;
				//int max_ok_flick = 1300;
				int min_ok_flick = p_vcom_info->f_ok_flick * 100;
				int max_ok_flick = p_vcom_info->f_max_valid_flick * 100;
				
				int flick = 0;
				z_flick_test(channel, lcd_channel, p_vcom_info->readDelay, p_vcom_info->initVcom, &flick);				

				ca310_capture_flick_data(lcd_channel, &f_flick);
				flick = f_flick * 100;
				
				printf("init vcom: %d, flick: %d.\n", p_vcom_info->initVcom, flick);
	
				int flick_erro = z_auto_flick_2(channel, lcd_channel, p_vcom_info->readDelay, 
												p_vcom_info->minvalue, p_vcom_info->maxvalue, 
												p_vcom_info->initVcom, &find_vcom, &find_flick, 1,
												p_vcom_info->initVcom, flick, 0, min_ok_flick, 0, 
												max_ok_flick, 0, 0, 0, 0);

				printf("z_auto_flick_2: end. find: vcom = %d, flick = %d.\n", find_vcom, find_flick);
				vcom_list[index] = find_vcom;
				flick_list[index] = find_flick;
			}

			gettimeofday(&tv2, NULL);
			int diff = ((tv2.tv_sec - tv1.tv_sec) * 1000 * 1000 + tv2.tv_usec - tv1.tv_usec) / 1000;
			printf("=== Auto Flick time: %dms.  tv1: %d.%d, tv2: %d.%d.	===\n", diff, tv1.tv_sec, tv1.tv_usec, tv2.tv_sec, tv2.tv_sec);
			#endif
		}
		
		//FLICK设备结束
		retVal = ca310_stop_flick_mode(lcd_channel);
		
		if(retVal == -1)
		{
			printf("channel %d lcd_autoFlickEnd return error. %d\n", channel, __LINE__);
		}

		//
		sem_post(&m_channel_flick_thread_sem[index]); //此次调节VCOM结束
	}
#endif
}

int get_lcd_channel_by_mipi_channel(int mipi_channel)
{
	int lcd_channel = 0;
	
	switch(mipi_channel)
	{
		case 1:
		case 2:
			lcd_channel = 1;
			break;

		case 3:
		case 4:
			lcd_channel = 2;
			break;
			
		default :
			lcd_channel = 1;
			break;
	}
}

void flick_test(int mipi_channel, int vcom, int delay)
{
	float f_flick = 0.00;
	int lcd_channel = get_lcd_channel_by_mipi_channel(mipi_channel);
	
	ic_mgr_write_vcom(mipi_channel, vcom);

	usleep(delay * 1000);

	int retVal = 0;
	ca310_capture_flick_data(lcd_channel, &f_flick);
	retVal = f_flick * 100;

	int lcd_flick_value = f_flick * 100;
	show_lcd_msg(mipi_channel, 1, 0, vcom, lcd_flick_value, 0, 0);

	// send flick data to client.
	client_flick_test_ack(mipi_channel, vcom, f_flick);
}

//手动调节FLICK
unsigned int m_flick_manual_vcom = 0;
unsigned int m_flick_manual_flick = 0;
static unsigned int m_flick_manual_last_time = 0;
void flick_manual(int vcomIndex)
{
	int channel = m_channel_1;
	int lcd_channel = 1;

	m_flick_manual_vcom = vcomIndex;

	ic_mgr_write_vcom(channel, vcomIndex);

	int last_flick = 0;
	//检查CA310串口是否打开
	if(lcd_getStatus(lcd_channel) != 0)
	{
		//通知FLICK设备, 准备开始FLICK操作�?
		ca310_stop_flick_mode(lcd_channel);
		usleep(1000*200);
		ca310_start_flick_mode(lcd_channel);

		int i;
		float f_flick = 0.00;
		
		for(i=0; i < 3; i++)
		{
			int retVal = 0;
			ca310_capture_flick_data(lcd_channel, &f_flick);
			retVal = f_flick * 100;
			
			if(retVal == -1) 
				printf("check lcd flick faild timout or ret failed\n"); //timeout
				
			if(last_flick == retVal)
			{
				break;
			}
			
			last_flick = retVal;
			usleep(1000*200);
		}

		//通知FLICK设备, 停止FLICK操作�?
		ca310_stop_flick_mode(lcd_channel);
	}
	
	m_flick_manual_flick = last_flick;

	show_lcd_msg(channel, 1, 0, vcomIndex, last_flick, 0, 0);
	
	printf("manual flick: vom: %d, flick: %d.\n", vcomIndex, last_flick);
	return ;
}

void flick_init()
{
    pthread_mutex_init(&flickLock,0);
}


