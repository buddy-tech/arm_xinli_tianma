#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "tm_mes.h"
#include "common.h"
#include "../json/cJSON.h"


#if 1
#define MES_SERVER_PORT				(8020)

#define MES_INVALID_SOCKET			(-1)

#define MES_MAX_BUF_LEN				(1024 * 4)

int s_mes_server_socket_fd = MES_INVALID_SOCKET;

int send_json_data(int sock_fd, unsigned short msg_id, char* p_json_data, unsigned int json_data_len)
{
	tm_mes_packet_header_t *p_head = NULL;

	int send_len = 0;
	unsigned char send_buf[MAX_MES_MSG_BUF_LEN] = { 0 };
	int buf_len = 0;
	int packet_len = 0;

	p_head = (tm_mes_packet_header_t*)send_buf;
	p_head->sync = 0x4747;
	p_head->cmd = msg_id;
	p_head->curNo = 0;
	p_head->lastNo = 0;
	p_head->ipnum = 0;
	p_head->packet_type = 0;

	if (p_json_data && json_data_len > 0)
	{
		memcpy(send_buf + sizeof(tm_mes_packet_header_t) + 4, p_json_data, json_data_len);
		p_head->data_len = json_data_len;
	}
	else
	{
		p_head->data_len = 0;
	}

	packet_len = sizeof(tm_mes_packet_header_t) + 4 + p_head->data_len + 4;

	// crc32		
	send_len = send(sock_fd, send_buf, packet_len, 0);
	if (send_len == -1)
	{
		//AfxMessageBox("send message error!");
		return -1;
	}


	return send_len;
}

char* get_json_data(char* p_data, unsigned char data_len)
{
	if (p_data && data_len >= sizeof(tm_mes_packet_header_t) + 4)
	{
		return p_data + sizeof(tm_mes_packet_header_t) + 4;
	}
	
	return NULL;
}

