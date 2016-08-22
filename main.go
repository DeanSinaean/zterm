package main

/*
	#include <termios.h>
	#include <malloc.h>
	#include <vte/vte.h>
	#include <gtk/gtk.h>
	#cgo LDFLAGS: -L./zmodem -lzmodem
	#cgo pkg-config:vte
	void zmodem_sendfile( char *filename) ;
	void setTermios(struct termios *pNewtio, unsigned short uBaudRate);
	int init(void);
	int read_uart(char ** buf_p, int len);
	int write_uart(char *buf, int len);
	void zmodem_recvfile( char *dir);
	GtkWidget * vte;
	GtkWidget * main_window;
	extern void InputCallback(void * data, int length);
	static void got_input(VteTerminal * widget, gchar * text, guint length, gpointer ptr) {
        InputCallback((void *)text, length);
	}
	static inline int ui_main()
	{
		int argc=0;
		char **argv = NULL;
		gtk_init(&argc, &argv);
		main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		vte = vte_terminal_new();
		gtk_widget_show(vte);
		gtk_container_add(GTK_CONTAINER(main_window),vte);
		gtk_widget_show(main_window);
		g_signal_connect_after(GTK_OBJECT(vte),"commit", G_CALLBACK(got_input),NULL);
		gtk_main();
		return 0;
	}
	static inline void feed(char * data, int length) {
		vte_terminal_feed(VTE_TERMINAL(vte), data, length);
	}
	static inline char * FileOpen() {
		char * filename = NULL;
		GtkWidget * dialog = gtk_file_chooser_dialog_new("Open File",main_window,
			GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
			NULL);
		if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
			filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		}
		gtk_widget_destroy(dialog);
		return filename;
	}
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
	go inputing()
	C.ui_main()
}

func parseCmd(cmd string) bool {
	switch cmd {
	case "rz":
		stop = true
		var filename *C.char = C.FileOpen()
		if filename == nil {
			C.free(filename)
			return false
		}
		var s = C.GoString(filename)
		C.free(filename)
		var c *C.char = C.CString(s)
		C.zmodem_sendfile(c)
		stop = false
		//C.free(c)
		return true
	}
	return false
}

var line []byte
var lineb []byte
var i C.int

//export InputCallback
func InputCallback(data unsafe.Pointer, length C.int) {
	lineb = C.GoBytes(data, length)
	for i = 0; i < length; i++ {
		line = append(line, lineb[i])
	}
	C.write_uart(C.CString(string(lineb)), (C.int)(len(lineb)))
	if lineb[length-1] == 0x0d {
		parseCmd(string(line))
		line = line[0:0]
	}
	return
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
				//	var s = ui.SaveFile(nil)
				var s = "ada"
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

		C.feed((*C.char)(unsafe.Pointer(&dati[0])), 1)
		if dati[0] != '\n' {
			continue
		}
		dat = dat[0:0]

	}
}
