#include "curl_utils.h"

#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "logging.h"

size_t
header_callback(char *buf, size_t size, size_t nmemb, void *data)
{
	return string_buffer_callback(buf, size, nmemb, data);
}

size_t
write_callback(void *buf, size_t size, size_t nmemb, void *data)
{
	return string_buffer_callback(buf, size, nmemb, data);
}

char *
urlEncode(const char *string, size_t len)
{
	const char *hexChars = "0123456789ABCDEF";
	size_t		length	 = len;
	char	   *encodedString =
		malloc(3 * length + 1); // Allocate memory for the encoded string
	size_t index = 0;

	for (size_t i = 0; i < length; i++)
	{
		char currentChar = string[i];

		if (isalnum(currentChar) || currentChar == '-' || currentChar == '_' ||
			currentChar == '.')
		{
			encodedString[index++] =
				currentChar; // Append alphanumeric and safe characters as is
		}
		else if (currentChar == ' ')
		{
			encodedString[index++] = '+'; // Convert space to '+'
		}
		else
		{
			encodedString[index++] =
				'%'; // Convert other characters to percent-encoding
			encodedString[index++] =
				hexChars[(currentChar >> 4) &
						 0xF]; // Append first hexadecimal digit
			encodedString[index++] =
				hexChars[currentChar & 0xF]; // Append second hexadecimal digit
		}
	}

	encodedString[index] = '\0'; // Add null terminator

	return encodedString;
}

char *
curl_POST(char *host, char *address, char *command, string_buffer_t *strbuf)
{
	DEBUG_LOG("POST: %s%s?%s\n", host, address, command);
#ifdef DRY_RUN
	return "ok";
#else
	return curl_execute(host, address, command, strbuf, CURL_POST);
#endif
}

char *
curl_GET(char *host, char *address, char *command, string_buffer_t *strbuf)
{
	DEBUG_LOG("GET: %s%s?%s\n", host, address, command);
	return curl_execute(host, address, command, strbuf, CURL_GET);
}

char *
curl_execute(char *host, char *address, char *command, string_buffer_t *strbuf,
			 reactions_t request_type)
{
	//*** HTTP REQUEST START ***//
	string_buffer_initialize(strbuf);
	char *url = malloc(strlen(host) + strlen(address) + strlen(command) + 2);
	strcpy(url, host);
	strcat(url, address);
	static bool first_err = false;
	if (request_type == CURL_GET && strlen(command))
	{
		strcat(url, "?");
		strcat(url, command);
	}
	CURL	*curl;
	CURLcode res;
	curl = curl_easy_init();

	if (!curl)
	{
		LOG_ERR("curl_easy_init() error: %s", "");
		string_buffer_finish(strbuf);
		free(url);
		return NULL;
	}

	curl_easy_setopt(curl, CURLOPT_URL, url);
	if (request_type == CURL_POST)
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, command);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, strbuf);
	curl_easy_setopt(curl, CURLOPT_HEADERDATA, strbuf);
	res = curl_easy_perform(curl);
	while (res == CURLE_COULDNT_CONNECT)
	{
		if (!first_err)
		{
			first_err = true;
			LOG_ERR("Request failed: curl_easy_perform(): %s",
					curl_easy_strerror(res));
		}
		sleep(1);
		res = curl_easy_perform(curl);
	}

	if (res != CURLE_OK)
	{
		LOG_ERR("Request failed: curl_easy_perform(): %s",
				curl_easy_strerror(res));

		curl_easy_cleanup(curl);
		string_buffer_finish(strbuf);
		free(url);
		return NULL;
	}
	if (first_err)
	{
		LOG_INFO("%s", "Server online. Continue...");
		first_err = false;
	}
	free(url);
	curl_easy_cleanup(curl);
	return (char *)strbuf->ptr;
}