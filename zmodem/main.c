#include "ZModemFileList.h"
#include "ZModemCore.h"
#include <termios.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <malloc.h>

extern "C" {

	int ufd=-1;
	char read_buf[1024];
	void zmodem_sendfile( char *filename) {
		Filelist * flist = new Filelist();
		flist->Add(filename);

		CZModemCore * zcore = new CZModemCore(0,ufd,0);
		zcore->Send(flist);
		free(filename);
	}

	void zmodem_recvfile( char *dir) {
		Filelist * flist = new Filelist();
		CZModemCore * zcore = new CZModemCore(0,ufd,0);
		zcore->Receive(flist,dir);
	}

	void setTermios(struct termios *pNewtio, unsigned short uBaudRate)
	{
		bzero(pNewtio,sizeof(struct termios));
		pNewtio->c_cflag = uBaudRate|CS8|CREAD|CLOCAL;
		pNewtio->c_iflag = IGNPAR;
		pNewtio->c_oflag = 0;
		pNewtio->c_lflag = 0;
		pNewtio->c_lflag &= ~(ICANON|ECHO|ECHOE|ECHOK|ECHONL|NOFLSH);
		pNewtio->c_cc[VINTR] = 0;
		pNewtio->c_cc[VQUIT] = 0;
		pNewtio->c_cc[VERASE] = 0;
		pNewtio->c_cc[VKILL] = 0;
		pNewtio->c_cc[VEOF] = 4;
		pNewtio->c_cc[VTIME] = 5;
		pNewtio->c_cc[VMIN] = 0;
		pNewtio->c_cc[VSWTC] = 0;
		pNewtio->c_cc[VSTART] = 0;
		pNewtio->c_cc[VSTOP] = 0;
		pNewtio->c_cc[VSUSP] = 0;
		pNewtio->c_cc[VEOL] = 0;
		pNewtio->c_cc[VREPRINT] = 0;
		pNewtio->c_cc[VDISCARD] = 0;
		pNewtio->c_cc[VWERASE] = 0;
		pNewtio->c_cc[VLNEXT] = 0;
		pNewtio->c_cc[VEOL2] = 0;
	}


	int init()
	{
		struct termios termios_p;
		ufd = open("/dev/ttyUSB0", O_RDWR|O_NOCTTY|O_NDELAY, 0);
		if(ufd <0) {
			printf("open ttyUSB0 failed.\n");
			return -1;
		}
		setTermios(&termios_p,B115200);
		tcflush(ufd,TCIFLUSH);
		tcsetattr(ufd,TCSANOW,&termios_p);
		return 0;
	}

	int read_uart(char ** buf_p, int len) {
		char *buf = read_buf;
		int ret = read(ufd, buf, len);
		*buf_p = buf;
		/*
		if (ret !=0) {
			int i=0;
			for(i=0;i<ret;i++) {
				printf("%c",buf[i]);
			}
		}
		*/
		return ret;
	}
	int write_uart(char *buf, int len) {
		int cnt = 0;
		/*
			int i=0;
			printf("write lne:%d\n",len);
			for(i=0;i<len;i++) {
				printf("%c",buf[i]);
			}
			*/
		while(cnt != len ) {
			int ret = write(ufd, buf, len-cnt);
			cnt += ret;
		}
		free(buf);
		return len;
	}

}


