libzmodem.so: crc32.h crcxm.h ZModemComm.cpp ZModemComm.h ZModemCore.cpp ZModemCore.h zmodemdef.h zmodemexpimp.h ZModemFile.cpp ZModemFile.h ZModemFileList.cpp ZModemFileList.h
	g++ ZModemCore.cpp ZModemComm.cpp ZModemFile.cpp ZModemFileList.cpp -fPIC -o libzmodem.so -std=c++11  -fpermissive -shared
