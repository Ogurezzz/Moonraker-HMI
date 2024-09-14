#ifndef LOGGING_H
#define LOGGING_H
#include <stdio.h>


#define UART_Print(...)                             \
	do                                              \
	{                                               \
		char _buf[BUFFER_SIZE];                     \
		sprintf(_buf, __VA_ARGS__);                 \
		int r __attribute__((unused));              \
		r = write(serial_port, _buf, strlen(_buf)); \
	} while (0)

#define LOG_ERR(fmt, ...)                                                  \
	fprintf(stderr, "[ERROR] <%s:%d> : " fmt "\n", __FUNCTION__, __LINE__, \
			__VA_ARGS__)

#define LOG_INFO(fmt, ...) fprintf(stderr, "[INFO] : " fmt "\n", __VA_ARGS__)

#ifdef DEBUG
#define DEBUG_LOG(fmt, ...) fprintf(stderr, "[DEBUG] : " fmt "", __VA_ARGS__)
#else
#define DEBUG_LOG(fmt, ...)
#endif

#endif /* LOGGING_H */