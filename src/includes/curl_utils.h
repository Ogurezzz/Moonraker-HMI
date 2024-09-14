#ifndef CURL_UTILS_H_
#define CURL_UTILS_H_

#include <ctype.h>
#include <stddef.h>
#include <curl/curl.h>

#include "strbuf.h"

typedef enum reactions_s
{
	CURL_GET,
	CURL_POST,
	UART_RESPOND,
	REACTION_ERROR
} reactions_t;

typedef struct
{
	string_buffer_t address;
	string_buffer_t command;
	reactions_t		r_type;
} reaction_t;

size_t header_callback(char *buf, size_t size, size_t nmemb, void *data);
size_t write_callback(void *buf, size_t size, size_t nmemb, void *data);
char  *curl_POST(char * host, char *address, char *command, string_buffer_t *strbuf);
char  *curl_GET(char * host, char *address, char *command, string_buffer_t *strbuf);
char  *curl_execute(char * host, char *address, char *command, string_buffer_t *strbuf,
					reactions_t request_type);

#endif /* CURL_UTILS_H_ */