//-----------------------------------------------------------------------------
// project:		ZModem
// author:		Frank Weiler, Genshagen, Germany
// version:		0.91
// date:		October 11, 2000
// email:		frank@weilersplace.de
// copyright:	This Software is OpenSource.
// file:		ZModemCore.h
//-----------------------------------------------------------------------------

#if !defined(AFX_ZMODEMCORE_H__6A43214A_9C2E_11D4_8639_F6B82A27C85A__INCLUDED_)
#define AFX_ZMODEMCORE_H__6A43214A_9C2E_11D4_8639_F6B82A27C85A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "zmodemdef.h"
#include "crcxm.h"
#include "crc32.h"
#include "ZModemComm.h"
#include "ZModemFile.h"
#include "ZModemFileList.h"

// macros
#define ALLOK (GetLastError()==0)
//#define ALLOK true
#define needsDLE(x) (((x) == ZDLE) || ((x) == 0x10) || ((x) == 0x11) || ((x) == 0x13)) //masking special ZModem chars

// states of the zmodem state machine
#define SM_SENDZDATA	0	//sending / receiving file information
#define SM_SENDZEOF		1	//finishing file
#define SM_SENDDATA		2	//data transfer is running
#define SM_ACTIONRPOS	3	//reposition the filepointer during sending or receiving
#define SM_WAITZRINIT	4	//waiting for initalisation finish

// constants
#define ZMCORE_MAXBUF 18000
#define ZMCORE_MAXTX 1024	//sending/receiving max. 1024 bytes over com-port in one step

// errors used by this implementation
#define ZMODEM_INIT					0x20000001	//zmodem-initialization failed
#define ZMODEM_POS					0x20000002	//force reposition
#define ZMODEM_ZDATA				0x20000003	//
#define ZMODEM_CRCXM				0x20000004	//16-bit-checksum error
#define ZMODEM_LONGSP				0x20000005	//too long subpaket recieved
#define ZMODEM_CRC32				0x20000006	//32-bit-checksum error
#define ZMODEM_FILEDATA				0x20000007	//filedata has errors or missing
#define ZMODEM_BADHEX				0x20000008	//unexpected hex-char received (in a hex header)
#define ZMODEM_TIMEOUT				0x20000009	//by name
#define ZMODEM_GOTZCAN				0x2000000A	//cancel recieved (form other side)
#define ZMODEM_ABORTFROMOUTSIDE		0x2000000B	//user break
#define ZMODEM_ERROR_FILE			0x2000000C	//file handling error (during open, create, read, write)

// msgs used by this implementation, posted to notify window
#define WM_USER_ZMODEMTOTALFILES            WM_USER+1	//number of files we will receiving
#define WM_USER_ZMODEMTOTALSIZE             WM_USER+2	//size of all files we will receiving
#define WM_USER_ZMODEMRPOS					WM_USER+3	//reposition was done (mostly forced by checksum errors)
#define WM_USER_ZMODEMERROR					WM_USER+4	//generic error
#define WM_USER_ZMODEMPROGRESS				WM_USER+5	//informs about the sent or received bytes
#define WM_USER_ZMODEMNEXTFILE              WM_USER+6	//file sending/receiving starts
#define WM_USER_ZMODEMNEXTFILESIZE          WM_USER+7	//by name

// ZModem core class
class CZModemCore
{
public:
	//construction, destruction
	CZModemCore(HWND howner,int ufd,HANDLE hcancelevent);
	virtual ~CZModemCore();
	//actions
	bool Send(Filelist* filelist);
	bool Receive(Filelist* filelist,char * receivedir);
protected:
	// layer 3 methods
	void ResetAll();
	bool sendFiles();
	bool sendFile();
	void receiveFile();
	void receiveData();
	// layer 2 methods
	void getZMHeaderImmediate();
	void getZMHeader();
	void getOO();
	void getFILEINFO();
	void sendZRINIT();
	void sendZSKIP();
	void sendZRPOS();
	void sendZFIN();
	void sendZACK();
	void sendrz();
	void sendZRQINIT();
	void sendZFILE();
	void sendFILEINFO();
	void sendZDATA();
	void sendZEOF();
	void sendOO();
	// layer 1 methods
	void sendHeader();
	void sendHexHeader();
	void sendBinHeader();
	void sendBin32Header();
	void getHexHeader();
	void getBinaryHeader();
	void getBin32Header();
	void getData();
	void getData16();
	void getData32();
	bool posMatch();
	void sendData();
    void sendData32(int len,char frameend);
    void sendData16(int len,char frameend);
	// layer 0 methods
	void getNextHexCh();
	void getNextDLECh();
	void getNextCh();
	void sendChar();
	void sendChar(int c);
	void sendDLEChar();
	void sendHexChar();
	// data members
	int ch;
	int gotSpecial;
	bool m_bWait;
	int gotHeader;
	unsigned char frameType;
	unsigned char headerType;
	int headerData[4];
	int moreData;
	char mainBuf[1024+5];//data + ZCRCG + 4byteCRC
	char* bufPos;
	char* bufTop;
	uint64_t goodOffset;
	int bytes;
	bool bcrc32,bcanfdx;
	bool skip;
	int maxTx;
	//
	HWND m_hOwner;
	int m_ufd;
	HANDLE m_hCancelEvent;
	CZModemComm* m_ZModemComm;
	CZModemFile* m_ZModemFile;
	Filelist* m_Filelist;
};

#endif // !defined(AFX_ZMODEMCORE_H__6A43214A_9C2E_11D4_8639_F6B82A27C85A__INCLUDED_)
