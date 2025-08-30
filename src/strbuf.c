#include "strbuf.h"
#include <string.h>
void string_buffer_finish(string_buffer_t *sb)
{
	free(sb->ptr);
	sb->len = 0;
	sb->ptr = NULL;
}

void string_buffer_initialize(string_buffer_t *sb)
{
	sb->len = 0;
	sb->ptr = malloc(sb->len + 1);
	sb->ptr[0] = '\0';
}

/**
 * @brief Append string to string buffer
 *
 * @param buf string to append
 * @param size size of string
 * @param nmemb size of char
 * @param data receive string buffer
 * @return size_t
 */
size_t
string_buffer_callback(void *buf, size_t size, size_t nmemb, void *data)
{
	string_buffer_t *sb = data;
	size_t new_len = sb->len + size * nmemb;

	sb->ptr = realloc(sb->ptr, new_len + 1);

	memcpy(sb->ptr + sb->len, buf, size * nmemb);

	sb->ptr[new_len] = '\0';
	sb->len = new_len;

	return size * nmemb;
}
size_t
string_buffer_append(char *str, string_buffer_t * strbuf)
{
	return string_buffer_callback(str, strlen(str), sizeof(char), strbuf);
}