int tm_mes_msg_proc(int client_fd, tm_mes_packet_header_t* p_packet, unsigned int packet_len)
{
	// sync_word + len + head + data + crc;
	char* p_json_data = get_json_data(p_packet, packet_len);
	
	printf("tm_mes_msg_proc: cmd = 0x%04x, len = 0x%04x. \n", p_packet->cmd, p_packet->data_len);
	printf("json data: \n%s.\n", p_json_data);
	
	switch (p_packet->cmd)
	{
		case E_TM_MES_MSG_POWER_ON_REQ:
			{
				printf("MES Message: E_TM_MES_MSG_POWER_ON_REQ.\n");

				if (p_json_data)
				{				
					cJSON *json_root = cJSON_Parse(p_json_data);
		            cJSON *json_power_on = cJSON_GetObjectItem(json_root, "power_on");

					if (json_power_on)
					{
						printf("json_power_on data: %d. \n", json_power_on->valueint);
					}
					
		            cJSON_Delete(json_root);
				}

				// send json packet.
				cJSON *json_root = cJSON_CreateObject();
				cJSON_AddNumberToObject(json_root, "power_status", 1);
				char *json_data = cJSON_Print(json_root);
				cJSON_Delete(json_root);

				unsigned char send_buf[MAX_MES_MSG_BUF_LEN] = { 0 };
				int buf_len = 0;
				tm_mes_packet_header_t *p_head = NULL;
				int json_data_len = 0;
				unsigned short msg_id = E_TM_MES_MSG_POWER_ON_ACK;

				if (json_data)
					json_data_len = strlen(json_data);

				
				int send_len = send_json_data(client_fd, msg_id, json_data, json_data_len);
				if (json_data)
					free(json_data);
				
				printf("send_len = %d.\n", send_len);
			}
			break;

		case E_TM_MES_MSG_POWER_OFF_REQ:
			{
				printf("MES Message: E_TM_MES_MSG_POWER_OFF_REQ.\n");

				if (p_json_data)
				{				
					cJSON *json_root = cJSON_Parse(p_json_data);
		            cJSON *json_power_on = cJSON_GetObjectItem(json_root, "power_off");

					if (json_power_on)
					{
						printf("json_power_on data: %d. \n", json_power_on->valueint);
					}
					
		            cJSON_Delete(json_root);
				}

				// send json packet.
				cJSON *json_root = cJSON_CreateObject();
				cJSON_AddNumberToObject(json_root, "power_status", 0);
				char *json_data = cJSON_Print(json_root);
				cJSON_Delete(json_root);

				unsigned char send_buf[MAX_MES_MSG_BUF_LEN] = { 0 };
				int buf_len = 0;
				tm_mes_packet_header_t *p_head = NULL;
				int json_data_len = 0;
				unsigned short msg_id = E_TM_MES_MSG_POWER_OFF_ACK;

				if (json_data)
					json_data_len = strlen(json_data);

				
				int send_len = send_json_data(client_fd, msg_id, json_data, json_data_len);
				if (json_data)
					free(json_data);
				
				printf("send_len = %d.\n", send_len);
			}
			break;

		case E_TM_MES_MSG_KEY_UP_REQ:
			{
				printf("MES Message: E_TM_MES_MSG_KEY_UP_REQ.\n");

				if (p_json_data)
				{				
					cJSON *json_root = cJSON_Parse(p_json_data);
		            cJSON *json_power_on = cJSON_GetObjectItem(json_root, "key_up");

					if (json_power_on)
					{
						printf("json_power_on data: %d. \n", json_power_on->valueint);
					}
					
		            cJSON_Delete(json_root);
				}

				// send json packet.
				cJSON *json_root = cJSON_CreateObject();
				cJSON_AddNumberToObject(json_root, "pic_index", 1);
				cJSON_AddStringToObject(json_root, "pic_name", "pic");
				char *json_data = cJSON_Print(json_root);
				cJSON_Delete(json_root);

				unsigned char send_buf[MAX_MES_MSG_BUF_LEN] = { 0 };
				int buf_len = 0;
				tm_mes_packet_header_t *p_head = NULL;
				int json_data_len = 0;
				unsigned short msg_id = E_TM_MES_MSG_KEY_UP_ACK;

				if (json_data)
					json_data_len = strlen(json_data);

				
				int send_len = send_json_data(client_fd, msg_id, json_data, json_data_len);
				if (json_data)
					free(json_data);
				
				printf("send_len = %d.\n", send_len);
			}
			break;

		case E_TM_MES_MSG_KEY_DOWN_REQ:
			{
				printf("MES Message: E_TM_MES_MSG_KEY_DOWN_REQ.\n");

				if (p_json_data)
				{				
					cJSON *json_root = cJSON_Parse(p_json_data);
		            cJSON *json_power_on = cJSON_GetObjectItem(json_root, "key_down");

					if (json_power_on)
					{
						printf("json_power_on data: %d. \n", json_power_on->valueint);
					}
					
		            cJSON_Delete(json_root);
				}

				// send json packet.
				cJSON *json_root = cJSON_CreateObject();
				cJSON_AddNumberToObject(json_root, "pic_index", 1);
				cJSON_AddStringToObject(json_root, "pic_name", "pic");;
				char *json_data = cJSON_Print(json_root);
				cJSON_Delete(json_root);

				unsigned char send_buf[MAX_MES_MSG_BUF_LEN] = { 0 };
				int buf_len = 0;
				tm_mes_packet_header_t *p_head = NULL;
				int json_data_len = 0;
				unsigned short msg_id = E_TM_MES_MSG_KEY_DOWN_ACK;

				if (json_data)
					json_data_len = strlen(json_data);

				
				int send_len = send_json_data(client_fd, msg_id, json_data, json_data_len);
				if (json_data)
					free(json_data);
				
				printf("send_len = %d.\n", send_len);
			}
			break;
			
		default:
			printf("Unknow MES Message: 0x%04x.\n", p_packet->cmd);
			break;
	}
	
	return 0;
}

