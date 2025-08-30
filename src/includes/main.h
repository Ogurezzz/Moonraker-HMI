#ifndef _MAIN_H
#define _MAIN_H
// C library headers
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Additional Libs

#define JSMN_HEADER
#include "configini.h"
#include "jsmn.h"
#include "printer.h"

// Linux headers
#include <errno.h>	 // Error integer and strerror() function
#include <fcntl.h>	 // Contains file controls like O_RDWR
#include <termios.h> // Contains POSIX terminal control definitions
// #include <sys/ioctl.h> // Used for TCGETS2/TCSETS2, which is required for custom baud rates
#include <unistd.h> // write(), read(), close()





#define BUFFER_SIZE 2048

#define VERSION "0.0.1"



//=============================================================================
//=======================   LCD / Controller Selection  =======================
//=============================================================================

#define ANYCUBIC_DGUS_TFT // Stock Anycubic i3 mega LCD
// #define MKS_TFT35			//Makerbase TFT35

/* Sanity checks */
#if (defined(ANYCUBIC_DGUS_TFT) && defined(MKS_TFT35)) || \
                                                          \
	(!defined(ANYCUBIC_DGUS_TFT) && !defined(MKS_TFT35))
#error "Only one LCD protocol should be defined"
#endif


char *urlEncode(const char *string, size_t len);

#endif