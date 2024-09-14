#include "jsmn.h"
#include "logging.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* Function realloc_it() is a wrapper function for standard realloc()
 * with one difference - it frees old memory pointer in case of realloc
 * failure. Thus, DO NOT use old data pointer in anyway after call to
 * realloc_it(). If your code has some kind of fallback algorithm if
 * memory can't be re-allocated - use standard realloc() instead.
 */
void *
realloc_it(void *ptrmem, size_t size)
{
	void *p = realloc(ptrmem, size);
	if (!p)
	{
		free(ptrmem);
		LOG_ERR("realloc(): errno=%d\n", errno);
	}
	return p;
}

int
jsoneq(const char *json, jsmntok_t *tok, const char *s)
{
	if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
		strncmp(json + tok->start, s, tok->end - tok->start) == 0)
	{
		return 0;
	}
	return -1;
}