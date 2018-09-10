#include "dwnFiles/dwnFiles.h"
#include "common.h"
#include "comStruct.h"

static MS_APARTMENT *dwnFilesApartMent = 0;

MS_U32 dwnFiles_message_queue_get()
{
    return dwnFilesApartMent->MessageQueue;
}

MS_U32 dwnFiles_message_proc(MS_APARTMENT *pPartMent,MSOS_MESSAGE Message)
{
    switch(Message.MessageID)
    {
        case DWN_FILES_REGIST:
        break;

        case DWN_FILES_DWN_FILE:
        break;

        case DWN_FILES_RECV_FILE:
        break;

        default:
        break;
    }
    return 0;
}

MS_U32 initDwnFiles(void)
{
    MS_EVENT_HANDLER_LIST *pEventHandlerList = 0;

    dwnFilesApartMent = MsOS_CreateApartment("dwnFiles",dwnFiles_message_proc,pEventHandlerList);

    return 0;
}

