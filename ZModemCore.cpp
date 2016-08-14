//-----------------------------------------------------------------------------
// project:		ZModem
// author:		Frank Weiler, Genshagen, Germany
// version:		0.91
// date:		October 10, 2000
// email:		frank@weilersplace.de
// copyright:	This Software is OpenSource.
// file:		ZModemCore.cpp
// description:	A class that implements a basic ZModem protocol.
//				Not all possible features are supported, especially
//				no support for buffersizes greater than 1024.
//				Sending data is done with 1024 buffersize,
//				receiving is event-driven using overlapped I/O.
//-----------------------------------------------------------------------------

#include "ZModemCore.h"
#include "type.h"
#include <unistd.h>
#include <signal.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//-----------------------------------------------------------------------------
// purpose:	construction
// params:	howner			->	handle of the Owner Window
//			hcomm			->	handle of the modem ressource, got from TAPI
//			hcancelevent	->  handle of an event signaling user-break
CZModemCore::CZModemCore(HWND howner,int ufd,HANDLE hcancelevent)
	//-----------------------------------------------------------------------------
{
	m_hOwner = howner;
	m_ufd = ufd;
	m_hCancelEvent = hcancelevent;
	m_ZModemComm = new CZModemComm(ufd,m_hCancelEvent);
	m_ZModemFile = new CZModemFile(howner);
	maxTx = ZMCORE_MAXTX;
}

//-----------------------------------------------------------------------------
// purpose:	destruction
CZModemCore::~CZModemCore()
	//-----------------------------------------------------------------------------
{
	delete m_ZModemComm;
	delete m_ZModemFile;
}

//-----------------------------------------------------------------------------
// purpose:	sending files
// params:	filelist	->	Array of filenames (full path) to send
// returns:	success
bool CZModemCore::Send(Filelist* filelist)
	//-----------------------------------------------------------------------------
{	// 
	bool fin=false,quit=false,ok=true;
	int tries=0;
	// 
	this->ResetAll();
	if(filelist == NULL) {
		printf("filelist is NULL\n");
		return false;
	}
	if(filelist->GetSize() == 0) {
		printf("filelist size 0 \n");
		return false;
	}
	m_Filelist = filelist;
	bufTop = mainBuf + (sizeof mainBuf);
	m_ZModemComm->ClearInbound();//clear all pending data from modem
	sendrz();//send initialization string 
	while(ALLOK && !fin && (tries < 100))
	{
		quit=false;
		sendZRQINIT();//send ZRQINIT
		getZMHeader();
		if(ALLOK)
		{
			switch(headerType)
			{
				case ZCAN://canceled from other side
					{
						TRACE("set error %s\n","ZMODEM_GOTZCAN");
						SetLastError(ZMODEM_GOTZCAN);
						//						::PostMessage(m_hOwner,WM_USER_ZMODEMERROR,(WPARAM)ZMODEM_GOTZCAN,0L);
						quit=true;
						fin=true;
						break;
					}
				case ZRINIT:// ok, let's communicate
					{
						TRACE("ZRINIT erhalten");
						//validate CRC-32-property and CANFX-property
						static int Rxflags = 0377 & headerData[3];
						bcrc32 = (Rxflags & CANFC32) != 0;
						bcanfdx = Rxflags & CANFDX;
						TRACE("bcanfdx ist %s\n",bcanfdx ? "true" : "false");
						TRACE("bcrc32 ist %s\n",bcrc32 ? "true" : "false");
						// start sending all files
						ok=sendFiles();
						fin=true;
						break;
					}
				case ZCHALLENGE:
					{
						TRACE("got ZCHALLENGE");
						sendZACK();
						break;
					}
				default:
					{
						TRACE("got unexpected header");
						tries++;
						break;
					}
			}//switch
		}//if(ALLOK)
		else
		{
			DWORD err=GetLastError();
			if((err==ZMODEM_TIMEOUT) || (err==ZMODEM_CRCXM) ||
					(err==ZMODEM_CRC32) || (err==ZMODEM_BADHEX))
			{
				//				::PostMessage(m_hOwner,WM_USER_ZMODEMERROR,(WPARAM)err,0L);
				if(tries < 100)
				{
					SetLastError(0);
					tries++;
				}
			}
		}
	}//while(...)
	// sendig is over, finish session
	if(ALLOK)
	{
		quit=false;
		tries=0;
		sendZFIN();
		while(ALLOK && !quit)
		{
			getZMHeader();
			if(ALLOK && (headerType == ZFIN))
				quit=true;//other side has finish accepted
			else
			{
				if(tries < 100)
				{
					SetLastError(0);
					sendZFIN();
					tries++;
				}
			}
		}
		if(ALLOK)
			sendOO();//over and out
	}
	return(quit && fin && ok);
}

