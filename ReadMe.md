zterm is a serial port terminal with zmodem
-------------------------------------------

Make
====

    cd zmodem; make 
	go build main.go

Usage
====

    sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)/zmodem  ./main 


