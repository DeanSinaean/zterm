//-----------------------------------------------------------------------------
// project:		ZModem
// author:		Frank Weiler, Genshagen, Germany
// version:		0.91
// date:		October 10, 2000
// email:		frank@weilersplace.de
// copyright:	This Software is OpenSource.
// file:		ZModemFile
// description:	a class to handle all the file-based stuff
// 
//-----------------------------------------------------------------------------

#if !defined(AFX_ZMODEMFILE_H__6A43214D_9C2E_11D4_8639_F6B82A27C85A__INCLUDED_)
#define AFX_ZMODEMFILE_H__6A43214D_9C2E_11D4_8639_F6B82A27C85A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define ZMODEMFILE_OK 0
#define ZMODEMFILE_NOMOREDATA 1
#define ZMODEMFILE_ERROR -1

#include "type.h"

class CZModemFile  
{
public:
	char * GetReceivedFileName();
	void ResetAll();
	bool InitFromFILEINFO(char* buffer);
	CZModemFile(HWND hOwner);
	virtual ~CZModemFile();
	bool Open(CString filename,bool create);
	void WriteData(void* buffer,int bytes);
	void Finish();
	void SetPos(int offset);
	void OpenReceivingFile(uint64_t * offset,bool* skip);
	int GetData(void *buffer,int max,int * bytes);
	LONG GetFileSize();
	int MakeFileInfo(unsigned char* buf);
	void SetReceivingDir(CString szDir);
protected:
	HWND m_hOwner;
	int m_fd;
	char m_Filename[1024];
	LONG m_Filesize;
	char m_ReceiveDir[1024];
	char m_fileinfo[1024];
};

#endif // !defined(AFX_ZMODEMFILE_H__6A43214D_9C2E_11D4_8639_F6B82A27C85A__INCLUDED_)