int start_tm_mes_server()
{
	int ret = 0;

	unsigned char buf[MES_MAX_BUF_LEN] = { 0 };
	int buf_offset = 0;
	
	s_mes_server_socket_fd = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(s_mes_server_socket_fd == MES_INVALID_SOCKET)
	{
		printf("start_tm_mes_server error: create socket failed!\n");
		return -1; 
	}

	/* Socket Reuse */
	ret = 1;
	setsockopt(start_tm_mes_server, SOL_SOCKET, SO_REUSEADDR, (void*)&ret, sizeof(ret));
	
	struct linger so_linger = { 0 };
	setsockopt(start_tm_mes_server,SOL_SOCKET, SO_LINGER,(const char*)&so_linger, sizeof(so_linger));

	/* Bind Socket */
	struct sockaddr_in addr;
	memset(&addr,0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(MES_SERVER_PORT);
	ret = bind( s_mes_server_socket_fd, (struct sockaddr*)&addr, sizeof( addr ) );
	if(ret < 0)
	{
		printf("start_tm_mes_server error: bind socket failed!\n");
		close(s_mes_server_socket_fd);
		s_mes_server_socket_fd = MES_INVALID_SOCKET;
		return -1;
	}

	//try to listen
	if( listen( s_mes_server_socket_fd, 1 ) < 0 )
	{
		printf("start_tm_mes_server error: listen failed!\n");
		close(s_mes_server_socket_fd);
		s_mes_server_socket_fd = MES_INVALID_SOCKET;
		return -1;
	}
	
	printf("start_tm_mes_server: listening on %d ...\n", MES_SERVER_PORT );

	while(1)
	{
		int len = sizeof(addr);
		int client_fd = accept(s_mes_server_socket_fd, (struct sockaddr *)&addr, &len);
		if(client_fd == MES_INVALID_SOCKET)
		{
			printf("start_tm_mes_server error: accept failed!\n");
			break;
		}

		printf("start_tm_mes_server: New Client: %s:%d.\n", inet_ntoa(addr.sin_addr), addr.sin_port);

		buf_offset = 0;
		memset(buf, 0, sizeof(buf));
		
		while(1)
		{		
			ret = recv(client_fd, buf + buf_offset, MES_MAX_BUF_LEN, 0);
			if(ret <= 0)
			{
				printf("start_tm_mes_server: server_start->recv Error: %d.\n", ret);
				break;
			}

			buf_offset += ret;

			if (buf_offset < sizeof(tm_mes_packet_header_t))
			{
				printf("start_tm_mes_server: data too short 1! len = %d. \n", buf_offset);
				continue;
			}

			tm_mes_packet_header_t* p_head = (tm_mes_packet_header_t* )buf;
			

			// sync_word[2] + len[2] + data[n] + crc[4]
			if (buf_offset < sizeof(tm_mes_packet_header_t) + 4 + p_head->data_len + 4)
			{
				printf("start_tm_mes_server: data too short 2! len = %d. \n", buf_offset);
				continue;
			}

			printf("socket recv data len = %d. \n", buf_offset);
			dump_data1(buf, buf_offset);

			// check CRC.
			unsigned int *p_crc = buf + sizeof(tm_mes_packet_header_t) + p_head->data_len;
			// calc crc32. 
			
			// parse packet.
			unsigned int packet_len = sizeof(tm_mes_packet_header_t) + 4 + p_head->data_len + 4;
			tm_mes_msg_proc(client_fd, p_head, packet_len);
			
			// update buf_offset.
			// sync_word[2] + len[2] + data[n] + crc[4]
			buf_offset -= packet_len;

		}

		close(client_fd);
		client_fd = MES_INVALID_SOCKET;

	}

	close(s_mes_server_socket_fd);

	s_mes_server_socket_fd = MES_INVALID_SOCKET;

	return 0;
}
#endif

int init_tm_mes()
{
	// start tm mes server
	start_tm_mes_server();

	// server + notify
}

