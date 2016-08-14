package main

/*
	#include <termios.h>
	#include <malloc.h>
	#cgo LDFLAGS: -L./ -lzmodem
	void zmodem_sendfile( char *filename) ;
	void setTermios(struct termios *pNewtio, unsigned short uBaudRate);
	int init(void);
	int read_uart(char ** buf_p, int len);
	int write_uart(char *buf, int len);
	void zmodem_recvfile( char *dir);
*/
import "C"

import (
	"bufio"
	"fmt"
	"os"
	"time"
	"unsafe"
)

var stop bool

func init() {
	stop = false
}
func main() {
	var ret C.int = C.init()
	if ret < 0 {
		return
	}

	go printing()
	inputing()

}
func parseCmd(cmd string) bool {
	switch cmd {
	case "rz":
		stop = true
		var s = "/home/dean/test.jpg"
		var c *C.char = C.CString(s)
		C.zmodem_sendfile(c)
		stop = false
		//C.free(c)
		return true
	}
	return false
}
func inputing() {
	reader := bufio.NewReader(os.Stdin)
	//var bytes =make([]byte, 1)
	for {
		if stop {
			time.Sleep(1 * time.Second)
			continue
		}
		line, isPrefix, err := reader.ReadLine()
		if err != nil || isPrefix {
			fmt.Println("golang:", err)
		}
		if parseCmd(string(line)) {
			continue
		}
		line = append(line, '\n')
		C.write_uart(C.CString(string(line)), (C.int)(len(line)))
		/*
			b,  err := reader.ReadByte()
			if err!=nil {
				fmt.Println("golang:", err)
			}
			bytes[0]=b
			C.write_uart(C.CString(string(bytes)), (C.int)(1))
		*/
	}
}

const (
	NORMAL = 0
	wait_z = 1
)

func printing() {
	var data *C.char
	var dat []byte
	var state int
	for {
		if stop {
			time.Sleep(1 * time.Second)
			continue
		}
		var ret C.int
		ret = C.read_uart(&data, 1)
		if ret <= 0 {
			continue
		}
		var s = string(C.GoStringN(data, 1))
		dati := []byte(string(s))
		dat = append(dat, dati[0])
		if len(dat) == 3 && dat[0] == 'r' && dat[1] == 'z' && dat[2] == '\r' {
			fmt.Println("zmodem sz\n")
			fmt.Println("ok to send zrinit\n")
			dat = dat[0:0]
			state = wait_z
			continue
		}
		if state == wait_z {
			if len(dat) == 21 {
				fmt.Print(dat)
				dat = dat[0:0]
				var s = "/home/dean"
				var bs = []byte(s)
				C.zmodem_recvfile((*C.char)(unsafe.Pointer(&bs[0])))
				stop = false
				state = NORMAL
				continue
			} else {
				continue //don't print
			}
		}
		fmt.Print(string(dati))
		if dati[0] != '\n' {
			continue
		}
		dat = dat[0:0]

	}
}
