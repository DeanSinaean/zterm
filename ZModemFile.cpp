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

#include "ZModemFile.h"
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

//-----------------------------------------------------------------------------
CZModemFile::CZModemFile(HWND hOwner)
//-----------------------------------------------------------------------------
{
	m_hOwner = hOwner;
	m_fd = -1;
}

//-----------------------------------------------------------------------------
CZModemFile::~CZModemFile()
//-----------------------------------------------------------------------------
{
}

//-----------------------------------------------------------------------------
void CZModemFile::WriteData(void* buffer,int bytes)
//-----------------------------------------------------------------------------
{	//Variablen
	write(m_fd, buffer, bytes);
}

//-----------------------------------------------------------------------------
void CZModemFile::Finish()
//-----------------------------------------------------------------------------
{
	close(m_fd);
	TRACE("error CloseHandle %u\n",GetLastError());
}

//-----------------------------------------------------------------------------
void CZModemFile::SetPos(int offset)
//-----------------------------------------------------------------------------
{
	lseek(m_fd, offset, SEEK_SET);
}

//-----------------------------------------------------------------------------
int CZModemFile::GetData(void *buffer,int max,int * bytes)
//-----------------------------------------------------------------------------
{
	int rw=ZMODEMFILE_OK;

	*bytes = read(m_fd, buffer,max);
	if(*bytes <0) {
		perror("error Readfile");
		rw = ZMODEMFILE_ERROR;
		return rw;
	}

	if(*bytes == 0 ) {//end of file
		TRACE("reached end of file\n");
		rw=ZMODEMFILE_NOMOREDATA;
		return rw;
	}

	if(max != *bytes) {
		TRACE("try to read %u bytes \n",max);
		TRACE("only %u bytes read\n",*bytes);    
	} else {
		TRACE("%u bytes read\n",*bytes);
	}
	return(rw);
}

//-----------------------------------------------------------------------------
bool CZModemFile::Open(CString filename,bool create)
//-----------------------------------------------------------------------------
{
	bool ret=false;
	TRACE("opening file %s",filename);


	//open
	if (create) {
		m_fd=open(filename,O_RDWR | O_CREAT, 0);// open for read and write, create if not exist
	}else {
		m_fd=open(filename, O_RDWR , 0);//open for read and write
	}

	if(m_fd<0) {
		TRACE("error open file %s in CreateFile %u",filename,GetLastError());
		return  false;
	}

	//get file size
	strcpy(m_Filename, filename);
	struct stat sstat;
	stat(m_Filename,&sstat);
	m_Filesize=sstat.st_size;//off_t

	return true;
}

//-----------------------------------------------------------------------------
void CZModemFile::OpenReceivingFile(uint64_t * offset,bool* skip)
//-----------------------------------------------------------------------------
{
	char filename[400];
	strcpy(filename,m_ReceiveDir);
	strcat(filename, m_Filename);

	strcpy(m_Filename, filename);
	m_fd=open(m_Filename,O_RDWR| O_CREAT, 0644);
	if(m_fd<0) {
		TRACE("error open file %s\n",m_Filename);
		TRACE("error CreateFile %u\n",GetLastError());
	} else {
		TRACE("file opened: %s\n",m_Filename);
	}
	*offset = 0;
	*skip = 0;
	//
	if(m_fileinfo[0]!='\0')//FILEINFO was sent
	{
		char* fi= new char[strlen(m_fileinfo)+1];
		memset(fi,0,strlen(m_fileinfo)+1);
		strcpy(fi,m_fileinfo);
		char* item=strtok(fi," ");
		int itemcount=0;
		while(item!=NULL)
		{
			switch(itemcount)
			{
         		case 0://size
					m_Filesize = atol(item);
					TRACE("filesize %u Bytes\n",m_Filesize);
					TRACE("filename: %s \n",m_Filename);
					
               break;
            case 1://	Modification date (ocatl number)
            	break;
            case 2://	file mode (only for UNIX, should be 0)
            	break;
            case 3://	serial number
            	break;
            case 4://	number of files remaining (incl. current)
            {
            	int fnum=atoi(item);
				break;
            }
            case 5://bytes remaining (incl. current)
            {
            	long ftotal=atol(item);
               break;
            }
         }
         itemcount++;
         item=strtok(NULL," ");
		}//while()
		delete[] fi;
	}
}

//-----------------------------------------------------------------------------
int CZModemFile::MakeFileInfo(unsigned char* buf)
//-----------------------------------------------------------------------------
{
	int cnt=0;

	int len = strlen(m_Filename);
	int i=0;
	for(i=len-1;i>=0;i--) {
		if(m_Filename[i]=='/') {
			break;
		}
	}
	printf("i===%d\n",i);
	if (i<0) {
		i=0;
	}
	if(m_Filename[i]=='/') {
		i++;
	}
	strcpy((char *)buf, &m_Filename[i]);
	cnt = strlen((char*)buf) + 1;

	sprintf((char*)buf + cnt, "%d", m_Filesize);
	cnt = cnt + strlen((char*)buf + cnt) + 1;
	printf("cnt is %d\n",cnt);

	return cnt;
}

//-----------------------------------------------------------------------------
bool CZModemFile::InitFromFILEINFO(char *buffer)
//-----------------------------------------------------------------------------
{
	memset(m_Filename,0,1024);
	strcpy(m_Filename,buffer);//copy upto first \0
	//get next block
	char* pos=(char*)memchr(buffer,'\0',(sizeof buffer)-1);
	if(pos!=NULL)
	{
    	pos++;
		//pos points to start of FILEINFO
		memset(m_fileinfo,0,1024);
		strncpy(m_fileinfo,pos,1023);
	}
	return true;
}

//-----------------------------------------------------------------------------
void CZModemFile::SetReceivingDir(CString szDir)
//-----------------------------------------------------------------------------
{
	strcpy(m_ReceiveDir , szDir);
	if(m_ReceiveDir[strlen(m_ReceiveDir)-1]!='/') {
		strcat(m_ReceiveDir, "/");
	}
}

//-----------------------------------------------------------------------------
void CZModemFile::ResetAll()
//-----------------------------------------------------------------------------
{
	memset(m_fileinfo,0,1024);
	memset(m_Filename,0,1024);
	memset(m_ReceiveDir, 0, 1024);
	m_Filesize=0;
	if(m_fd >0) 
		Finish();
	m_fd = -1;
}

//-----------------------------------------------------------------------------
char * CZModemFile::GetReceivedFileName()
//-----------------------------------------------------------------------------
{
	return m_Filename;
}
