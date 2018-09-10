/*
 * pubPwrTask.h
 *
 *  Created on: 2018-7-24
 *      Author: tys
 */

#ifndef PUBPWRTASK_H_
#define PUBPWRTASK_H_

#include "common.h"
#include "comStruct.h"
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define POWER_CHECK_PERCENT		(4)
#define MIN_POWER_ADJUST		(200)
#define POWER_MIN_0_VALUE		(1500)

typedef struct pwrTaskInfo
{
	pthread_mutex_t mutex;
	bool bPwrUpdateStarted;
	bool bPwrUpdatePaused;
	pthread_t pwrUpdateTaskId;

	bool bNtcCheckStarted;
	bool bNtcCheckPaused;
	pthread_t ntcCheckTaskId;

	main_power_info_t pwrInfo[2];
}pwrTaskInfo_t;

#define PWR_UPGRADE_INFO_HEAD_LEN	(16)
typedef struct pwrUpgradeInfo
{
	unsigned char dataType;
	unsigned char devType;
	unsigned short len;
	unsigned int offset;
	unsigned int blockSize;
	unsigned int totalSize;
	void* payLoad;
}pwrUpgradeInfo_t;

typedef struct singlePwrInfo
{
	unsigned char isOn;
	unsigned char type;
	unsigned short volta;
}singlePwrInfo_t;

typedef struct calibrationPowerItem
{
	unsigned char reserved;
	unsigned char powerType;
	unsigned char action;
	unsigned char direction;
	unsigned int delta;
}calibrationPowerItem_t;

void printPowerInfo(sByPtnPwrInfo* p_power_inf);
void powerCmdCalibrationPowerItem(PWR_CHANNEL_E channel, int powerType, int action, int powerData);
void pwrSetVol(unsigned char isOn, unsigned char type, short volta);
void pwrUpgrade(unsigned char* pData, int length);
void pwrSetState(bool bOn, PWR_CHANNEL_E channel);
void pwrSetCfg(s1103PwrCfgDb* pCfg);
void pwrContorl(bool bOn, PWR_CHANNEL_E channel);
void getPwrInfo(PWR_CHANNEL_E channel);
void pwrTaskInit(void);
void pwrTaskDeinit(void);
void setNtcCheckPause(bool bPause);
#ifdef __cplusplus
}
#endif
#endif /* PUBPWRTASK_H_ */
