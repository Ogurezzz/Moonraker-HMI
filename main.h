#ifndef _MAIN_H
#define _MAIN_H
// C library headers
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <math.h>

// Additional Libs
#include <curl/curl.h>
//#include "json.h"

// Linux headers
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
// #include <sys/ioctl.h> // Used for TCGETS2/TCSETS2, which is required for custom baud rates
#include <unistd.h> // write(), read(), close()

#include "printer.h"

#define UART_Print(...)		do {	char _buf[128]; 			\
								sprintf(_buf, __VA_ARGS__); 	\
								printf("2uart: %s",_buf);		\
								write(serial_port, _buf, strlen(_buf));} while(0)
//#define ANUCUBIC_DGUS_TFT
#define MKS_TFT35










#endif