//-----------------------------------------------------------------------------
// purpose:	receiving files
// params:	filelist	->	Array of filenames received
//			receivedir	->  folder to save the received files
// returns:	success
bool CZModemCore::Receive(Filelist* filelist,char * receivedir)
	//-----------------------------------------------------------------------------
{	// 
	bool fin,quit;
	int tries;
	// 
	this->ResetAll();
	if(filelist == NULL) {
		printf("filelist is NULL\n");
		return false;
	}
	m_Filelist = filelist;
	if(receivedir)
		m_ZModemFile->SetReceivingDir(receivedir);
	bufTop = mainBuf + (sizeof mainBuf);
	m_ZModemComm->ClearInbound();
	skip = false;
	fin = false;
	while(ALLOK && !fin)
	{
		sendZRINIT();//start receiving a file
		quit = false;
		tries = 0;
		while(ALLOK && !quit) {//the procedure for receiving one file
			getZMHeader();
			if(!ALLOK) {//error handling
				DWORD err=GetLastError();
				if((err==ZMODEM_TIMEOUT) || (err==ZMODEM_CRCXM) || (err==ZMODEM_CRC32) || (err==ZMODEM_BADHEX)) {
					if(tries < 100) {
						SetLastError(0);
						sendZRINIT();
						tries++;
					}else {
						printf("error geting a header. exiting\n");
						return (-1);
					}
				}
			}
			switch(headerType) {
				case ZFIN:
					sendZFIN();//reply
					getOO();//try to get over and out signal
					quit = true;
					fin = true;
					break;
				case ZRQINIT:
					if(tries < 100) {
						sendZRINIT();
						tries++;
					} else {
						TRACE("set error %s\n","ZMODEM_INIT");
						SetLastError(ZMODEM_INIT);
					}
					break;
				case ZFILE:
					skip = false;
					getFILEINFO();//extract the FILEINFO from data block
					if(ALLOK)
					{
						receiveFile();//receive current file
						if(ALLOK)
							m_Filelist->Add(m_ZModemFile->GetReceivedFileName());
						quit = true;
					} else if(GetLastError()==ZMODEM_CRC32) {
						//						::PostMessage(m_hOwner,WM_USER_ZMODEMERROR,(WPARAM)ZMODEM_CRC32,0L);
						if(tries < 100) {
							SetLastError(0);
							sendZRINIT();
							tries++;
						}
					}
					break;
				default:
					printf("invalid headerType, exiting\n");
					return (-1);
			}
		}
	}
	return(ALLOK);
}

// Layer 3 ####################################################################
//-----------------------------------------------------------------------------
void CZModemCore::ResetAll()
	//-----------------------------------------------------------------------------
{
	ch = 0;
	gotSpecial =0;
	m_bWait = true;
	gotHeader =0;
	frameType =0;
	headerType =0;
	memset(headerData,0,4);
	moreData=0;
	memset(mainBuf,0,sizeof mainBuf);
	bufPos = NULL;
	bufTop = NULL;
	goodOffset=0;
	bytes=0;
	bcrc32=false,bcanfdx=false;
	skip=false;
	m_ZModemFile->ResetAll();
	m_ZModemComm->ResetAll();
}

//-----------------------------------------------------------------------------
bool CZModemCore::sendFiles()
	//-----------------------------------------------------------------------------
{	// 
	bool gotfile;
	int count = 0;
	int tries;
	bool sent;
	bool rw=true;
	DWORD offset;
	int i=0;
	bool fileinfosent=false;
	// 
	int nSize=m_Filelist->GetSize();

	while(ALLOK && ( i < nSize))
	{
		gotfile = m_ZModemFile->Open(m_Filelist->GetAt(i), false);
		if(gotfile)
			printf("Sending file:%s\n",m_Filelist->GetAt(i));
		else
			break;
		goodOffset = 0;
		tries = 0;
		sent = false;
		while(ALLOK && (tries < 10) && !sent) {//send one file
			if(!fileinfosent) {//send ZFILE
				sendZFILE();
				sendFILEINFO();
				fileinfosent=true;
			}
			getZMHeader();
			if(!ALLOK) { 
				DWORD err=GetLastError();
				if(err==ZMODEM_TIMEOUT)
				{
					SetLastError(0);
				}
				tries++;
			}
			switch (headerType) {
				case ZRINIT:
					TRACE("got ZRINIT\n");
					tries++;
					break;
				case ZRPOS:
					offset=(headerData[3] << 24) | (headerData[2] << 16) |
						(headerData[1] << 8)  | headerData[0];
					m_ZModemFile->SetPos(offset);
					goodOffset = offset;
					TRACE("got ZRPOS, new pos is %d\n", offset);
					rw=sendFile();
					sent = true;
					break;
				case ZNAK:
					TRACE("got ZNAK\n");
					fileinfosent=false;
					break;
				case ZSKIP:
					TRACE("got ZSKIP\n");
					sent=true;
					break;
				case ZCRC:
				default:
					break;
			}
		}
		m_ZModemFile->Finish();
		i++;//next file
	}
	return rw;
}

