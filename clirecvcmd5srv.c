#include <stdlib.h>
#include "recvcmd.h"
#include "packsocket/packsocket.h"
#include "client.h"
#include "json/cJSON.h"
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include "rwini.h"
#include "util/debug.h"
#include "pgDB/pgLocalDB.h"
#include "common.h"
#include "pgDB/pgDB.h"
#include "vcom.h"
#include "fpgaFunc.h"

#include "net_general_command.h"
#include "mcu_spi.h"
#include "spi_sp6.h"
#include "clipg.h"

extern int m_channel_1; 
extern int m_channel_2;
extern int m_channel_3;
extern int m_channel_4;

static char achomeDir[128];


static recv_file_info_t* get_cur_file_info(void *infoSet)
{
    recv_file_info_t *pCur = 0;
    recv_file_info_set_t *recv_file_info_set = (recv_file_info_set_t*)infoSet;
	
    if(recv_file_info_set->curNo < recv_file_info_set->totalFilesNums)
    {
        pCur = &recv_file_info_set->pRecv_file_info[recv_file_info_set->curNo];
    }
	
    return pCur;
}

static void add_file_index(void *infoSet)
{
    recv_file_info_set_t *recv_file_info_set = (recv_file_info_set_t*)infoSet;
    recv_file_info_set->curNo ++;
}

typedef struct tag_SUpdataFileList
{
    char name[256];
    int filesize;
    int hasnext; //char hasnext; xujie 原始的没有对齐
} SUpdataFileList;

static unsigned char *tmpFirmwareBuf = 0; //buf
static unsigned char *tmpFirmwareWpt = 0; //writer ptr
static unsigned int s_total_data_len = 0;


//控制盒升级数据
static unsigned char *tmpBoxUpdateBuf = NULL; //buf
static unsigned int   tmpBoxUpdateSize = 0;
static unsigned char *tmpBoxUpdateWpt = NULL; //writer ptr

//电源升级数据
static unsigned char *tmpPowerUpdateBuf = NULL; //buf
static unsigned int   tmpPowerUpdateSize = 0;
static unsigned char *tmpPowerUpdateWpt = NULL; //writer ptr

char m_current_module_filename[256] = "";

int parse_gamma_reg_data(char* org_data, unsigned char* p_gamma_reg_data_buf, 
							int gamma_reg_data_buf_len)
{
	char* ptr = org_data;
	int len = 0;

	if (org_data == NULL)
		return 0;
	
	while(ptr[0])
	{
		char cc[20];
		cc[0] = ptr[0];
		cc[1] = ptr[1];
		cc[2] = 0;
		
		int val = 0;
		sscanf(cc, "%02X", &val);
		p_gamma_reg_data_buf[len] = (unsigned char)(val & 0xff);
		
		len ++;		
		ptr += 3;
	}

	return len;
}

int pg_do_box_home_proc()
{
	box_go_do_home_proc();
}

