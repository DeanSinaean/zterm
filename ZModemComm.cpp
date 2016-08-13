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

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//-----------------------------------------------------------------------------
CZModemComm::CZModemComm(HANDLE hcomm,HANDLE hCancelEvent)
//-----------------------------------------------------------------------------
{
	m_hcomm = hcomm;
	m_hCancelEvent = hCancelEvent;
}

//-----------------------------------------------------------------------------
CZModemComm::~CZModemComm()
//-----------------------------------------------------------------------------
{

}

//-----------------------------------------------------------------------------
DWORD CZModemComm::ReadBuffer(void *buffer, DWORD num)
//-----------------------------------------------------------------------------
{	//Variablen
	return(0);
}

//-----------------------------------------------------------------------------
DWORD CZModemComm::WriteBuffer(const void *buffer, DWORD num)
//-----------------------------------------------------------------------------
{

	return(0);
}

//-----------------------------------------------------------------------------
void CZModemComm::GetBlock(void *buffer,int max,int * actual)
//-----------------------------------------------------------------------------
{	//Variablen
}

//-----------------------------------------------------------------------------
void CZModemComm::GetBlockImm(void *buffer,int max,int * actual)
//-----------------------------------------------------------------------------
{	
	int x;
	
	x= ReadBuffer(buffer,max);
	if(x==0)
	{
		TRACE("set error %s\n","ZMODEM_TIMEOUT");
		SetLastError(ZMODEM_TIMEOUT);
	}
	*actual=x;
}

//-----------------------------------------------------------------------------
int CZModemComm::WriteBlock(const void* buf,int max)
//-----------------------------------------------------------------------------
{
	return WriteBuffer(buf,max);
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
