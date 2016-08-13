//-----------------------------------------------------------------------------
// project:		ZModem
// version:		0.91
// date:		October 10, 2000
// email:		frank@weilersplace.de
// author:		Frank Weiler, Genshagen, Germany
// copyright:	This Software is OpenSource.
//
//-----------------------------------------------------------------------------

#ifndef _ZMODEMDEF_H_
#define _ZMODEMDEF_H_

// frametypes
#define ZPAD			'*'
#define ZBIN 			'A'
#define ZHEX 			'B'
#define ZBIN32 			'C'

// headertypes
#define ZRQINIT			0	/* Request attention */
#define ZRINIT			1	/* Attention header */
#define ZSINIT			2	/* */
#define ZACK			3	/* Acknowlege request */
#define ZFILE			4	/* File name from sender */
#define ZSKIP			5	/* To sender: skip this file */
#define ZNAK			6	/* Last packet was garbled */
#define ZABORT			7	/* Abort batch transfers */
#define ZFIN			8	/* Finish session */
#define ZRPOS			9	/* Resume data trans at this position */
#define ZDATA			10	/* Data packet(s) follow */
#define ZEOF			11	/* End of file */
#define ZFERR			12	/* Fatal Read or Write error Detected */
#define ZCRC			13	/* Request for file CRC and response */
#define ZCHALLENGE		14	/* Receiver's Challenge */
#define ZCOMPL			15	/* Request is complete */
#define ZCAN			16	/* Other end canned session with CAN*5 */
#define ZFREECNT		17	/* Request for free bytes on filesystem */
#define ZCOMMAND		18	/* Command from sending program */
#define ZSTDERR			19	/* Output to standard error, data follows */

// ZDLE sequences
#define ZCRCE 'h'	/* CRC next, frame ends, header packet follows */
#define ZCRCG 'i'	/* CRC next, frame continues nonstop */
#define ZCRCQ 'j'	/* CRC next, frame continues, ZACK expected */
#define ZCRCW 'k'	/* CRC next, ZACK expected, end of frame */
#define ZRUB0 'l'	/* Translate to rubout 0177 */
#define ZRUB1 'm'	/* Translate to rubout 0377 */

// misc ZModem properties
#define CANFC32		0x20
#define CANFDX		0x01
#define CANOVIO		0x02

// special chars and flags
#define ZDLE		0x18
#define ZCNL		0x02
#define ZCBIN		0x01

#define ZRQRINIT_STR "\x2a\x2a\x18\x42\x30\x31"

#endif//_ZMODEMDEF_H_