//-----------------------------------------------------------------------------
bool CZModemCore::sendFile()
	//-----------------------------------------------------------------------------
{	// 
	int tries = 0;
	DWORD lastpos = 0;
	DWORD newpos;
	int state;
	bool sentfile = false;
	bool rw=true;
	// 
	state = SM_SENDZDATA;
	while(ALLOK && !sentfile) {
		switch(state) {
			case SM_SENDZDATA:
				{
					moreData = 1;
					int erg=m_ZModemFile->GetData(mainBuf,maxTx,&bytes);
					if(erg==ZMODEMFILE_NOMOREDATA) {
						state = SM_SENDZEOF;
						moreData = 0;
					} else if(erg == ZMODEMFILE_OK) {
						sendZDATA();
						state = SM_SENDDATA;
					} else {
						TRACE("set error ZMODEM_ERROR_FILE");
						SetLastError(ZMODEM_ERROR_FILE);
					}
					break;
				}
			case SM_SENDDATA:
				while (ALLOK && (state == SM_SENDDATA)) {
					sendData();
					getZMHeaderImmediate();
					DWORD err=GetLastError();
					if(ALLOK && (headerType == ZRPOS)) {
						state = SM_ACTIONRPOS;
					} else {
						DWORD err=GetLastError();
						if(err==ZMODEM_TIMEOUT)//got no header
							SetLastError(0);
						if(ALLOK) {
							if(!moreData) {
								state = SM_SENDZEOF;
								break;
							} else {
								int erg=m_ZModemFile->GetData(mainBuf,maxTx,&bytes);
								if (erg == ZMODEMFILE_NOMOREDATA) {
									moreData = 0;
									state = SM_SENDZEOF;
									break;
								}
//								usleep(200000);//wait 1 ms
							}
						}
					}
				}//while
				break;
			case SM_ACTIONRPOS:
				newpos=headerData[0] | (headerData[1] << 8) |
					(headerData[2] << 16) | (headerData[3] << 24);
				TRACE("in SM_ACTIONRPOS\n");
#ifdef DEBUG
				char out[1000];
				sprintf(out,"newpos: %lu\n",newpos);
				TRACE(out);
#endif
				if(newpos <= lastpos)
					tries++;
				else
					tries = 0;
				m_ZModemFile->SetPos(newpos);
				//				::PostMessage(m_hOwner,WM_USER_ZMODEMERROR,ZMODEM_POS,0L);
				//				::PostMessage(m_hOwner,WM_USER_ZMODEMRPOS,newpos,0L);
				goodOffset = newpos;
				state = SM_SENDZDATA;
				break;
			case SM_SENDZEOF:
				TRACE("in SM_SENDZEOF\n");
				sendZEOF();
				tries = 0;
				state = SM_WAITZRINIT;
				break;
			case SM_WAITZRINIT:
				TRACE("in SM_WAITZRINIT\n");
				getZMHeader();
				if(ALLOK && (headerType == ZRINIT))
					sentfile = true;
				else if (ALLOK && (headerType == ZRPOS))
				{
					TRACE("got ZRPOS\n");
					state = SM_ACTIONRPOS;
				}
				//erweitert ...
				else if(ALLOK && (headerType == ZFERR))
				{
					TRACE("got ZFERR, finish sending");
					sentfile = true;
					rw=false;
				}
				else if(ALLOK && (headerType == ZFIN))
				{
					TRACE("got ZFIN, finish sending");
					sentfile = true;
				}
				else
				{
					tries++;
					if(tries < 100)
						SetLastError(0);
				}
				break;
		}//switch
	}//while
	return rw;
}