//处理pgServer发送给ARM的消息
int msgProc(socket_cmd_t *pSocketCmd)
{
    //DBG("msgProc %d",pSocketCmd->cmd);
    switch (pSocketCmd->cmd)
    {
        case SERVER2CLI_MSG_REGISTER:
        //客户端一旦登录成功，将返回时间和服务端的状态
        {
        	printf("=====: SERVER2CLI_MSG_REGISTER\n");
            cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
            cJSON *timeStamp = cJSON_GetObjectItem(root,"timeStamp");
            cJSON *homePath  = cJSON_GetObjectItem(root,"homePath");
            //DBG("::%s",pSocketCmd->pcmd);
            DBG("cur timeStamp is %d %s",timeStamp->valueint, homePath->valuestring);
            client_pg_setTime(timeStamp->valueint);
            strcpy(achomeDir,homePath->valuestring);
            cJSON_Delete(root);
        }
        break;

        case SERVER2CLI_MSG_SYNCFILE: //同步模组文件
        //客户端将比较文件，然后去下载文件
        {
        	printf("=====: SERVER2CLI_MSG_SYNCFILE\n");

        	if (client_pg_pwr_status_get())
        		client_pg_shutON(0, NULL, 0, 0);

            int i = 0;
            cJSON *item = NULL;
            cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
            cJSON *TotalFiles = cJSON_GetObjectItem(root,"TotalFiles");            
            cJSON *tstModleName = cJSON_GetObjectItem(root,"tstModleName");
			
            set_cur_module_name(tstModleName->valuestring);
            printf("Sync Files: module name: %s, file nums: %d.\n", tstModleName->valuestring, TotalFiles->valueint);
			
            cJSON *FilesArray   = cJSON_GetObjectItem(root, "FilesArray");
            int arraySize = cJSON_GetArraySize(FilesArray);
			
            recv_file_info_set_t *recv_file_info_set = (recv_file_info_set_t*)malloc(sizeof(recv_file_info_set_t));
            recv_file_info_set->curNo = 0;
            recv_file_info_set->totalFilesNums  = arraySize;
            recv_file_info_set->pRecv_file_info = (recv_file_info_t*)malloc(arraySize * sizeof(recv_file_info_t));
            recv_file_info_set->fn_recv_file = client_pg_syncFile;
            recv_file_info_set->param = 0;
            recv_file_info_set->fn_get_cur_file_info  = get_cur_file_info;
            recv_file_info_set->fn_add_file_index = add_file_index;
			
            recv_file_list_insert(recv_file_info_set);
            memset(recv_file_info_set->pRecv_file_info, 0, sizeof(recv_file_info_t) * arraySize);

			// parse file info array, save file info array into file_info_set.
            for(i = 0; i < arraySize; i ++)
            {
                item = cJSON_GetArrayItem(FilesArray,i);
                cJSON *FileName = cJSON_GetObjectItem(item, "FileName");
                cJSON *FileSize = cJSON_GetObjectItem(item, "FileSize");
                cJSON *FileTime = cJSON_GetObjectItem(item, "FileTime");
                DBG("FileName:%s FileSize:%d", FileName->valuestring, FileSize->valueint);
				
                recv_file_info_set->pRecv_file_info[i].rfiSize = FileSize->valueint;
                recv_file_info_set->pRecv_file_info[i].rfiTime = FileTime->valueint;
                strcpy(recv_file_info_set->pRecv_file_info[i].rfiName, FileName->valuestring);
            }
			
            if(client_getFileFromSet(recv_file_info_set) != 0)
            {
            	printf("All File is new, do nothing!\n");
				
                if(recv_file_info_set->param)
                {
                    free(recv_file_info_set->param);
					recv_file_info_set->param = NULL;
                }

				if (recv_file_info_set->pRecv_file_info)
				{
					free(recv_file_info_set->pRecv_file_info);
					recv_file_info_set->pRecv_file_info = NULL;
				}

				recv_file_list_del(recv_file_info_set);
                free(recv_file_info_set);
				recv_file_info_set = NULL;

				printf("File sync end! =====\n");
                client_syncFinish();

				// box state reset.
				printf("=========================   SYNC END ===================\n");
				pg_do_box_home_proc();
            }
			
            cJSON_Delete(root);
        }
        break;

        case SERVER2CLI_MSG_SENDFILE: //同步模组文件,  之后,   各个小文件下载
        //接收文件
        {
            recv_file_info_set_t *recv_file_info_set = (recv_file_info_set_t *)recv_file_list_get((void*)pSocketCmd->ipnum);
            recv_file_info_t *recv_file_info = recv_file_info_set->fn_get_cur_file_info(recv_file_info_set);
            if(pSocketCmd->type == BINFILE_BEGIN)
            {
                if(recv_file_info->pFileData)
                {
                    free(recv_file_info->pFileData);
                    recv_file_info->pFileData = NULL;
                }
				
                cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
                cJSON *FileName = cJSON_GetObjectItem(root, "FileName");				
                cJSON_Delete(root);
				
                recv_file_info->pFileData = (unsigned char*)malloc(recv_file_info->rfiSize);
                recv_file_info->actRecvSize = 0;
            }
            else if(pSocketCmd->type == BINFILE_BODY)
            {
            	//printf("recv file data: data ...\n");
                if(((recv_file_info->actRecvSize + pSocketCmd->len) <= recv_file_info->rfiSize)
						|| (recv_file_info->rfiSize == 0))
                {
                    memcpy(&recv_file_info->pFileData[recv_file_info->actRecvSize],
                        	pSocketCmd->pcmd, pSocketCmd->len);
					
                    recv_file_info->actRecvSize += pSocketCmd->len;
                    //DBG("ipnum %d",pSocketCmd->ipnum);
                }
                else
                {
                    printf("msgProc: SERVER2CLI_MSG_SENDFILE. recv file size error %d + %d > %d",
                           	recv_file_info->actRecvSize, pSocketCmd->len, recv_file_info->rfiSize);
                }
            }
            else
            {
                if( (recv_file_info->rfiSize == recv_file_info->actRecvSize)
						|| (recv_file_info->rfiSize == 0) )
                {
					// recv next file ...
                    recv_file_info_set->fn_recv_file(recv_file_info_set);
					
					free(recv_file_info->pFileData);
                    recv_file_info->pFileData = 0;
                }
                else
                {
                    DBG("recv file failed rfisize %d actSize %d type %x len %d",recv_file_info->rfiSize,
                        recv_file_info->actRecvSize,pSocketCmd->type,pSocketCmd->len);
                }
            }
        }
        break;

        case SERVER2CLI_MSG_SHUTON:
        //将PG开电
        {
        	printf("=====: SERVER2CLI_MSG_SHUTON\n");
            cJSON *root = NULL;
            do
            {
            	root = cJSON_Parse((char*)pSocketCmd->pcmd);
				if (NULL == root)
					break;

				cJSON* pElem = cJSON_GetObjectItem(root,"tstModleName");
				if (pElem)
					strcpy(m_current_module_filename, pElem->valuestring);
				else
					break;

				int ptn_id = 0;

				pElem = cJSON_GetObjectItem(root, "ptn_index");
				if (pElem)
					ptn_id = pElem->valueint;

				int channel = 1;

				pElem = cJSON_GetObjectItem(root, "channel");
				if (pElem)
					channel = pElem->valueint;

				printf("SERVER2CLI_MSG_SHUTON: ptn_id = %d.\n", ptn_id);
				client_pg_shutON(1, m_current_module_filename, channel, ptn_id);
            }while (0);

            if (root)
            	cJSON_Delete(root);
        }
        break;

        case SERVER2CLI_MSG_SHUTDWN:
        //将PG关电
        {
        	printf("=====: SERVER2CLI_MSG_SHUTDWN\n");
            client_pg_shutON(0, 0, 0xFF, 1);
			// clear power info.
			box_shutdown_clear_power_info();
        }
        break;

        case SERVER2CLI_MSG_SHOWPTN:
        //PG显示指定的PTN
        {
            cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
            cJSON *ptnId = cJSON_GetObjectItem(root,"ptnId");

            client_pg_showPtn(ptnId->valueint);
            cJSON_Delete(root);
        }
        break;

		case SERVER2CLI_MSG_CROSSCUR:
        //PG显示十字光标
        {
            pointer_xy_t *pstPointer;
            int i,pointerNum;
            cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
            pointerNum = cJSON_GetArraySize(root);
            pstPointer = (pointer_xy_t*)malloc(sizeof(pointer_xy_t)*pointerNum);
            for(i=0;i<pointerNum;i++)
            {
                cJSON *pxy = cJSON_GetArrayItem(root,i);
                if(pxy)
                {
                    cJSON *x= cJSON_GetObjectItem(pxy,"x");
                    cJSON *y= cJSON_GetObjectItem(pxy,"y");
                    pstPointer[i].x = x->valueint;
                    pstPointer[i].y = y->valueint;
                }
            }
            cJSON_Delete(root);
            client_pg_showCrossCur(pstPointer,pointerNum);
            free(pstPointer);
        }
        break;

        case SERVER2CLI_MSG_HIDECUR:
            client_pg_hideCrossCur();
            break;

        case SERVER2CLI_MSG_WRITEREG:
        //PG 写寄存器然后通知给 SERVER&SPONSOR
        {
        	printf("=====: SERVER2CLI_MSG_WRITEREG\n");
            cJSON *root   = cJSON_Parse((char*)pSocketCmd->pcmd);
            cJSON *regAddr= cJSON_GetObjectItem(root,"regAddr");
            cJSON *regVal = cJSON_GetObjectItem(root,"regVal");
            fpga_write(regAddr->valueint,regVal->valueint);
            cJSON_Delete(root);
            break;
        }

        case SERVER2CLI_MSG_SYNCTIME:
        //PG 同步当前的时间与上位机保持一致
        {
        	printf("=====: SERVER2CLI_MSG_SYNCTIME\n");
            cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
            cJSON *timeStamp= cJSON_GetObjectItem(root,"timeStamp");
            client_pg_setTime(timeStamp->valueint);
            cJSON_Delete(root);
        }
        break;

        case SERVER2CLI_MSG_UPDATEFILE:
        // PG 去下载对应的升级文件，并把下载和更新的状态通知给上位机
        {
        	printf("=====: SERVER2CLI_MSG_UPDATEFILE\n");
            cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
            cJSON *srcFilePath= cJSON_GetObjectItem(root,"srcFilePath");
            cJSON *fileType= cJSON_GetObjectItem(root,"fileType");
            cJSON *dstFilePath= cJSON_GetObjectItem(root,"dstFilePath");
            DBG("src %s type %s dst %d",srcFilePath->valuestring,fileType->valuestring,
                dstFilePath->valuestring);

            recv_file_info_set_t *recv_file_info_set = (recv_file_info_set_t*)malloc(sizeof(recv_file_info_set_t));
            recv_file_info_set->curNo = 0;
            recv_file_info_set->totalFilesNums = 1;
            recv_file_info_set->fn_get_cur_file_info = get_cur_file_info;
            recv_file_info_set->fn_add_file_index = add_file_index;
            recv_file_info_set->pRecv_file_info = (recv_file_info_t*)malloc(sizeof(recv_file_info_t));
			
            recv_file_info_t *recv_file_info = recv_file_info_set->pRecv_file_info;
            memset(recv_file_info,0,sizeof(recv_file_info_t));
			
            strcpy(recv_file_info->rfiName,srcFilePath->valuestring);
            recv_file_info_set->fn_recv_file = client_pg_downFile;
			
            dwn_file_info_t *dwn_file_info = (dwn_file_info_t*)malloc(sizeof(dwn_file_info_t));
            strcpy(dwn_file_info->srcFileName,srcFilePath->valuestring);
            strcpy(dwn_file_info->srcFileType,fileType->valuestring);
            strcpy(dwn_file_info->dstFileName,dstFilePath->valuestring);
			
            recv_file_info_set->param = dwn_file_info;
            client_getFile(recv_file_info->rfiName,recv_file_info_set);
            cJSON_Delete(root);
        }
        break;

        case SERVER2CLI_MSG_COMPLETESYNC: //完全同步测试文件, 没有清空ARM的所有cfg配置文件
        {
			printf("=====: SERVER2CLI_MSG_COMPLETESYNC\n");
            int i;
            cJSON *item;
            cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
            cJSON *TotalFiles = cJSON_GetObjectItem(root,"TotalFiles");
            cJSON *tstModleName = cJSON_GetObjectItem(root,"tstModleName");
			
            set_cur_module_name(tstModleName->valuestring);
            DBG("TotalFiles is %d %s",TotalFiles->valueint,tstModleName->valuestring);
			
            cJSON *FilesArray = cJSON_GetObjectItem(root, "FilesArray");
            int arraySize = cJSON_GetArraySize(FilesArray);
            recv_file_info_set_t *recv_file_info_set = (recv_file_info_set_t*)malloc(sizeof(recv_file_info_set_t));
            recv_file_info_set->curNo = 0;
            recv_file_info_set->totalFilesNums  = arraySize;
            recv_file_info_set->pRecv_file_info = (recv_file_info_t*)malloc(arraySize*sizeof(recv_file_info_t));
            recv_file_info_set->fn_recv_file = client_pg_syncFile;
            recv_file_info_set->param = 0;
            recv_file_info_set->fn_get_cur_file_info  = get_cur_file_info;
            recv_file_info_set->fn_add_file_index = add_file_index;
            recv_file_list_insert(recv_file_info_set);
			
            memset(recv_file_info_set->pRecv_file_info,0,sizeof(recv_file_info_t) * arraySize);
            for(i = 0; i < arraySize; i ++)
            {
                item = cJSON_GetArrayItem(FilesArray,i);
                cJSON *FileName = cJSON_GetObjectItem(item,"FileName");
                cJSON *FileSize = cJSON_GetObjectItem(item,"FileSize");
                cJSON *FileTime = cJSON_GetObjectItem(item,"FileTime");
                DBG("FileName:%s FileSize:%d",FileName->valuestring,FileSize->valueint);
				
                recv_file_info_set->pRecv_file_info[i].rfiSize = FileSize->valueint;
                recv_file_info_set->pRecv_file_info[i].rfiTime = FileTime->valueint;
                strcpy(recv_file_info_set->pRecv_file_info[i].rfiName, FileName->valuestring);
            }
			cJSON_Delete(root);
			
			system("rm -rf ./cfg/*"); //xujie
            localDBDelAll();
			
            if(client_getFileFromSet(recv_file_info_set)!= 0)
            {
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
				printf("=========================   SYNC END 2 ===================\n");

				pg_do_box_home_proc();
            }
            
            break;
        }
		
        case SERVER2CLI_MSG_TSTTIMON:
        {
			printf("=====: SERVER2CLI_MSG_TSTTIMON.    ===== do nothing ====\n");
        }
        break;
		
        case SERVER2CLI_MSG_TSTTIMDWN:
        {
			printf("=====: SERVER2CLI_MSG_TSTTIMDWN.    ===== do nothing ====\n");
        }
        break;

        case SERVER2CLI_MSG_GETPWRON:
        {
			printf("=====: SERVER2CLI_MSG_GETPWRON. \n");
        }
        break;
		
        case SERVER2CLI_MSG_GETPWRDWN:
        {
			printf("=====: SERVER2CLI_MSG_GETPWRDWN. \n");
        }
        break;
		
        case SERVER2CLI_MSG_AUTOFLICK: //自动FLICK
        {
			printf("=====: SERVER2CLI_MSG_AUTOFLICK. \n");
            cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
            cJSON *enableFlick = cJSON_GetObjectItem(root,"enableFlick");

			int vcom_ptn_index = get_current_vcom_ptn_index();
			if (vcom_ptn_index >= 0)
			{
				printf("switch flick picture: %d.\n", vcom_ptn_index);
				client_pg_showPtn(vcom_ptn_index);
			}
			
            flick_auto(enableFlick->valueint,0);
            cJSON_Delete(root);

			printf("SERVER2CLI_MSG_AUTOFLICK End.\n");
        }
        break;

        case SERVER2CLI_MSG_MANUALFLICK: //手动FLICK
        {
			printf("=====: SERVER2CLI_MSG_MANUALFLICK. \n");
            cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
			cJSON *json_mipi_channel = cJSON_GetObjectItem(root, "channel");
            cJSON *flickValue = cJSON_GetObjectItem(root,"flickValue");
			{
        		int mipi_channel = 1;
				
				if (json_mipi_channel)
					mipi_channel = json_mipi_channel->valueint;

				qc_set_vcom_value(mipi_channel, flickValue->valueint);
        	}
			
            //flick_manual(flickValue->valueint);
			printf("=====: SERVER2CLI_MSG_MANUALFLICK.  %d  END.\n", flickValue->valueint);
            cJSON_Delete(root);
        }
        break;

        case SERVER2CLI_MSG_FIRMWAREUPDATA: //固件升级
        {
        	printf("=====: SERVER2CLI_MSG_FIRMWAREUPDATA. \n");
           	if(pSocketCmd->type == BINFILE_BEGIN)
            {
                cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
                cJSON *FileName = cJSON_GetObjectItem(root, "firmwareFileName");
                cJSON *FileSize = cJSON_GetObjectItem(root, "fileSize");
				
				if( tmpFirmwareBuf != NULL )
                {
                    free(tmpFirmwareBuf);
                    tmpFirmwareBuf = NULL;
                    tmpFirmwareWpt = NULL;
                }
				
                tmpFirmwareBuf = (unsigned char*)malloc(FileSize->valueint);
                tmpFirmwareWpt = tmpFirmwareBuf;
                printf("SERVER2CLI_MSG_FIRMWAREUPDATA: begin. filesize: %u.\n", FileSize->valueint);
                cJSON_Delete(root);

				s_total_data_len = 0;
            }
            else if(pSocketCmd->type == BINFILE_BODY)
            {
                if( tmpFirmwareWpt == NULL )
                {
                    DBG("tmpFirmwareWpt == 0. no send BINFILE_BEGIN");
                    break;
                }

                memcpy(tmpFirmwareWpt, pSocketCmd->pcmd, pSocketCmd->len);
                tmpFirmwareWpt += pSocketCmd->len;
				s_total_data_len += pSocketCmd->len;
				//printf("body: recv len = %u, total len = %u.\n", pSocketCmd->len, s_total_data_len);
            }
            else
            {
                if( tmpFirmwareBuf == 0 )
                {
                    DBG("tmpFirmwareBuf == 0. no send BINFILE_BEGIN");
                    break;
                }

				printf("SERVER2CLI_MSG_FIRMWAREUPDATA: over. recv size: %u.\n", s_total_data_len);
				
                SUpdataFileList *pufl = (SUpdataFileList*)tmpFirmwareBuf;
                SUpdataFileList *poldufl = pufl;
				
                do{
					//printf("=== next file ====.\n");
                    recv_file_info_set_t *recv_file_info_set = (recv_file_info_set_t*)malloc(sizeof(recv_file_info_set_t));
                    recv_file_info_set->curNo = 0;
                    recv_file_info_set->totalFilesNums  = 1;                    
                    recv_file_info_set->fn_recv_file = client_pg_firmware;
                    recv_file_info_set->param = 0;
                    recv_file_info_set->fn_get_cur_file_info  = get_cur_file_info;
                    recv_file_info_set->fn_add_file_index = add_file_index;

					// fill recv file info.
					recv_file_info_set->pRecv_file_info = (recv_file_info_t*)malloc(sizeof(recv_file_info_t));
                    memcpy(recv_file_info_set->pRecv_file_info->rfiName, pufl->name, strlen(pufl->name) + 1);
                    recv_file_info_set->pRecv_file_info->rfiSize = pufl->filesize;

					// copy file data.
                    recv_file_info_set->pRecv_file_info->pFileData = (unsigned char*)malloc(pufl->filesize);					
                    unsigned char* pdata = (unsigned char*)pufl + sizeof(SUpdataFileList);
                    memcpy(recv_file_info_set->pRecv_file_info->pFileData, pdata, pufl->filesize);

					// save file data.
                    recv_file_info_set->fn_recv_file(recv_file_info_set);
                    printf("Upgrade file: %s.\n", recv_file_info_set->pRecv_file_info->rfiName);
                    
                    free(recv_file_info_set->pRecv_file_info->pFileData);
                    free(recv_file_info_set->pRecv_file_info);
                    free(recv_file_info_set);

                    poldufl = pufl;
                    char *poffer = (char*)pufl;

					printf("upgrade image size: %d.\n", pufl->filesize);
                    poffer += pufl->filesize + sizeof(SUpdataFileList);
                    pufl = (SUpdataFileList*)poffer;
					
                }while( poldufl->hasnext == 0x01 );

                DBG("SERVER2CLI_MSG_FIRMWAREUPDATA end");

				int upgrade_type = 0;
				int upgrade_state = 0;
				client_end_firmware_upgrade(upgrade_type, upgrade_state);

				char cmd_buf[1024] = "";
				sprintf(cmd_buf, "%s %s", "/home/updatafirmware.sh", poldufl->name);
				printf("cmd: %s\n", cmd_buf);
				system(cmd_buf);

				if( tmpFirmwareBuf != NULL )
                {
                    free(tmpFirmwareBuf);
                    tmpFirmwareBuf = NULL;
                    tmpFirmwareWpt = NULL;
                }
            }
        }
        break;

        case SERVER2CLI_MSG_SYNCMODE:
        {
			printf("=====: SERVER2CLI_MSG_SYNCMODE. \n");
            cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
            cJSON *modeName = cJSON_GetObjectItem(root, "modeName");
            cJSON_Delete(root);
        }
        break;

        case SERVER2CLI_MSG_FLICKVCOM:
        {
			printf("=====: SERVER2CLI_MSG_FLICKVCOM. \n");
        }
        break;

        case SERVER2CLI_MSG_REALTIMECONTROL:
        {
			printf("=====: SERVER2CLI_MSG_REALTIMECONTROL. \n");
        }
        break;

		case SERVER2CLI_MSG_BOXUPDATA: //控制盒升级
		{
			if(pSocketCmd->type == BINFILE_BEGIN)
			{
				printf("=====: SERVER2CLI_MSG_BOXUPDATA. BINFILE_BEGIN\n");
				DBG("pSocketCmd->type == BINFILE_BEGIN");
				if( tmpBoxUpdateBuf != NULL )
				{
					free(tmpBoxUpdateBuf);
					tmpBoxUpdateBuf = tmpBoxUpdateWpt = NULL;
					tmpBoxUpdateSize = 0;
				}

				//解析得到文件大小
				cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
				//cJSON *FileName = cJSON_GetObjectItem(root,"firmwareFileName");
				cJSON *FileSize = cJSON_GetObjectItem(root, "fileSize");
				tmpBoxUpdateSize = FileSize->valueint;
				cJSON_Delete(root);

				//分配内存
				tmpBoxUpdateBuf = (unsigned char*)malloc(tmpBoxUpdateSize);
				DBG("upgrade data: %p, size: %d. \n", tmpBoxUpdateBuf, tmpBoxUpdateSize);
				tmpBoxUpdateWpt = tmpBoxUpdateBuf;
				//DBG("tmpBoxUpdateWpt %d", tmpBoxUpdateWpt);
			}
			else if(pSocketCmd->type == BINFILE_BODY) //升级文件数据块
			{
				printf("=====: SERVER2CLI_MSG_BOXUPDATA. BINFILE_BODY %d\n", pSocketCmd->len);
				if( tmpBoxUpdateWpt == 0 )
				{
					DBG("tmpBoxUpdateWpt == 0. no send BINFILE_BEGIN");
					break;
				}
				
				memcpy(tmpBoxUpdateWpt, pSocketCmd->pcmd, pSocketCmd->len);
				tmpBoxUpdateWpt += pSocketCmd->len;
			}
			else //升级文件接收结束
			{
				printf("=====: SERVER2CLI_MSG_BOXUPDATA. BINFILE_OVER\n");
				if( tmpBoxUpdateBuf == 0 )
				{
					printf("Error: tmpBoxUpdateBuf == NULL. \n");
					break;
				}
				
				//保存升级文件
				unsigned int u32QueueId = box_message_queue_get();
				//MsOS_PostMessage(u32QueueId, 0x12345678, (MS_U32)tmpBoxUpdateBuf, tmpBoxUpdateSize);
				MsOS_SendMessage(u32QueueId, 0x12345678, (MS_U32)tmpBoxUpdateBuf, tmpBoxUpdateSize);

				if (tmpBoxUpdateBuf)
					free(tmpBoxUpdateBuf);
				
				tmpBoxUpdateBuf = tmpBoxUpdateWpt = NULL;
				tmpBoxUpdateSize = 0;

				printf("==== upgrade end ===\n");

				int upgrade_type = 1;
				int upgrade_state = 0;
				client_end_firmware_upgrade(1, 0);
			}
		}
		break;

		case SERVER2CLI_MSG_POWERUPDATA: //电源升级 2017-4-24
		{
			//printf("=====: SERVER2CLI_MSG_POWERUPDATA. \n");
			if(pSocketCmd->type == BINFILE_BEGIN)
			{
				printf("=====: SERVER2CLI_MSG_POWERUPDATA = BINFILE_BEGIN\n");
				DBG("pSocketCmd->type == BINFILE_BEGIN");

				//释放之前旧的升级数据
				if( tmpPowerUpdateBuf != NULL )
				{
					free(tmpPowerUpdateBuf);
					tmpPowerUpdateBuf = tmpPowerUpdateWpt = NULL;
					tmpPowerUpdateSize = 0;
				}

				//解析得到文件大小
				cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
				//cJSON *FileName = cJSON_GetObjectItem(root,"firmwareFileName");
				cJSON *FileSize = cJSON_GetObjectItem(root, "fileSize");
				tmpPowerUpdateSize = FileSize->valueint;
				cJSON_Delete(root);

				//分配内存
				tmpPowerUpdateBuf = (unsigned char*)malloc( tmpPowerUpdateSize );
				DBG("tmpPowerUpdateBuf = %x  tmpPowerUpdateSize = %d", tmpPowerUpdateBuf, tmpPowerUpdateSize);
				tmpPowerUpdateWpt = tmpPowerUpdateBuf;
				DBG("tmpPowerUpdateWpt %x", tmpPowerUpdateWpt);
			}
			else if(pSocketCmd->type == BINFILE_BODY) //升级文件数据块
			{
				printf("=====: SERVER2CLI_MSG_POWERUPDATA = BINFILE_BODY %d\n", pSocketCmd->len);
				if( tmpPowerUpdateWpt == 0 )
				{
					DBG("tmpPowerUpdateWpt == 0. no send BINFILE_BEGIN");
					break;
				}
				
				memcpy(tmpPowerUpdateWpt, pSocketCmd->pcmd, pSocketCmd->len);
				tmpPowerUpdateWpt += pSocketCmd->len;
			}
			else //升级文件接收结束
			{
				printf("=====: SERVER2CLI_MSG_POWERUPDATA = BINFILE_OVER\n");
				if( tmpPowerUpdateBuf == 0 )
				{
					DBG("tmpPowerUpdateBuf == 0. no send BINFILE_BEGIN");
					break;
				}
				
				//通知电源模块升级
				pwrUpgrade(tmpPowerUpdateBuf, tmpPowerUpdateSize);
				//清除状态信息
				if (tmpPowerUpdateBuf)
					free(tmpPowerUpdateBuf);
				
				tmpPowerUpdateBuf = tmpPowerUpdateWpt = NULL;
				tmpPowerUpdateSize = 0;

				int upgrade_type = 2;
				int upgrade_state = 0;
				client_end_firmware_upgrade(upgrade_type, upgrade_state);
			}
		}
		break;

		// capture x, y, lv data.
		case SERVER2CLI_MSG_RGBW:
		{
			cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
			cJSON *jsonChannel = cJSON_GetObjectItem(root, "channel");
			cJSON *jsonRGBW = cJSON_GetObjectItem(root, "rgbw");
			int channel = jsonChannel->valueint;
			int rgbw = jsonRGBW->valueint;		
			cJSON_Delete(root);
			
			printf("=====: SERVER2CLI_MSG_RGBW. channel %d, r[%3d] g[%3d] b[%3d]\n", channel, (rgbw>>16)&0xff, (rgbw>>8)&0xff, rgbw&0xff);

			//设置FPGA
			showRgbInfo_t showRgb;
			showRgb.rgb_r = (rgbw >> 16) & 0xff;
			showRgb.rgb_g = (rgbw >> 8) & 0xff;
			showRgb.rgb_b = (rgbw) & 0xff;
			fpgaShowRgb(&showRgb);
			usleep(300 * 1000);

			double x = 0.1;
			double y = 0.1;
			double Lv = 0.1;
			int lcd_channel = 1;

			ca310_capture_xylv_data(lcd_channel, &x, &y, &Lv);
			printf("channel %d, RGB[%3d:%3d:%3d], x: %f, y: %f, Lv: %f.\n", channel, (rgbw>>16)&0xff, (rgbw>>8)&0xff, 
						rgbw&0xff, x, y, Lv);
			//回应PG上位机, 此操作结束
			client_end_rgbw(channel, rgbw, x, y, Lv);
		}
		break;

		// write gamma reg data.
		case SERVER2CLI_MSG_GAMMA_WRITE_REG:
		printf("====== Write GAMMA REG DATA ===========\n");
		{
			#define GAMMA_MAX_REG_DATA_LEN	(255)
			unsigned char reg_data_buf[GAMMA_MAX_REG_DATA_LEN] = { 0 };
			int reg_data_buf_len = GAMMA_MAX_REG_DATA_LEN;
			
			cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
			cJSON *json_channel = cJSON_GetObjectItem(root, "channel");
			cJSON *json_write_type = cJSON_GetObjectItem(root, "write_type");
			int channel = json_channel->valueint;

			// default type: analog gamma.
			int write_type = 0;
			if (json_write_type)
				write_type = json_write_type->valueint;

			if (write_type == 0)
			{
				// save new analog gamma
				cJSON *json_data = cJSON_GetObjectItem(root, "data");
				if(json_data != NULL)
				{
					reg_data_buf_len = parse_gamma_reg_data(json_data->valuestring, reg_data_buf, reg_data_buf_len);
					printf("new analog gamma data len: %d.\n", reg_data_buf_len);
					dump_data1(reg_data_buf, reg_data_buf_len);

					// write new analog gamma reg data.
					if (reg_data_buf_len > 0)
					{
						ic_mgr_write_analog_gamma_data(channel, reg_data_buf, reg_data_buf_len);
						ic_mgr_write_fix_digital_gamma_data(channel);
					}
				}
			}
			else if (write_type == 1)
			{
				// save new digital gamma.
				cJSON *json_data = cJSON_GetObjectItem(root, "data");
				if(json_data != NULL)
				{
					reg_data_buf_len = parse_gamma_reg_data(json_data->valuestring, reg_data_buf, reg_data_buf_len);
					printf("new digital gamma data len: %d.\n", reg_data_buf_len);
					dump_data1(reg_data_buf, reg_data_buf_len);

					// write new analog gamma reg data.
					if (reg_data_buf_len > 0)
					{
						ic_mgr_write_digital_gamma_data(channel, reg_data_buf, reg_data_buf_len);
					}
				}
			}
			else if (write_type == 2)
			{
			}
			
			//回应PG上位机, 此操作结束
			client_end_gamma_write(channel, write_type);
		}
		break;

		// read gamma reg data.
		case SERVER2CLI_MSG_READ_GAMMA:
		{	
			#define GAMMA_MAX_REG_DATA_LEN1	(255)
			unsigned char reg_data_buf[GAMMA_MAX_REG_DATA_LEN1] = { 0 };
			int reg_data_buf_len = GAMMA_MAX_REG_DATA_LEN1;

			unsigned char read_buffer[GAMMA_MAX_REG_DATA_LEN1] = { 0 };
			int read_data_len = GAMMA_MAX_REG_DATA_LEN1;
			
			cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
			cJSON *json_channel = cJSON_GetObjectItem(root, "channel");
			cJSON *json_read_type = cJSON_GetObjectItem(root, "read_type");
			int channel = json_channel->valueint;

			// default read type: analog gamma.
			int read_type = 0;	
			if (json_read_type)
				read_type = json_read_type->valueint;
			
			cJSON *json_data = cJSON_GetObjectItem(root, "data");
			if(json_data != NULL)
			{
				reg_data_buf_len = parse_gamma_reg_data(json_data->valuestring, reg_data_buf, reg_data_buf_len);
				printf("new digital gamma data len: %d.\n", reg_data_buf_len);
				dump_data1(reg_data_buf, reg_data_buf_len);

				unsigned char otp_times = 0;

				// load gamma regdata from config.				
				// get init analog gamma config.
				gamma_cfg_t* p_gamma_cfg = get_current_gamma_cfg();

				ic_mgr_d3g_control(channel, 0);
				
				printf("enable gamma: %d.\n", p_gamma_cfg->enable_gamma_reg);

				if (reg_data_buf_len > 0)
				{					
					printf("write e0_ok data!\n");
					ic_mgr_write_analog_gamma_data(channel, reg_data_buf, reg_data_buf_len);
				}
				else if (p_gamma_cfg->enable_gamma_reg)
				{
					printf("write gamma cfg data\n");
					ic_mgr_write_analog_gamma_data(channel, p_gamma_cfg->gamma_reg, p_gamma_cfg->gamma_reg_nums);
				}
				else
				{
					ic_mgr_write_fix_analog_gamma_data(channel);
				}
				
				// read analog gamma value.
				read_data_len = ic_mgr_read_analog_gamma_data(channel, read_buffer, read_data_len);

				printf("read alanog reg len: 0x%02x.\n", read_data_len);
				dump_data1(read_buffer, read_data_len);
				ic_mgr_read_chip_vcom_otp_times(channel, &otp_times);
				client_end_read_gamma(channel, read_data_len, read_buffer, otp_times, 
										otp_times, read_type);				
			}
			
			cJSON_Delete(root);
			printf("=====: SERVER2CLI_MSG_READ_GAMMA. channel %d\n", channel);
		}
		break;

		// burn gamma reg data: analog gamma or digital gamma.
		case SERVER2CLI_MSG_GAMMA_OTP:
		{
			int i = 0;
			int j = 0;
			int check_error = 0;
			int vcom_burn = 0;
			int gamma_burn_2nd = 0;
			
			cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
			cJSON *json_channel = cJSON_GetObjectItem(root, "channel");
			cJSON *json_otp_flag = cJSON_GetObjectItem(root, "otp_flag");
			int channel = json_channel->valueint;
			int otp_flag = json_otp_flag ? json_otp_flag->valueint : 0x00;
			cJSON_Delete(root);
			
			printf("=====: SERVER2CLI_MSG_GAMMA_OTP. channel %d, otp_flag: %x\n", channel, otp_flag);

			if (otp_flag == 0)
			{
				check_error = -1;
				printf("SERVER2CLI_MSG_GAMMA_OTP: otp flag is %d.\n", otp_flag);
				client_end_gamma_otp_info(channel, check_error, 0, 0, 0, 0, 0);
				break;
			}
			else
			{
				if (otp_flag & 0x02)
				{
					vcom_burn = 1;
				}

				if (otp_flag & 0x08)
				{
					gamma_burn_2nd = 1;
				}
			}
			
			int err = 0;
			
			unsigned int read_vcom = 0;
			unsigned char otp_times = 0;
			int check_flick = 0;
			double f_check_x = 0;
			double f_check_y = 0;
			double f_check_Lv = 0;

			int vcom = 0;
			int flick = 0;
			mipi_channel_get_vcom_and_flick(channel, &vcom, &flick);

			// get analog gamma reg data
			unsigned char analog_gamma_reg_data[255] = { 0 };
			int analog_gamma_reg_data_len = 255;

			// get digital gamma reg data.
			unsigned char d3g_r_reg_data[255] = { 0 };
			int d3g_r_reg_data_len = 255;
			unsigned char d3g_g_reg_data[255] = { 0 };
			int d3g_g_reg_data_len = 255;
			unsigned char d3g_b_reg_data[255] = { 0 };
			int d3g_b_reg_data_len = 255;

			printf("====  get vcom value: %d, 0x%02x. \n", vcom, vcom);
			ic_mgr_read_chip_vcom_otp_times(channel, &otp_times);
			ic_mgr_get_analog_gamma_reg_data(channel, analog_gamma_reg_data, &analog_gamma_reg_data_len);
			ic_mgr_get_digital_gamma_reg_data_t(channel, d3g_r_reg_data, &d3g_r_reg_data_len, 
												d3g_g_reg_data, &d3g_g_reg_data_len, 
												d3g_b_reg_data, &d3g_b_reg_data_len);
			ic_mgr_burn_gamma_otp_values(channel, gamma_burn_2nd, vcom_burn, vcom, 
										analog_gamma_reg_data, analog_gamma_reg_data_len, 
										d3g_r_reg_data, d3g_r_reg_data_len, 
										d3g_g_reg_data, d3g_g_reg_data_len,
										d3g_g_reg_data, d3g_g_reg_data_len
										);
			
			usleep(300 * 1000);
			client_pg_shutON(1, m_current_module_filename, 0xFF, 1);

			// check 
			check_error = ic_mgr_check_gamma_otp_values(channel, vcom_burn, vcom, otp_times, 
											analog_gamma_reg_data, analog_gamma_reg_data_len, 
											d3g_r_reg_data, d3g_r_reg_data_len, 
											d3g_g_reg_data, d3g_g_reg_data_len,
											d3g_b_reg_data, d3g_b_reg_data_len);
			
			// set flick pic
			client_pg_showPtn(0); 
			usleep(300 * 1000);
			
			// capture flick value.
			int lcd_channel = 1;			
			if(lcd_getStatus(lcd_channel))
			{
				float f_flick = 0.00;
				ca310_start_flick_mode(lcd_channel);
				usleep(1000*100);
				ca310_capture_flick_data(lcd_channel, &f_flick);
				usleep(1000*10);
				ca310_stop_flick_mode(lcd_channel);
			}

			printf("check flick: %d.\n", check_flick);

			// set white photo.			
			showRgbInfo_t showRgb;
			int rgbw = 0xFFFFFF;
			showRgb.rgb_r = (rgbw >> 16) & 0xff;
			showRgb.rgb_g = (rgbw >> 8) & 0xff;
			showRgb.rgb_b = (rgbw) & 0xff;
			fpgaShowRgb(&showRgb);
			
			// capture white point x, y, lv.
			ca310_start_xylv_mode(lcd_channel);
			usleep(300 * 1000);
			ca310_capture_xylv_data(lcd_channel, &f_check_x, &f_check_y, &f_check_Lv);
			ca310_stop_xylv_mode(lcd_channel);
			
			printf("check white point: x: %f, y: %f, lv: %f.\n", f_check_x, f_check_y, f_check_Lv);

			if (check_error)
			{
				set_fpga_text_color(channel, 1);
				set_fpga_text_show(0, 1, 0, 0);
			}
			else
			{
				set_fpga_text_color(channel, 0);
				set_fpga_text_show(0, 1, 1, 0);
			}
			
			// OTP end ack.
			//client_end_gamma_otp(channel, "烧录成功!");
			int is_ok = check_error ? 0 : 1;
			client_end_gamma_otp_info(channel, check_error, read_vcom, check_flick, 
										f_check_x, f_check_y, f_check_Lv);
		}
		break;

		// ca310 start work.
		case SERVER2CLI_MSG_CA310_START:
		{
			cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
			cJSON *json_lcd_channel = cJSON_GetObjectItem(root, "lcd_channel");
			cJSON *json_mode = cJSON_GetObjectItem(root, "mode");
			int lcd_channel = json_lcd_channel->valueint;
			int mode = json_mode->valueint;
			cJSON_Delete(root);
			printf("=====: SERVER2CLI_MSG_CA310_START %d\n", lcd_channel);

			
			if(mode == 0) //xyLv
			{
				ca310_start_xylv_mode(lcd_channel);
			}
			else if(mode == 1) //Flicker
			{
				ca310_start_flick_mode(lcd_channel);
			}
			//usleep(1000 * 200);
		}
		break;

		// ca310 stop work.
		case SERVER2CLI_MSG_CA310_STOP:
		{
			cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
			cJSON *json_lcd_channel = cJSON_GetObjectItem(root, "lcd_channel");
			int lcd_channel = json_lcd_channel->valueint;
			cJSON_Delete(root);
			printf("=====: SERVER2CLI_MSG_CA310_STOP %d\n", lcd_channel);

			ca310_stop_flick_mode(lcd_channel);
		}
		break;

		case SERVER2CLI_MSG_LCD_READ_REG:
		{
			cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
			cJSON *json_channel = cJSON_GetObjectItem(root, "channel");
			cJSON *json_read_reg = cJSON_GetObjectItem(root, "reg");
			cJSON *json_read_len = cJSON_GetObjectItem(root, "len");
			
			int channel = json_channel ? json_channel->valueint : 1;
			unsigned char read_reg = json_read_reg ? json_read_reg->valueint : 0;
			unsigned char read_len = json_read_len ? json_read_len->valueint : 1;

			#define READ_BUFFER_LEN		(1024)
			unsigned char read_buffer[READ_BUFFER_LEN] = { 0 };
			int real_read_len = 0;

			read_buffer[0] = 0;	// we should read data from ic
			real_read_len = 1;
			
			printf("lcd read reg: 0x%02x, len: %d.\n", read_reg, real_read_len);
			dump_data1(read_buffer, real_read_len);

			// send read data to pc.
			client_lcd_read_reg_ack(channel, real_read_len, read_buffer);
			
			cJSON_Delete(root);
		}
		break;

		case SERVER2CLI_MSG_LCD_WRITE_CODE:
		{
			cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
			cJSON *json_channel = cJSON_GetObjectItem(root, "channel");
			cJSON *json_code = cJSON_GetObjectItem(root, "code");
			
			int channel = json_channel ? json_channel->valueint : 1;
			char* code = json_code ? json_code->valuestring : NULL;

			if (code)
			{
				unsigned char temp_mipi_code[1024 * 4];
				memset(temp_mipi_code, 0xff, sizeof(temp_mipi_code));
//		        mipi_write_init_code("/tmp/mipi", code, strlen(code));

				/* we should parse code and send the code to ic by spiCtrl
		        or i2cCtrl for testing.*/
			}
			
			cJSON_Delete(root);
		}
		break;

		case SERVER2CLI_MSG_PHOTO_DEBUG:
		{
			int photo_type = 0;
			int photo_value = 0xFFFFFF;
			
			cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
			cJSON *json_channel = cJSON_GetObjectItem(root, "channel");
			cJSON *json_photo_type = cJSON_GetObjectItem(root, "photo_type");
			cJSON *json_photo_value = cJSON_GetObjectItem(root, "photo_value");
			photo_type = json_photo_type->valueint;
			photo_value = json_photo_value->valueint;
			cJSON_Delete(root);

			showRgbInfo_t showRgb;
			showRgb.rgb_r = (photo_value >> 16) & 0xff;
			showRgb.rgb_g = (photo_value >> 8) & 0xff;
			showRgb.rgb_b = (photo_value) & 0xff;
			fpgaShowRgb(&showRgb);
		}
		break;
		
		case SERVER2CLI_MSG_MIPI_CHANNEL_MTP:
		{
			int mipi_channel = 0;
			int mtp_on = 0;
			cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
			cJSON *json_channel = cJSON_GetObjectItem(root, "channel");
			cJSON *json_mtp_on = cJSON_GetObjectItem(root, "mtp_on");
			mipi_channel = json_channel ? json_channel->valueint : 1;
			mtp_on = json_mtp_on ? json_mtp_on->valueint : 0;
			if (mtp_on)
			{
				mtp_power_on(mipi_channel);
				printf("SERVER2CLI_MSG_MIPI_CHANNEL_MTP: mtp power on channel %d.\n", mipi_channel);
			}
			else
			{
				mtp_power_off(mipi_channel);
				printf("SERVER2CLI_MSG_MIPI_CHANNEL_MTP: mtp power off channel %d.\n", mipi_channel);
			}
			cJSON_Delete(root);
		};
		break;
		
		case SERVER2CLI_MSG_MIPI_MODE:
		{
			int mipi_channel = 0;
			int hs_mode = 0;
			cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
			cJSON *json_channel = cJSON_GetObjectItem(root, "channel");
			cJSON *json_hs_mode = cJSON_GetObjectItem(root, "mode");
			mipi_channel = json_channel ? json_channel->valueint : 1;
			hs_mode = json_hs_mode ? json_hs_mode->valueint : 0;
			
			if (hs_mode)
			{
				printf("SERVER2CLI_MSG_MIPI_MODE: channel %d to HS mode.\n", mipi_channel);
				setMipi2828HighSpeedMode(mipi_channel, true);
				
			}
			else
			{
				setMipi2828HighSpeedMode(mipi_channel, false);
				printf("SERVER2CLI_MSG_MIPI_MODE: channel %d to LP mode.\n", mipi_channel);
			}
			cJSON_Delete(root);
		};
		break;

		case SERVER2CLI_MSG_FLICK_TEST: //Flick Test.
        {
			printf("=====: SERVER2CLI_MSG_FLICK_TEST. \n");
            cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
			cJSON *json_mipi_channel = cJSON_GetObjectItem(root, "mipi_channel");
            cJSON *json_vcom = cJSON_GetObjectItem(root, "vcom");
			cJSON *json_test_delay = cJSON_GetObjectItem(root, "delay");

			int mipi_channel = json_mipi_channel ? json_mipi_channel->valueint : 1;
			int vcom = json_vcom ? json_vcom->valueint : 0;
			int delay = json_test_delay ? json_test_delay->valueint : 100;

			printf("vcom: %d, delay: %d ms.\n", vcom, delay);
            flick_test(mipi_channel, vcom, delay);
            cJSON_Delete(root);
        }
        break;

		case SERVER2CLI_MSG_OPEN_SHORT_DEBUG:
		{
			printf("=====: SERVER2CLI_MSG_OPEN_SHORT_DEBUG. \n");

			unsigned int value = 0;
			int channel = 1;

            cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
            if (root)
            {
    			cJSON *pElem = cJSON_GetObjectItem(root, "channel");
    			if (pElem)
    				channel = pElem->valueint;

    			pElem = cJSON_GetObjectItem(root, "value_1");
    			if (pElem)
    				value |= (pElem->valueint&0xff);

    			pElem = cJSON_GetObjectItem(root, "value_2");
    			if (pElem)
    				value |= ((pElem->valueint&0xff) << 8);

    			pElem = cJSON_GetObjectItem(root, "value_3");
    			if (pElem)
    				value |= ((pElem->valueint&0xff) << 16);

    			pElem = cJSON_GetObjectItem(root, "value_4");
    			if (pElem)
    				value |= ((pElem->valueint&0xff) << 24);

                cJSON_Delete(root);
            }

			sp6_set_open_short_data(channel, value);
		}
		break;

		case SERVER2CLI_MSG_OTP: //手动烧录 2017-5-8
		{
			printf("=====: SERVER2CLI_MSG_OTP. \n%s\n", (char*)pSocketCmd->pcmd);
			cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
			cJSON *Channel = cJSON_GetObjectItem(root, "channel");
			cJSON *json_vcom = cJSON_GetObjectItem(root, "vcom");
			cJSON *json_max_otp_times = cJSON_GetObjectItem(root, "max_vcom_otp_times");

			int channel = Channel->valueint;
			int vcom = 0;
			int max_otp_times = 3;

			if (json_vcom)
				vcom = json_vcom->valueint;

			if (json_max_otp_times)
				max_otp_times = json_max_otp_times->valueint;

			cJSON_Delete(root);

			printf("manual vcom otp: channel: %d, vcom: %d, %#x, max_otp_times = %d.\n", channel, vcom, vcom, max_otp_times);

			unsigned char vcom_h = (vcom >> 8) & 0xFF;
			unsigned char vcom_l = vcom & 0xFF;
			int otp_check_error = E_ERROR_OK;

			// read old vcom and times,
			unsigned char old_vcom_h = 0;
			unsigned char old_vcom_l = 0;
			unsigned char old_otp_times = 0;
			qc_otp_data_read_vcom(channel, &old_vcom_h, &old_vcom_l, &old_otp_times);

			//max_vcom_times = QC_VCOM_MAX_OTP_TIMES;

			if ( (old_vcom_h != vcom_h) || (old_vcom_l != vcom_l) )
			{
				printf("do new otp proc ...\n");

				#if ENABLE_OTP_TESE_MODE
				max_otp_times = old_otp_times + 1;
				#endif

				if (max_otp_times <= old_otp_times)
				{
					printf("manual otp: VCOM OTP Times error! max otp times: %d, read otp: %d.\n", max_otp_times, old_otp_times);
					otp_check_error = E_ERROR_OTP_TIMES_END;
					client_sendOtp(channel, otp_check_error, max_otp_times);
				}
				else
				{
					int ptn_id = client_pg_ptnId_get();
					// do otp
					qc_otp_vcom(channel, vcom_h, vcom_l, ptn_id);
					// read new vcom and times.
					unsigned char new_vcom_h = 0;
					unsigned char new_vcom_l = 0;
					unsigned char new_otp_times = 0;
					qc_otp_data_read_vcom(channel, &new_vcom_h, &new_vcom_l, &new_otp_times);

					// check otp burn result.
					#if ENABLE_OTP_TESE_MODE
					new_otp_times = old_otp_times + 1;
					#endif

					if (new_otp_times <= old_otp_times)
					{
						printf("VCOM OTP Times error! old times: %d, new times: %d.\n", old_otp_times, new_otp_times);
						otp_check_error = E_ERROR_OTP_TIMES_NOT_CHANGE;
					}

					if (!otp_check_error)
					{
						#if ENABLE_OTP_TESE_MODE
						new_vcom_h = vcom_h;
						new_vcom_l = vcom_l;
						#endif

						if ( (vcom_h != new_vcom_h) || (vcom_l != new_vcom_l) )
						{
							printf("VCOM OTP data error! write vcom: %d, read otp vcom: %d.\n", vcom_h, vcom_l,
									new_vcom_h, new_vcom_l);
							otp_check_error = E_ERROR_OTP_DATA_WRITE_FAILED;
						}
					}

					printf("vcom otp msg: result = %d. max_otp_times = %d.\n", otp_check_error, max_otp_times);
					client_sendOtp(channel, otp_check_error, max_otp_times);
				}
			}
			else
			{
				printf("manual otp: VCOM OTP data same! don't do otp.\n");
				client_sendOtp(channel, otp_check_error, max_otp_times);
			}

			showRgbInfo_t showRgb;
			showRgb.rgb_r = 0xFF;
			showRgb.rgb_g = 0xFF;
			showRgb.rgb_b = 0xFF;
			fpgaShowRgb(&showRgb);
			printf("SERVER2CLI_MSG_OTP end.\n");
		}
		break;

		case SERVER2CLI_MSG_SET_OPEN_SHORT_MODE:
		{
			printf("=====: SERVER2CLI_MSG_SET_OPEN_SHORT_MODE. \n");
            cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
            unsigned int enable = 0;

            cJSON *pElem = cJSON_GetObjectItem(root, "enable");
			if (pElem)
				enable = pElem->valueint;

			if (enable)
				sp6_entry_open_short_mode();
			else 
				sp6_leave_open_short_mode();
			
            cJSON_Delete(root);
		}
		break;

		case SERVER2CLI_MSG_READ_OPEN_SHORT_MODE:
		{
			printf("=====: SERVER2CLI_MSG_READ_OPEN_SHORT_MODE. \n");

			unsigned char mode = sp6_read_open_short_mode();
			printf("openshort_enable: %d.\n", mode);

			client_power_test_read_open_short_mode_ack(mode);
		}
		break;

		// VCOM
		case SERVER2CLI_MSG_POWER_TEST_SET_VCOM_VOLT:
		{
			printf("=====: SERVER2CLI_MSG_POWER_TEST_SET_VCOM_VOLT. \n");
            cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
			cJSON *json_volt_vcom = cJSON_GetObjectItem(root, "volt_vcom");

			short volt_vcom = json_volt_vcom ? json_volt_vcom->valueint : 0;
			printf("set VCOM volt: %d.\n", volt_vcom);

			mcu_set_vcom_volt(volt_vcom);

            cJSON_Delete(root);
		}
		break;

		case SERVER2CLI_MSG_POWER_TEST_ENABLE_VCOM_VOLT:
		{
			printf("=====: SERVER2CLI_MSG_POWER_TEST_ENABLE_VCOM_VOLT. \n");
            cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
			cJSON *json_volt_vcom_enable = cJSON_GetObjectItem(root, "vcom_enable");

			int volt_vcom_enable = json_volt_vcom_enable ? json_volt_vcom_enable->valueint : 0;
			printf("set VCOM volt: %d.\n", volt_vcom_enable);

			mcu_set_vcom_enable(volt_vcom_enable);
            cJSON_Delete(root);
		}
		break;

		case SERVER2CLI_MSG_POWER_TEST_READ_VCOM_VOLT:
		{
			printf("=====: SERVER2CLI_MSG_POWER_TEST_READ_VCOM_VOLT. \n");
			short vcom_volt = 0;
            cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
			cJSON *pElem = cJSON_GetObjectItem(root, "channel");
			unsigned int channel = 1;

			if (pElem)
				channel = pElem->valueint;

			if (channel == 1)
				vcom_volt = mcu_read_vcom_volt(TTL_LVDS_CHANNEL_LEFT);
			else
				vcom_volt = mcu_read_vcom_volt(TTL_LVDS_CHANNEL_RIGHT);

			client_power_test_read_vcom_volt_ack(vcom_volt);
		}
		break;

		case SERVER2CLI_MSG_POWER_TEST_READ_VCOM_VOLT_STATUS:
		{
			printf("=====: SERVER2CLI_MSG_POWER_TEST_READ_VCOM_VOLT_STATUS. \n");
			short vcom_status = mcu_read_vcom_status();
			printf("=== VCOM Status: %04x.\n", vcom_status);
			client_power_test_read_vcom_status_ack(vcom_status);
		}
		break;

		// OTP
		case SERVER2CLI_MSG_POWER_TEST_SET_OTP_VOLT:
		{
			printf("=====: SERVER2CLI_MSG_POWER_TEST_SET_OTP_VOLT. \n");
            cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
			cJSON *json_volt_otp = cJSON_GetObjectItem(root, "volt_otp");

			unsigned short volt_otp = json_volt_otp ? json_volt_otp->valueint : 0;
			printf("set OTP volt: %u.\n", volt_otp);

			//ttl_mcu_module_set_otp_value(volt_otp, 1);

            cJSON_Delete(root);
		}
		break;

		case SERVER2CLI_MSG_POWER_TEST_ENABLE_OTP_VOLT:
		{
			printf("=====: SERVER2CLI_MSG_POWER_TEST_ENABLE_OTP_VOLT. \n");
            cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
			cJSON *json_volt_otp_enable = cJSON_GetObjectItem(root, "otp_enable");

			int volt_otp_enable = json_volt_otp_enable ? json_volt_otp_enable->valueint : 0;
			printf("set OTP volt: %d.\n", volt_otp_enable);

//			ttl_mcu_module_set_otp_status(volt_otp_enable);
			
            cJSON_Delete(root);
		}
		break;

		case SERVER2CLI_MSG_POWER_TEST_READ_OTP_VOLT:
		{
			printf("=====: SERVER2CLI_MSG_POWER_TEST_READ_OTP_VOLT. \n");
			unsigned short otp_volt = 0;
			int otp_status = 0;
			otp_volt = mcu_read_otp_volt();
			client_power_test_read_otp_volt_ack(otp_volt);
		}
		break;

		case SERVER2CLI_MSG_POWER_TEST_READ_OTP_VOLT_STATUS:
		{
			printf("=====: SERVER2CLI_MSG_POWER_TEST_READ_OTP_VOLT_STATUS. \n");
			unsigned short otp_status = 0;
			otp_status = mcu_read_otp_status();
			
			client_power_test_read_otp_status_ack(otp_status);
		}
		break;

		case SERVER2CLI_MSG_POWER_TEST_READ_OPEN_SHORT_DATA:
		{
			printf("=====: SERVER2CLI_MSG_POWER_TEST_READ_OPEN_SHORT_DATA. \n");
            cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
			cJSON *pElem = cJSON_GetObjectItem(root, "channel");
			unsigned int channel = 1;

			if (pElem)
				channel = pElem->valueint;

			unsigned short open_short = 0;
			if (channel == 1)
				open_short = mcu_read_open_short_data(TTL_LVDS_CHANNEL_LEFT);
			else
				open_short = mcu_read_open_short_data(TTL_LVDS_CHANNEL_RIGHT);

			client_power_test_read_open_short_ack(open_short);
		}
		break;

		case SERVER2CLI_MSG_POWER_TEST_READ_NTC_AG_DATA:
		{
			printf("=====: SERVER2CLI_MSG_POWER_TEST_READ_NTC_AG_DATA. \n");
			unsigned short ntc_ag = 0;
            cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
			cJSON *pElem = cJSON_GetObjectItem(root, "channel");
			unsigned int channel = 1;

			if (pElem)
				channel = pElem->valueint;

			if (channel == 1)
				ntc_ag = mcu_read_ntc_ag_data(TTL_LVDS_CHANNEL_LEFT);
			else
				ntc_ag = mcu_read_ntc_ag_data(TTL_LVDS_CHANNEL_RIGHT);

			client_power_test_read_ntc_ag_ack(ntc_ag);
		}
		break;
		
		case SERVER2CLI_MSG_GPIO_GET_STATE:
		{
			printf("SERVER2CLI_MSG_GPIO_GET_STATE：do nothing. \n");
		}
		break;
		
		// reg read/write test.
		case SERVER2CLI_MSG_READ_ID_OTP_INFO:
		{
			printf("=====: SERVER2CLI_MSG_READ_ID_OTP_INFO. \n");
            cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
			cJSON *json_mipi_channel = cJSON_GetObjectItem(root, "mipi_channel");

			int mipi_channel = json_mipi_channel ? json_mipi_channel->valueint : 1;

			printf("read id: channel = %d.\n", mipi_channel);

            cJSON_Delete(root);

			unsigned char otp_id1 = 0;
			unsigned char otp_id2 = 0;
			unsigned char otp_id3 = 0;
			unsigned char otp_times = 0;


			unsigned char id_datas[8] = { 0 };
			cliService_t* p_client = get_client_service_data();
			int id_len = 4;

			int ret = ic_mgr_read_chip_id_otp_info(mipi_channel, id_datas, &id_len, &otp_times);
			if (ret != 0)
			{
				id_len = 0;
			}

			printf("ic_mgr_read_chip_id_otp_info\n");

			printf("read id otp data: len = %d.\n", id_len);
			dump_data1(id_datas, id_len);

			//client_get_id_otp_info_ack(mipi_channel, otp_id1, otp_id2, otp_id3, otp_times);
			client_send_gen_cmd_read_lcd_id_notify(&p_client->socketCmdClass, p_client->send_fd, mipi_channel,
													id_datas, id_len, otp_times);
        }
        break;

		case SERVER2CLI_MSG_WRITE_ID_OTP_INFO:
		{
			printf("=====: SERVER2CLI_MSG_WRITE_ID_OTP_INFO. \n%s\n", (char*)pSocketCmd->pcmd);
			cJSON *root = cJSON_Parse((char*)pSocketCmd->pcmd);
			cJSON *Channel = cJSON_GetObjectItem(root, "channel");
			cJSON *json_id1 = cJSON_GetObjectItem(root, "id1");
			cJSON *json_id2 = cJSON_GetObjectItem(root, "id2");
			cJSON *json_id3 = cJSON_GetObjectItem(root, "id3");
			cJSON *json_max_otp_times = cJSON_GetObjectItem(root, "max_id_otp_times");

			int channel = Channel->valueint;
			unsigned char id1 = 0;
			unsigned char id2 = 0;
			unsigned char id3 = 0;
			int max_otp_times = 3;

			if (json_max_otp_times)
				max_otp_times = json_max_otp_times->valueint;

			if (json_id1)
				id1 = json_id1->valueint;
			if (json_id2)
				id2 = json_id2->valueint;
			if (json_id3)
				id3 = json_id3->valueint;

			cJSON_Delete(root);

			printf("manual otp: channel: %d, id: 0x%02x, 0x%02x, 0x%02x. max_otp_times = %d.\n", channel,
						id1, id2, id3, max_otp_times);

			int otp_check_error = E_ERROR_OK;

			// read old vcom and times,
			unsigned char old_id1 = 0;
			unsigned char old_id2 = 0;
			unsigned char old_id3 = 0;
			unsigned char old_otp_times = 0;

			qc_otp_data_read_id(channel, &old_id1, &old_id2, &old_id3, &old_otp_times);

			#if ENABLE_OTP_TESE_MODE
			max_otp_times = old_otp_times + 1;
			#endif

			if ( (old_id1 != id1) || (old_id2 != id2) || (old_id3 != id3))
			{
				printf("id otp msg: do otp proc ...\n");

				if (max_otp_times <= old_otp_times)
				{
					printf("ID otp: ID OTP Times error! max otp times: %d, read otp: %d.\n", max_otp_times, old_otp_times);
					otp_check_error = E_ERROR_OTP_TIMES_END;
					client_send_id_Otp_end(channel, otp_check_error, max_otp_times);
				}
				else
				{
					// burn id.
					int ptn_id = client_pg_ptnId_get();

					unsigned char id_data[3] = { 0 };
					int id_data_len = 3;
					id_data[0] = id1;
					id_data[1] = id2;
					id_data[2] = id3;

					ic_mgr_burn_chip_id(channel, id_data, id_data_len, ptn_id);

					// read new vcom and times.
					unsigned char new_id1 = 0;
					unsigned char new_id2 = 0;
					unsigned char new_id3 = 0;
					unsigned char new_otp_times = 0;
					qc_otp_data_read_id(channel, &new_id1, &new_id2, &new_id3, &new_otp_times);

					// check otp burn result.
					#if ENABLE_OTP_TESE_MODE
					new_otp_times = old_otp_times + 1;
					#endif

					if (new_otp_times <= old_otp_times)
					{
						printf("ID OTP Times error! old times: %d, new times: %d.\n", old_otp_times, new_otp_times);
						otp_check_error = E_ERROR_OTP_TIMES_NOT_CHANGE;
					}

					if (!otp_check_error)
					{
						#if ENABLE_OTP_TESE_MODE
						new_id1 = id1;
						new_id2 = id2;
						new_id3 = id3;
						#endif

						if ( (id1 != new_id1) || (id2 != new_id2) || (id3 != new_id3) )
						{
							printf("VCOM OTP data error! write vcom: %d, read otp vcom: %d.\n", id1, id2, id3,
									new_id1, new_id2, new_id3);
							otp_check_error = E_ERROR_OTP_DATA_WRITE_FAILED;
						}
					}

					printf("id otp msg: result = %d. max_otp_times = %d.\n", otp_check_error, max_otp_times);
					client_send_id_Otp_end(channel, otp_check_error, max_otp_times);
				}
			}
			else
			{
				printf("id otp msg: id same, don't do otp.\n");
				client_send_id_Otp_end(channel, otp_check_error, max_otp_times);
			}

			printf("SERVER2CLI_MSG_WRITE_ID_OTP_INFO end.\n");
		}
		break;

		case SERVER2CLI_MSG_GENERAL_CMD:
		{
			printf("SERVER2CLI_MSG_GENERAL_COMMAND_REQ\n");

			// MSG_TAG: command key.
			cliService_t* p_client_obj = get_client_service_data();
			if (p_client_obj == NULL)
			{
				printf("SERVER2CLI_MSG_GENERAL_COMMAND_REQ error: p_client_obj = NULL!\n");
				break;
			}

			net_general_command_proc(&p_client_obj->socketCmdClass, p_client_obj->send_fd, pSocketCmd->pcmd, pSocketCmd->len);
		}
		break;

        default:
        	DBG("unknow cmd is 0x%x", pSocketCmd->cmd);
        break;
    }

    if(pSocketCmd->pcmd && pSocketCmd->len)
    {
        free(pSocketCmd->pcmd);
        pSocketCmd->pcmd = 0;
    }
	
    free(pSocketCmd);
}
