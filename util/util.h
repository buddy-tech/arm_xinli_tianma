#ifndef _UTIL_H
#define _UTIL_H

#include <unistd.h>

#ifndef KB
#define KB(a) (1024*(a))
#endif
#ifndef MB
#define MB(a) (1024*1024*(a))
#endif

#define closesocket close
#define stricmp strcasecmp
#define SLEEP(a) sleep(a)

typedef struct networkaddress{
	char		ip[32];
	unsigned short	port;
}networkaddress;

#define CRCPOLY_LE 0xedb88320

int parse_path( char* path );
int get_line( char* buf, int size, int* pos, char* line, int max );
char* http_ext_desc( char* ext );
char* http_code_string(int no);
int  cur_time();
//void format_time(time_t t, char* buf);
int  file_exists( const char* path, int* is_dir );
int  mkdir_recursive( char* path );
void read_network_addr( networkaddress* srv, char* s, int* count, int max  );
struct sockaddr_in;
int netaddr_set( const char* name, struct sockaddr_in* addr );
int send_and_wait( int sock, char* p, int len );
int set_socket_nonblocking(int sock);
int def( const char* in, int in_len, char* out, int out_len );
unsigned long crc32_le(unsigned long crc, unsigned char const *p, long len);
int ftpmsg_format(int code, char* src, char* dst, int dest_len);

int  parseBmp(char *pBmpName,char **ppRGBData,int *pSize);
void saveBmpToPtn(char *pBmpFullName);
void loadPtnToMem(char *pBmpFullName,char **pRgbBuffer,int *u32RgbSize);

#endif