//-----------------------------------------------------------------------------
void CZModemCore::receiveFile()
{	 
	bool quit;
	int tries;

#ifdef DEBUG
	TRACE("%s\n",__func__);
#endif

	goodOffset = 0;

	m_ZModemFile->OpenReceivingFile(&goodOffset,&skip);
	if(skip) {
		sendZSKIP();
	} else {
		sendZRPOS();
		quit = false;
		tries = 0;
		while(ALLOK && !quit) {
			getZMHeader();
			if(ALLOK) {
				if(headerType == ZFILE) {
					if(tries < 100) {
						sendZRPOS();
						tries++;
					} else {
						TRACE("set error %s\n","ZMODEM_POS");
						SetLastError(ZMODEM_POS);
						//						::PostMessage(m_hOwner,WM_USER_ZMODEMERROR,ZMODEM_POS,0L);
					}
				} else if(headerType == ZDATA) {
					receiveData();
					quit = true;
				}
			} else {
				DWORD err=GetLastError();
				if((err==ZMODEM_TIMEOUT) || (err==ZMODEM_CRCXM) ||
						(err==ZMODEM_CRC32)) {
					//					PostMessage(m_hOwner,WM_USER_ZMODEMERROR,err,0L);
					if(tries < 100) {
						SetLastError(0);
						sendZRPOS();
						tries++;
					}
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
void CZModemCore::receiveData()
{	 
	bool quit = false;
	int tries = 0;
	moreData = 1;
	while(ALLOK && !quit)
	{
		if(moreData) {
			getData();
			if(ALLOK) {
				m_ZModemFile->WriteData(mainBuf,(DWORD)(bufPos - mainBuf));
				tries = 0;
			}
		} else {
			getZMHeader();
			if(ALLOK) {
				if(headerType == ZDATA) {
					if(posMatch())
						moreData = 1;
				}
				else if (headerType == ZEOF) {
					if (posMatch()) {
						m_ZModemFile->Finish();
						quit = true;
					}
				}
			}
		}
		if(!ALLOK)
		{
			DWORD err=GetLastError();
			if((err==ZMODEM_TIMEOUT) || (err==ZMODEM_LONGSP) ||
					(err==ZMODEM_CRCXM) || (err==ZMODEM_CRC32))
			{
				if(tries < 100)
				{
					//					::PostMessage(m_hOwner,WM_USER_ZMODEMERROR,err,0L);
					SetLastError(0);
					moreData = 0;
					sendZRPOS();
					tries++;
				}
			}
		}
	}
}

// Layer 2 ####################################################################
//-----------------------------------------------------------------------------
void CZModemCore::getZMHeaderImmediate()
	//-----------------------------------------------------------------------------
{
	m_bWait = false;
	getZMHeader();
	m_bWait = true;
}

//-----------------------------------------------------------------------------
void CZModemCore::getZMHeader()
	//-----------------------------------------------------------------------------
{	// 
	int count;
	// 
	gotHeader = 0;
	getNextCh();
	while(ALLOK && !gotHeader) {

		while (ALLOK && (ch != ZPAD)) {
			getNextCh();
		}

		if(ALLOK) {
			count = 1;
			getNextCh();
			while (ALLOK && (ch == ZPAD)) {
				count++;
				getNextCh();
			}

			if(ALLOK && (ch == ZDLE)) {
				getNextCh();
				if(ALLOK) {
					if (ch == ZBIN) {
						frameType = ZBIN;
						getBinaryHeader();
					} else if (ch == ZBIN32) {
						frameType = ZBIN32;
						getBin32Header();
					} else if ((ch == ZHEX) && (count >= 2)) {
						frameType = ZHEX;
						getHexHeader();
					}
				}
			}
		}
	}
	if(gotHeader)
	{
#ifdef DEBUG
		char out[1000];
		sprintf(out,"headerdata 0: %u 1: %u 2: %u 3: %u",headerData[0],
				headerData[1],headerData[2],headerData[3]);
		TRACE("%s\n",out);
#endif
		;      
	}
}

//-----------------------------------------------------------------------------
void CZModemCore::getOO()		
	//-----------------------------------------------------------------------------
{
	getNextCh();
	if(ALLOK)
		getNextCh();
	DWORD err=GetLastError();
	if(!ALLOK && (err==ZMODEM_TIMEOUT))
	{
		TRACE("set error 0");
		SetLastError(0);
	}
}

//-----------------------------------------------------------------------------
void CZModemCore::getFILEINFO()
	//-----------------------------------------------------------------------------
{	// 
	bool noproblem;
	// 
#ifdef DEBUG
	TRACE("%s\n",__func__);
#endif
	getData();
	if(ALLOK)
	{
		noproblem=m_ZModemFile->InitFromFILEINFO(mainBuf);
		if(!noproblem)
		{
			TRACE("set error ZMODEM_FILEDATA");
			SetLastError(ZMODEM_FILEDATA);
		}
	}
}

//-----------------------------------------------------------------------------
void CZModemCore::sendZRINIT()
	//-----------------------------------------------------------------------------
{
#ifdef DEBUG
	TRACE("%s\n",__func__);
#endif
	frameType = ZHEX;
	headerType = ZRINIT;
	headerData[0] = 0x00;
	headerData[1] = 0x00;
	headerData[2] = 0x00;
	headerData[3] = CANOVIO | CANFDX | CANFC32;
	sendHeader();
}

//-----------------------------------------------------------------------------
void CZModemCore::sendZSKIP()
	//-----------------------------------------------------------------------------
{
#ifdef DEBUG
	TRACE("%s\n",__func__);
#endif
	frameType = ZHEX;
	headerType = ZSKIP;
	headerData[0] = 0x00;
	headerData[1] = 0x00;
	headerData[2] = 0x00;
	headerData[3] = 0x00;
	sendHeader();
}

//-----------------------------------------------------------------------------
void CZModemCore::sendZRPOS()
	//-----------------------------------------------------------------------------
{	// 
	long templ;
	// 
#ifdef DEBUG
	TRACE("%s\n",__func__);
#endif
	frameType = ZHEX;
	headerType = ZRPOS;
	templ = goodOffset;
	headerData[0] = (unsigned char)(templ & 0xff);
	templ = templ >> 8;
	headerData[1] = (unsigned char)(templ & 0xff);
	templ = templ >> 8;
	headerData[2] = (unsigned char)(templ & 0xff);
	templ = templ >> 8;
	headerData[3] = (unsigned char)(templ & 0xff);
	m_ZModemComm->ClearInbound();
	sendHeader();
}

//-----------------------------------------------------------------------------
void CZModemCore::sendZFIN()
	//-----------------------------------------------------------------------------
{
#ifdef DEBUG
	TRACE("%s\n",__func__);
#endif
	frameType = ZHEX;
	headerType = ZFIN;
	headerData[0] = 0x00;
	headerData[1] = 0x00;
	headerData[2] = 0x00;
	headerData[3] = 0x00;
	sendHeader();
}

//-----------------------------------------------------------------------------
void CZModemCore::sendZEOF()
	//-----------------------------------------------------------------------------
{
#ifdef DEBUG
	TRACE("%s\n",__func__);
#endif
	frameType = ZHEX;
	headerType = ZEOF;
	headerData[0] = (unsigned char)(goodOffset);
	headerData[1] = (unsigned char)((goodOffset) >> 8);
	headerData[2] = (unsigned char)((goodOffset) >> 16);
	headerData[3] = (unsigned char)((goodOffset) >> 24);
	sendHeader();
}

//-----------------------------------------------------------------------------
void CZModemCore::sendrz()
	//-----------------------------------------------------------------------------
{
	m_ZModemComm->WriteBlock("\x72\x7a\x0d",3);
}

//-----------------------------------------------------------------------------
void CZModemCore::sendZRQINIT()
	//-----------------------------------------------------------------------------
{
	frameType = ZHEX;
	headerType = ZRQINIT;
	headerData[0]=0x00;
	headerData[1]=0x00;
	headerData[2]=0x00;
	headerData[3]=0x00;
	sendHeader();
}

//-----------------------------------------------------------------------------
void CZModemCore::sendZFILE()
	//-----------------------------------------------------------------------------
{
	if(bcrc32)
		frameType= ZBIN32;
	else
		frameType= ZBIN;
	headerType=ZFILE;
	headerData[0]=0x00;
	headerData[1]=0x00;
	headerData[2]=0x00;
	headerData[3]=0x00;//ZCBIN;
	sendHeader();
}

//-----------------------------------------------------------------------------
void CZModemCore::sendFILEINFO()
	//-----------------------------------------------------------------------------
{	// 
	unsigned char* buf;//buffer for FILEINFO
	int cnt;
	int x;
	CRCXM crc;
	CRC32 crc32;
	// 
	buf = new unsigned char[500];
	memset(buf,0,500);
	cnt = m_ZModemFile->MakeFileInfo(buf);
	if(bcrc32)
		crc32Init(&crc32);
	else
		crcxmInit(&crc);
	for(x = 0; x < cnt; x++)
	{
		if(bcrc32)
			crc32Update(&crc32, buf[x]);
		else
			crcxmUpdate(&crc, buf[x]);
	}
	// add CRC
	buf[cnt++] = ZDLE;
	buf[cnt++] = ZCRCW;
	if(bcrc32)
		crc32Update(&crc32, ZCRCW);
	else
		crcxmUpdate(&crc, ZCRCW);
	crc32 = ~crc32;

	if(bcrc32)
	{
		for(int i=4;--i>=0;)
		{
			buf[cnt++]=(unsigned char)crc32;
			crc32 >>= 8;
		}
	}
	else
	{
		buf[cnt++] = (unsigned char)crcxmHighbyte(&crc);
		buf[cnt++] = (unsigned char)crcxmLowbyte(&crc);
	}
	buf[cnt++] = 0x11;
	m_ZModemComm->WriteBlock(buf,cnt);
	delete[] buf;
}

//-----------------------------------------------------------------------------
void CZModemCore::sendZDATA()
	//-----------------------------------------------------------------------------
{
	if(bcrc32)
		frameType=ZBIN32;
	else
		frameType=ZBIN;
	headerType=ZDATA;
	headerData[0]=goodOffset & 0xff;
	headerData[1]=(goodOffset >> 8) & 0xff;
	headerData[2]=(goodOffset >> 16) & 0xff;
	headerData[3]=(goodOffset >> 24) & 0xff;
	sendHeader();
}

//-----------------------------------------------------------------------------
void CZModemCore::sendOO()
	//-----------------------------------------------------------------------------
{
	m_ZModemComm->WriteBlock("OO",2);
}

//-----------------------------------------------------------------------------
void CZModemCore::sendZACK()
	//-----------------------------------------------------------------------------
{
	frameType = ZHEX;
	headerType = ZACK;
	/*	headerData[0] = is already filled, bouncing back
		headerData[1] =
		headerData[2] =
		headerData[3] =*/
	sendHeader();
}

// Layer 1 ####################################################################
//-----------------------------------------------------------------------------
void CZModemCore::sendHeader()
	//-----------------------------------------------------------------------------
{
#ifdef DEBUG
	char out[1000];
	printf("sending frametype: %u headertyp: %u, headerdata 0: %u 1: %u 2: %u 3: %u\n",
			frameType,headerType,headerData[0],headerData[1],headerData[2],headerData[3]);
#endif
	switch(frameType)
	{
		case ZHEX:
			sendHexHeader();
			break;
		case ZBIN:
			sendBinHeader();
			break;
		case ZBIN32:
			sendBin32Header();
			break;
		default:
			TRACE("wrong headertype");
			break;
	}
}

//-----------------------------------------------------------------------------
void CZModemCore::sendHexHeader()
	//-----------------------------------------------------------------------------
{	// 
	CRCXM crc;
	// 
	crcxmInit(&crc);
	ch = ZPAD;
	sendChar();
	sendChar();
	ch = ZDLE;
	sendChar();
	ch = frameType;
	sendChar();
	ch = headerType;
	crcxmUpdate(&crc, ch);
	sendHexChar();
	ch = headerData[0];
	crcxmUpdate(&crc, ch);
	sendHexChar();
	ch = headerData[1];
	crcxmUpdate(&crc, ch);
	sendHexChar();
	ch = headerData[2];
	crcxmUpdate(&crc, ch);
	sendHexChar();
	ch = headerData[3];
	crcxmUpdate(&crc, ch);
	sendHexChar();
	ch = crcxmHighbyte(&crc);
	sendHexChar();
	ch = crcxmLowbyte(&crc);
	sendHexChar();
	ch = 0x0d;
	sendChar();
	ch = 0x0a;
	sendChar();
	ch = 0x11;
	sendChar();
}

//-----------------------------------------------------------------------------
void CZModemCore::sendBinHeader()
	//-----------------------------------------------------------------------------
{	// 
	CRCXM crc;
	// 
	crcxmInit(&crc);
	ch = ZPAD;
	sendChar();
	ch = ZDLE;
	sendChar();
	ch = frameType;
	sendChar();
	ch = headerType;
	crcxmUpdate(&crc, ch);
	sendDLEChar();
	ch = headerData[0];
	crcxmUpdate(&crc, ch);
	sendDLEChar();
	ch = headerData[1];
	crcxmUpdate(&crc, ch);
	sendDLEChar();
	ch = headerData[2];
	crcxmUpdate(&crc, ch);
	sendDLEChar();
	ch = headerData[3];
	crcxmUpdate(&crc, ch);
	sendDLEChar();
	ch = crcxmHighbyte(&crc);
	sendDLEChar();
	ch = crcxmLowbyte(&crc);
	sendDLEChar();
}

//-----------------------------------------------------------------------------
void CZModemCore::sendBin32Header()
	//-----------------------------------------------------------------------------
{	// 
	CRC32 crc;
	// 
	crc32Init(&crc);
	ch = ZPAD;
	sendChar();
	ch = ZDLE;
	sendChar();
	ch = frameType;
	sendChar();
	ch = headerType;
	crc32Update(&crc, ch);
	sendDLEChar();
	ch = headerData[0];
	crc32Update(&crc, ch);
	sendDLEChar();
	ch = headerData[1];
	crc32Update(&crc, ch);
	sendDLEChar();
	ch = headerData[2];
	crc32Update(&crc, ch);
	sendDLEChar();
	ch = headerData[3];
	crc32Update(&crc, ch);
	sendDLEChar();
	crc = ~crc;
	for(int i=0;i<4;i++)
	{
		ch=(unsigned char)crc;
		sendDLEChar();
		crc >>= 8;
	}
}

//-----------------------------------------------------------------------------
void CZModemCore::getHexHeader()
	//-----------------------------------------------------------------------------
{	// 
	CRCXM crc;
	unsigned int theirCRC;
	// 
	getNextHexCh();
	while(ALLOK && !gotHeader)
	{
		crcxmInit(&crc);
		headerType = (unsigned char)ch;
		TRACE("got headertype %u\n",headerType);
		crcxmUpdate(&crc, ch);
		getNextHexCh();
		if(ALLOK)
		{
			headerData[0] = (unsigned char)ch;
			crcxmUpdate(&crc, ch);
			getNextHexCh();
			headerData[1] = (unsigned char)ch;
			crcxmUpdate(&crc, ch);
			getNextHexCh();
			headerData[2] = (unsigned char)ch;
			crcxmUpdate(&crc, ch);
			getNextHexCh();
			headerData[3] = (unsigned char)ch;
			crcxmUpdate(&crc, ch);
			getNextHexCh();
		}
		if(ALLOK)
		{
			theirCRC = ch;
			getNextHexCh();
		}
		if(ALLOK)
		{
			theirCRC = (theirCRC << 8) | ch;
			if (crcxmValue(&crc) != theirCRC)
			{
				TRACE("set error %s\n","ZMODEM_CRCXM");
				SetLastError(ZMODEM_CRCXM);
			}
			else
			{
				gotHeader = 1;
			}
		}
	}
}

//-----------------------------------------------------------------------------
void CZModemCore::getBinaryHeader()
	//-----------------------------------------------------------------------------
{	// 
	CRCXM crc;
	unsigned int theirCRC;
	// 
	getNextDLECh();
	while(ALLOK && !gotHeader)
	{
		crcxmInit(&crc);
		headerType = (unsigned char)ch;
		TRACE("got headertype %u\n",headerType);
		crcxmUpdate(&crc, ch);
		getNextDLECh();
		if(ALLOK)
		{
			headerData[0] = (unsigned char)ch;
			crcxmUpdate(&crc, ch);
			getNextDLECh();
			headerData[1] = (unsigned char)ch;
			crcxmUpdate(&crc, ch);
			getNextDLECh();
			headerData[2] = (unsigned char)ch;
			crcxmUpdate(&crc, ch);
			getNextDLECh();
			headerData[3] = (unsigned char)ch;
			crcxmUpdate(&crc, ch);
			getNextDLECh();
		}
		if(ALLOK)
		{
			theirCRC = ch;
			getNextDLECh();
		}
		if(ALLOK)
		{
			theirCRC = (theirCRC << 8) | ch;
			if (crcxmValue(&crc) != theirCRC)
			{
				TRACE("set error %s\n","ZMODEM_CRCXM");
				SetLastError(ZMODEM_CRCXM);
			}
			else
				gotHeader = 1;
		}
	}
}

//-----------------------------------------------------------------------------
void CZModemCore::getBin32Header()
	//-----------------------------------------------------------------------------
{	// 
	CRC32 crc;
	DWORD theirCRC;
	// 
	getNextDLECh();
	while(ALLOK && !gotHeader)
	{
		crc32Init(&crc);
		headerType = (unsigned char)ch;
		TRACE("got headertype %u\n",headerType);
		crc32Update(&crc, ch);
		getNextDLECh();
		if(ALLOK)
		{
			headerData[0] = (unsigned char)ch;
			crc32Update(&crc, ch);
			getNextDLECh();
			headerData[1] = (unsigned char)ch;
			crc32Update(&crc, ch);
			getNextDLECh();
			headerData[2] = (unsigned char)ch;
			crc32Update(&crc, ch);
			getNextDLECh();
			headerData[3] = (unsigned char)ch;
			crc32Update(&crc, ch);
			getNextDLECh();
		}
		if(ALLOK)
		{
			theirCRC = (unsigned long)ch;
			getNextDLECh();
			theirCRC = theirCRC | ((unsigned long)ch << 8);
			getNextDLECh();
			theirCRC = theirCRC | ((unsigned long)ch << 16);
			getNextDLECh();
			theirCRC = theirCRC | ((unsigned long)ch << 24);
			if (~crc32Value(&crc) != theirCRC)
			{
				TRACE("set error %s\n","ZMODEM_CRC32");
				SetLastError(ZMODEM_CRC32);
			}
			else
				gotHeader = 1;
		}
	}
}

//-----------------------------------------------------------------------------
bool CZModemCore::posMatch()
	//-----------------------------------------------------------------------------
{	// 
	DWORD templ;
	bool ret;
	// 
	templ = headerData[3];
	templ = (templ << 8) | headerData[2];
	templ = (templ << 8) | headerData[1];
	templ = (templ << 8) | headerData[0];
	if(templ == goodOffset)
		ret = true;
	else
	{
		TRACE("error posMatch");
		ret = false;
	}
	return(ret);
}

//-----------------------------------------------------------------------------
void CZModemCore::getData()
	//-----------------------------------------------------------------------------
{
	if(frameType == ZBIN32)
		getData32();
	else
		getData16();
	if(!ALLOK)
		moreData = 0;
}

//-----------------------------------------------------------------------------
void CZModemCore::getData16()
	//-----------------------------------------------------------------------------
{	// 
	int quit;
	CRCXM crc;
	unsigned int theirCRC;
	// 
	bufPos = mainBuf;
	quit = 0;
	crcxmInit(&crc);
	while(ALLOK && (bufPos < bufTop) && !quit)
	{
		getNextDLECh();
		if(ALLOK)
		{
			if(gotSpecial)
			{
				if(ch != ZCRCG)
				{
					moreData = 0;
				}
				crcxmUpdate(&crc, ch);
				getNextDLECh();
				if(ALLOK)
				{
					theirCRC = ch;
					getNextDLECh();
				}
				if(ALLOK)
				{
					theirCRC = (theirCRC << 8) | ch;
					if(crcxmValue(&crc) != theirCRC)
					{
						TRACE("set error %s\n","ZMODEM_CRCXM");
						SetLastError(ZMODEM_CRCXM);
					}
					else
					{
						goodOffset += (bufPos - mainBuf);
						quit = 1;
					}
				}
			}
			else
			{
				crcxmUpdate(&crc, ch);
				*bufPos = (unsigned char)ch;
				bufPos++;
			}
		}
	}
	if(bufPos == bufTop)
	{
		TRACE("set error %s\n","ZMODEM_LONGSP");
		//		::PostMessage(m_hOwner,WM_USER_ZMODEMERROR,(WPARAM)ZMODEM_LONGSP,0L);
		SetLastError(ZMODEM_LONGSP);
	}
}

//-----------------------------------------------------------------------------
void CZModemCore::getData32()
	//-----------------------------------------------------------------------------
{	//variablen
	int quit = 0;
	CRC32 crc;
	DWORD theirCRC;

	bufPos = mainBuf;
	crc32Init(&crc);
	while(ALLOK && (bufPos < bufTop) && !quit) {
		getNextDLECh();
		if(ALLOK) {
			if(gotSpecial) {
				if(ch != ZCRCG) {
					moreData = 0;
				}
				crc32Update(&crc, ch);

				getNextDLECh();
				if(ALLOK) {
					theirCRC = ch;
					getNextDLECh();
				}
				if(ALLOK) {
					theirCRC = theirCRC | ((DWORD)ch << 8);
					getNextDLECh();
				}
				if(ALLOK) {
					theirCRC = theirCRC | ((DWORD)ch << 16);
					getNextDLECh();
				}
				if(ALLOK) {
					theirCRC = theirCRC | ((DWORD)ch << 24);
					if (~crc32Value(&crc) != theirCRC) {
						TRACE("set error %s\n","ZMODEM_CRC32");
						SetLastError(ZMODEM_CRC32);
					} else {
						goodOffset += (bufPos - mainBuf);
						quit = 1;
					}
				}
			} else {
				crc32Update(&crc, ch);
				*bufPos = (unsigned char)ch;
				bufPos++;
			}
		}
	}
	if(bufPos == bufTop)
	{
		TRACE("set error %s\n","ZMODEM_LONGSP");
		SetLastError(ZMODEM_LONGSP);
		printf("%s\n", mainBuf);
		//		kill(getpid(), SIGSEGV);
	}
}

//-----------------------------------------------------------------------------
void CZModemCore::sendData()
	//-----------------------------------------------------------------------------
{	// 
	char sptype;
	// 
	// send ZCRCG if more data, ZCRCE otherwise
	sptype = moreData ? ZCRCG : ZCRCE;
	if(bcrc32)
		sendData32(bytes,sptype);
	else
		sendData16(bytes,sptype);
	goodOffset = goodOffset + bytes;
}

//-----------------------------------------------------------------------------
void CZModemCore::sendData32(int len,char frameend)
	//-----------------------------------------------------------------------------
{
	unsigned long crc;

	crc32Init(&crc);
	for(int i=0;i<len;i++)
	{
		ch=mainBuf[i];
		crc32Update(&crc,ch);
		sendDLEChar();
	}
	crc32Update(&crc,frameend);
	crc = ~crc;
	ch = ZDLE;
	sendChar();
	ch = frameend;
	sendChar();
	for(int j=0;j<4;j++)
	{
		ch = (unsigned char)crc;
		sendDLEChar();
		crc >>= 8;
	}
}

//-----------------------------------------------------------------------------
void CZModemCore::sendData16(int len,char frameend)
	//-----------------------------------------------------------------------------
{
	CRCXM	crcxm;

	crcxmInit(&crcxm);
	for(int i=0;i<len;i++)
	{
		ch=mainBuf[i];
		crcxmUpdate(&crcxm,ch);
		sendDLEChar();
	}
	crcxmUpdate(&crcxm, frameend);

	ch = ZDLE;
	sendChar();
	ch = frameend;
	sendChar();
	for(int j=0;j<2;j++)
	{
		ch = (unsigned char)crcxm;
		sendDLEChar();
		crcxm >>= 8;
	}
}

// Layer 0 ####################################################################
//-----------------------------------------------------------------------------
void CZModemCore::getNextHexCh()
	//-----------------------------------------------------------------------------
{	// 
	int tempCh;
	// 
	getNextCh();
	if(ALLOK)
	{
		if ((ch <= 0x39) && (ch >= 0x30))
		{
			tempCh = (ch - 0x30);
		}
		else if ((ch >= 0x61) && (ch <= 0x66))
		{
			tempCh = (ch - 0x61) + 0x0a;
		}
		else
		{
			SetLastError(ZMODEM_BADHEX);
			TRACE("set error %s\n","ZMODEM_BADHEX");
		}
		if(ALLOK)
			getNextCh();
		if(ALLOK)
		{
			tempCh = tempCh << 4;
			if ((ch <= 0x39) && (ch >= 0x30))
			{
				ch = (ch - 0x30);
			}
			else if ((ch >= 0x61) && (ch <= 0x66))
			{
				ch = (ch - 0x61) + 0x0a;
			}
			else
			{
				TRACE("set error %s\n","ZMODEM_BADHEX");
				SetLastError(ZMODEM_BADHEX);
			}
		}
		if(ALLOK)
		{
			ch = ch | tempCh;
		}
	}
}

//-----------------------------------------------------------------------------
void CZModemCore::getNextDLECh()
	//-----------------------------------------------------------------------------
{
	gotSpecial = 0;
	getNextCh();
	if(ALLOK)
	{
		if(ch == ZDLE)
		{
			getNextCh();
			if(ALLOK)
			{
				if(((ch & 0x40) != 0) && ((ch & 0x20) == 0))
				{
					ch &= 0xbf;
					gotSpecial = 0;
				}
				else
				{
					gotSpecial = 1;
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
void CZModemCore::getNextCh()
	//-----------------------------------------------------------------------------
{	// 
	unsigned char buf[1];
	int actual=0;
	// 
	if(m_bWait)
		m_ZModemComm->GetBlock(buf,1,&actual);
	else
		m_ZModemComm->GetBlockImm(buf,1,&actual);
	if(ALLOK && (actual == 1))
		ch = buf[0];
}

//-----------------------------------------------------------------------------
void CZModemCore::sendChar()
	//-----------------------------------------------------------------------------
{	// 
	unsigned char buf[1];
	// 
	buf[0] = (unsigned char)ch;
	m_ZModemComm->WriteBlock(buf, 1);
}

//-----------------------------------------------------------------------------
void CZModemCore::sendChar(int c)
	//-----------------------------------------------------------------------------
{	// 
	unsigned char buf[1];
	// 
	buf[0] = (unsigned char)c;
	m_ZModemComm->WriteBlock(buf, 1);
}

//-----------------------------------------------------------------------------
// purpose: masking special characters withZDLE
//			HEX: 0x18(ZDLE selbst) 0x10 0x90 0x11 0x91 0x13 0x93 0xFF
//			OCT:  030               020 0220  021 0221  023 0223 0377
void CZModemCore::sendDLEChar()
	//-----------------------------------------------------------------------------
{
	// 
	ch &= 0xFF;
	switch(ch)
	{
		case ZDLE:
			sendChar(ZDLE);
			sendChar(ch ^= 0x40);
			break;
		case 021:
		case 0221:
		case 023:
		case 0223:
			sendChar(ZDLE);
			sendChar(ch ^= 0x40);
			break;
		default:
			if((ch & 0140) == 0)
			{
				sendChar(ZDLE);
				ch ^= 0x40;
			}
			sendChar(ch);
			break;
	}
}

//-----------------------------------------------------------------------------
void CZModemCore::sendHexChar()
	//-----------------------------------------------------------------------------
{	// 
	int tempCh;
	unsigned char hexdigit[]="\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x61\x62\x63\x64\x65\x66";
	// 
	tempCh = ch;
	ch = hexdigit[(tempCh >> 4) & 0x0f];
	sendChar();
	if(ALLOK)
	{
		ch = hexdigit[tempCh & 0x0f];
		sendChar();
	}
}
