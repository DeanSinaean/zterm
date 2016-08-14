//-----------------------------------------------------------------------------
// project:		ZModem
// author:		Frank Weiler, Genshagen, Germany
// version:		0.91
// date:		October 10, 2000
// email:		frank@weilersplace.de
// copyright:	This Software is OpenSource.
// file:		ZModemComm.cpp
// description:	a class to handle all the communication stuff for ZModem
//-----------------------------------------------------------------------------

#include "ZModemComm.h"
#include "ZModemCore.h"
#include "type.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif
int error;

//-----------------------------------------------------------------------------
CZModemComm::CZModemComm(int ufd,HANDLE hCancelEvent)
//-----------------------------------------------------------------------------
{
	m_ufd= ufd;
	m_hCancelEvent = hCancelEvent;
}

//-----------------------------------------------------------------------------
CZModemComm::~CZModemComm()
//-----------------------------------------------------------------------------
{

}

//-----------------------------------------------------------------------------
void CZModemComm::GetBlock(void *buffer,int max,int * actual)
//-----------------------------------------------------------------------------
{	//Variablen
	
	do{
		*actual = read(m_ufd, buffer, max);
	} while(*actual<=0);
}

//-----------------------------------------------------------------------------
void CZModemComm::GetBlockImm(void *buffer,int max,int * actual)
//-----------------------------------------------------------------------------
{	
	*actual = read(m_ufd, buffer, max);
	if(*actual<=0) {
		SetLastError(ZMODEM_TIMEOUT);
	}
}

//-----------------------------------------------------------------------------
int CZModemComm::WriteBlock(const void* buf,int max)
//-----------------------------------------------------------------------------
{
	/*
	printf("ufd:%d %s-->", m_ufd, __func__);
	int i=0;
	for(i=0;i<max;i++) {
		printf("%x ", *((char* )buf+i));
	}
	printf("\n");
	printf("ufd:%d %s---->", m_ufd, __func__);
	for(i=0;i<max;i++) {
		printf("%c", *((char* )buf+i));
	}
	printf("\n");
	*/
	int written = 0;
	while(written != max) {
	    int ret = write(m_ufd, buf + written,max-written);
		if (ret <=0) {
			continue;
		}else{
			written += ret;
		}
	}
	return 0;

	return write(m_ufd, buf,max);
}

//-----------------------------------------------------------------------------
void CZModemComm::ClearInbound()
//-----------------------------------------------------------------------------
{
}

//-----------------------------------------------------------------------------
void CZModemComm::ResetAll()
//-----------------------------------------------------------------------------
{
		
}
