#ifndef __DWN_FILES__
#define __DWN_FILES__

#include "pgos/MsOS.h"

typedef enum
{
    DWN_FILES_REGIST,
    DWN_FILES_DWN_FILE,
    DWN_FILES_RECV_FILE,
}dwn_files_enum;

MS_U32 dwnFiles_message_queue_get();

MS_U32 dwnFiles_message_proc(MS_APARTMENT *pPartMent,MSOS_MESSAGE Message);

MS_U32 initDwnFiles(void);


#endif
