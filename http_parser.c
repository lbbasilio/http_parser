#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "http_parser.h"

#define HTTP_SUCCESS				0x0
#define HTTP_EMPTY_TOKEN			0x1
#define HTTP_EMPTY_METHOD			0x2
#define HTTP_METHOD_TOO_LARGE		0x3
#define HTTP_WHITESPACE_EXPECTED	0x4
#define HTTP_CRLF_EXPECTED			0x5
#define HTTP_TARGET_EXPECTED		0x6
#define HTTP_EMPTY_TARGET			0x7
#define HTTP_TARGET_TOO_LONG		0x8
#define HTTP_END_OF_CONTENT			0x9
#define HTTP_VERSION_EXPECTED		0xA

static char* error_strs[] = 
{
	"No errors found",
	"Found empty token",
	"Found empty method",
	"HTTP method too large (>7)",
	"Whitespace expected",
	"CRLF expected",
	"Target URI expected",
	"Found empty target URI",
	"Target URI too long (>255)",
	"Request content ended unexpectedly",
	"HTTP Version expected",
};

static uint8_t last_parser_error = HTTP_SUCCESS;
uint8_t http_get_last_error() { return last_parser_error; }

void http_get_last_error_str(char* buffer, size_t len)
{
	if (!buffer) return;
	if (last_parser_error >= sizeof(error_strs) / sizeof(char*)) return;
	memset(buffer, 0, len);
	strncpy(buffer, error_strs[last_parser_error], len);
}

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

static uint8_t http_is_uri_char(const char c)
{
	const char uri_chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-._:/?#[]@!$&'()*+,;%=";
	return strchr(uri_chars, c) != NULL;
}

static char* http_parse_token(char* start, char** tok, size_t* len) 
{
	*tok = NULL;
	*len = 0;

	char* it = start;
	while (*it && http_is_tchar(*it)) ++it;

	*len = it - start;
	if (*len == 0) {
		last_parser_error = HTTP_EMPTY_TOKEN;
		return NULL;
	}
	
	*tok = malloc(*len + 1);
	memcpy(*tok, start, *len);
	(*tok)[*len] = '\0';
	return it;
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
	else if (len > 7) {
		last_parser_error = HTTP_METHOD_TOO_LARGE;
		return NULL;
	}

	memcpy(method, start, len);
	method[len] = '\0';
	return it;
}

static char* http_parse_target(char* start, char target[256])
{
	char* it = start;
	if (*it++ != '/') {
		last_parser_error = HTTP_TARGET_EXPECTED;
		return NULL;
	}

	while (*it && http_is_uri_char(*it)) ++it;

	size_t len = it - start;
	if (len == 0) {
		last_parser_error = HTTP_EMPTY_TARGET;
		return NULL;
	}
	else if (len > 255) {
		last_parser_error = HTTP_TARGET_TOO_LONG;
		return NULL;
	}

	memcpy(target, start, len);
	target[len] = '\0';
	return it;
}

static char* http_parse_version(char* start, uint8_t* major, uint8_t* minor)
{
	char* it = start;
	if (*it++ != 'H' || !it ||
		*it++ != 'T' || !it ||
		*it++ != 'T' || !it ||
		*it++ != 'P' || !it) 
		goto PARSE_VERSION_ERROR;

	char major_str[4];
	char* new_start = it;
	while (*it && http_is_digit(*it)) ++it;

	size_t len = it - new_start;
	if (!len || len >= sizeof(major_str)) goto PARSE_VERSION_ERROR;
	memcpy(major_str, new_start, len); 
	major_str[len] = '\0';

	if (*it++ != '.') goto PARSE_VERSION_ERROR;

	char minor_str[4];
	new_start = it;
	while (*it && http_is_digit(*it)) ++it;

	len = it - new_start;
	if (!len || len >= sizeof(minor_str)) goto PARSE_VERSION_ERROR;
	memcpy(minor_str, new_start, len); 
	minor_str[len] = '\0';

	*major = atoi(major_str);
	*minor = atoi(minor_str);
	return it;

PARSE_VERSION_ERROR:
	last_parser_error = HTTP_VERSION_EXPECTED;
	return NULL;
}

static char* http_parse_start_line(char* start, http_start_line_t* line) 
{
		enum start_line_machine_state { PARSING_METHOD, FIRST_WHITESPACE, PARSING_TARGET, SECOND_WHITESPACE, PARSING_VERSION, CRLF };
	enum start_line_machine_state state = PARSING_METHOD;

	char* it = start;
	while (*it) {
		switch (state) {
			case PARSING_METHOD:
				it = http_parse_method(it, line->method);
				if (!it) return NULL;
				state++;
				break;

			case FIRST_WHITESPACE:
				if (*it++ != ' ') {
					last_parser_error = HTTP_WHITESPACE_EXPECTED;
					return NULL;
				}
				state++;
				break;

			case PARSING_TARGET:
				it = http_parse_target(it, line->target);
				if (!it) return NULL;
				state++;
				break;

			case SECOND_WHITESPACE:
				if (*it++ != ' ') {
					last_parser_error = HTTP_WHITESPACE_EXPECTED;
					return NULL;
				}
				state++;
				break;

			case PARSING_VERSION:
				it = http_parse_version(it, &line->major_version, &line->minor_version);
				if (!it) return NULL;
				state++;
				break;

			case CRLF:
				if (*it++ != '\r' || !*it ||*it++ != '\n') {
					last_parser_error = HTTP_CRLF_EXPECTED;
					return NULL;
				}
				goto PARSE_LINE_END;
		}
	}

	last_parser_error = HTTP_END_OF_CONTENT;
	return NULL;

PARSE_LINE_END:
	return it;
}

char* http_parse_request(char* buffer, size_t len)
{
	char met[256];
	http_start_line_t line;
	return http_parse_start_line(buffer, &line);
	return buffer;
}
