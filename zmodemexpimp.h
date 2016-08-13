
#ifndef _ZMODEMEXPIMP_H_
#define _ZMODEMEXPIMP_H_

#undef ZMODEM

#ifdef ZMODEM_CREATE
	#define ZMODEM __declspec(dllexport)
#else
	#define ZMODEM __declspec(dllimport)
#endif


#endif//_ZMODEMEXPIMP_H_