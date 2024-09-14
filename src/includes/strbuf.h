#ifndef _STRBUF_H
#define _STRBUF_H

#include <inttypes.h>
#include <stdlib.h>

typedef struct string_buffer_s
{
	char *ptr;
	size_t len;
} string_buffer_t;

void string_buffer_initialize(string_buffer_t *sb);
size_t string_buffer_append(char *str, string_buffer_t * strbuf);
size_t string_buffer_callback(void *buf, size_t size, size_t nmemb, void *data);
void string_buffer_finish(string_buffer_t *sb);

#endif