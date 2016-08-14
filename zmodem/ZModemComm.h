//-----------------------------------------------------------------------------
// project:		ZModem
// author:		Frank Weiler, Genshagen, Germany
// version:		0.91
// date:		October 10, 2000
// email:		frank@weilersplace.de
// copyright:	This Software is OpenSource.
// file:		ZModemComm.h
// description:	a class to handle all the communication stuff for ZModem
//-----------------------------------------------------------------------------

#if !defined(AFX_ZMODEMCOMM_H__6A43214C_9C2E_11D4_8639_F6B82A27C85A__INCLUDED_)
#define AFX_ZMODEMCOMM_H__6A43214C_9C2E_11D4_8639_F6B82A27C85A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "type.h"

class CZModemComm  
{
public:
	void ResetAll();
	CZModemComm(int ufd,void * hCancelEvent);
	void GetBlock(void *buffer,int max,int * actual);
	void GetBlockImm(void *buffer,int max,int * actual);
	int WriteBlock(const void* buf,int max);
	void ClearInbound();
	virtual ~CZModemComm();
protected:
	DWORD WriteBuffer(const void* buffer, DWORD num);
	DWORD ReadBuffer(void* buffer, DWORD num);
	bool SetupReadEvent(LPOVERLAPPED lpOverlappedRead,void* lpszInputBuffer,
                          DWORD dwSizeofBuffer,LPDWORD lpnNumberOfBytesRead);
	bool HandleReadEvent(LPOVERLAPPED lpOverlappedRead,void* lpszInputBuffer,
                           DWORD dwSizeofBuffer,LPDWORD lpnNumberOfBytesRead);

	HANDLE m_hCancelEvent;
	int m_ufd;
};

#endif // !defined(AFX_ZMODEMCOMM_H__6A43214C_9C2E_11D4_8639_F6B82A27C85A__INCLUDED_)
