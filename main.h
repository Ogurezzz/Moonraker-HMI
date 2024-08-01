#ifndef _MAIN_H
#define _MAIN_H
// C library headers
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <inttypes.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <math.h>

// Additional Libs
#include <curl/curl.h>
#define JSMN_HEADER
#include "libs/jsmn.h"
#include "libs/configini.h"

// Linux headers
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
// #include <sys/ioctl.h> // Used for TCGETS2/TCSETS2, which is required for custom baud rates
#include <unistd.h> // write(), read(), close()

#include "printer.h"

#define VERSION "0.0.1"

#define UART_Print(...)		do {	char _buf[1024]; 			\
								sprintf(_buf, __VA_ARGS__); 	\
								int r __attribute__((unused)); \
								r = write(serial_port, _buf, strlen(_buf));} while(0)

//=============================================================================
//=======================   LCD / Controller Selection  =======================
//=============================================================================

#define ANYCUBIC_DGUS_TFT		//Stock Anycubic i3 mega LCD
// #define MKS_TFT35			//Makerbase TFT35

/* Sanity checks */
#if (defined(ANYCUBIC_DGUS_TFT) &&	\
	defined(MKS_TFT35)) ||			\
									\
	(!defined(ANYCUBIC_DGUS_TFT) &&	\
	!defined(MKS_TFT35))
	#error "Only one LCD protocol should be defined"
#endif

#ifdef DEBUG
#define DEBUG_LOG(...)		do{ printf(__VA_ARGS__);}while(0)
#else
#define DEBUG_LOG(...)		do{}while(0)
#endif









#endif