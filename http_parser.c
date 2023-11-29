#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define HTTP_SUCCESS			0x0
#define HTTP_EMPTY_TOKEN		0x1
#define HTTP_EMPTY_METHOD		0x2
#define HTTP_METHOD_TOO_LARGE	0x3

static uint8_t last_parser_error = HTTP_SUCCESS;
uint8_t http_get_last_error() { return last_parser_error; }
void http_get_last_error_str(char* buffer, size_t len)
{
	if (!buffer) return;

	memset(buffer, 0, len);
	switch (last_parser_error) {
		case HTTP_SUCCESS:			strncpy(buffer, "No errors found", len); break;
		case HTTP_EMPTY_TOKEN:		strncpy(buffer, "Found empty token", len); break;
		case HTTP_EMPTY_METHOD:		strncpy(buffer, "Found empty method", len); break;
		case HTTP_METHOD_TOO_LARGE:	strncpy(buffer, "HTTP method too large (>7)", len); break;
	}
}

typedef struct 
{
	char method[8];
	char target[256];
	uint8_t version_major;
	uint8_t version_minor;
} http_start_line_t;

static uint8_t http_is_whitespace(const char c)
{
	const char whitespace[] = " \t";
	return strchr(whitespace, c) != NULL;
}

static uint8_t http_is_digit(const char c)
{
	const char digits[] = "0123456789";
	return strchr(digits, c) != NULL;
}

static uint8_t http_is_alpha(const char c)
{
	const char alphabet[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	return strchr(alphabet, c) != NULL;
}

static uint8_t http_is_tchar(const char c)
{
	const char tchars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!#$%&'*+-.^_`|~";
	return strchr(tchars, c) != NULL;
}

static char* http_parse_token(char* start, char** tok, size_t* len) 
{
	*tok = NULL;
	*len = 0;

	char* it = start;
	while (*it && http_is_tchar(*it)) ++it;

	if (it - start == 0) {
		last_parser_error = HTTP_EMPTY_TOKEN;
		return NULL;
	}
	else {
		*len = it - start;
		*tok = malloc(*len + 1);
		memcpy(*tok, start, *len);
		(*tok)[*len] = '\0';
		return ++it;
	}
}

static char* http_parse_method(char* start, char method[8])
{
	char* it = start;
	while (*it && http_is_tchar(*it)) ++it;

	size_t len = it - start;
	if (len == 0) {
		last_parser_error = HTTP_EMPTY_METHOD;
		return NULL;
	}
	else if (len >= 7) {
		last_parser_error = HTTP_METHOD_TOO_LARGE;
		return NULL;
	}
	else {
		memcpy(method, start, len);
		method[len] = '\0';
		return ++it;
	}
}

static uint8_t http_parse_start_line(char* start, http_start_line_t* line) 
{
	uint8_t status = 0;

	enum start_line_machine_state { PARSING_METHOD, FIRST_WHITESPACE, PARSING_TARGET, SECOND_WHITESPACE, CRLF };
	enum start_line_machine_state state = PARSING_METHOD;

	for (char* it = start; *it != '\0'; ++it) {
		switch (state) {
			case PARSING_METHOD: break;
			default: break;
		}
	}

	return status;
}

char* http_parse_request(char* buffer, size_t len)
{
	return buffer;
}
