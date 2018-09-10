#include "common.h"
#include "comStruct.h"
#include "comThread.h"
#include "comIfnet.h"

static ComDeviceIf gComDevInf;
static INT  gConnetFlag  = -1;

void ifnetSetComDevNode(ComDeviceIf *pNode)
{
	memcpy(&gComDevInf, pNode, sizeof(ComDeviceIf));
	return;
}

ComDeviceIf * ifnetGetComDevNode()
{
	return &gComDevInf;
}

void ifnetSetConnetFlag(INT nVal)
{
	gConnetFlag = nVal;
}

INT ifnetGetConnetFlag()
{
	return gConnetFlag;
}

INT ifnetDataSend(BYTE *pBuf, INT len, void *pNode)
{
	INT ret = -1;
	INT i = 0;
	BYTE cmdbuf[1024];
	struct timeval golbal_timer;
	ComDeviceIf *pfd = NULL;

	if (pNode)
	{
		pfd = (ComDeviceIf *) pNode;

		if ((pfd->sndFunc) && (pfd->nodeInf.fd >= 0))
		{
			ret = pfd->sndFunc(pBuf, len, pNode);
		}
	}
	else
	{
		pfd = &gComDevInf;

		if ((NULL != pfd->sndFunc) && (pfd->nodeInf.fd >= 0))
		{
			ret = pfd->sndFunc(pBuf, len, pfd);
		}
	}

	return ret;
}

INT ifnetDataSendCmdBuf(BYTE ucCmd, BYTE ucType, WORD16 wLen, BYTE *pBuf, void *pNode)
{
    return SUCCESS;
}

INT ifnetDataSendResult32(BYTE ucCmd, BYTE ucType, INT nResult, void *pNode)
{
    return SUCCESS;
}
