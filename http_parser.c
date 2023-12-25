#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "http_parser.h"

#define CUP_STRUTILS_IMPLEMENTATION
#include "strutils.h"

#define CUP_HASHTABLE_IMPLEMENTATION
#include "hashtable.h"

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
#define HTTP_HEADER_EXPECTED		0xB
#define HTTP_COLON_EXPECTED			0xC
#define HTTP_INVALID_HEADER_BYTE	0xD
#define HTTP_HEADER_VALUE_EXPECTED	0xE
#define HTTP_INVALID_BODY_LENGTH	0XF

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
	"HTTP Header expected",
	"Colon expected",
	"Invalid byte in header value",
	"Invalid body length (Content-Length header)"
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

static uint8_t http_is_vchar(const char c)
{ 
	return 0x20 < c && c < 0x7F;
}

static uint8_t http_is_obs_data(const char c)
{
	return c >= 0x80;	
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

static char* http_parse_start_line(char* start, http_request_t* req) 
{
	enum start_line_machine_state { PARSING_METHOD, FIRST_WHITESPACE, PARSING_TARGET, SECOND_WHITESPACE, PARSING_VERSION, CRLF };
	enum start_line_machine_state state = PARSING_METHOD;

	char* it = start;
	while (*it) {
		switch (state) {
			case PARSING_METHOD:
				it = http_parse_method(it, req->method);
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
				it = http_parse_target(it, req->target);
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
				it = http_parse_version(it, &req->major_version, &req->minor_version);
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

static char* http_parse_ows(char* start)
{
	char* it = start;
	while (*it && http_is_whitespace(*it)) it++;
	return it;
}

static char* http_parse_header_value(char* start, char** value, size_t* len) 
{
	*value = NULL;
	*len = 0;

	uint8_t parsing_lf = 0;
	char* it = start;
	while (*it) {
		if (parsing_lf) {
			if (*it == '\n') {
				it++;
				goto PARSE_HEADER_VALUE_END;
			}
			else {
				last_parser_error = HTTP_CRLF_EXPECTED;
				return NULL;
			}
		}
		else {
			if (*it == '\r') parsing_lf = 1;
			else if (!http_is_vchar(*it) && !http_is_obs_data(*it)) {
				last_parser_error = HTTP_INVALID_HEADER_BYTE;
				return NULL;
			}
			it++;
		}
	}

	last_parser_error = HTTP_END_OF_CONTENT;
	return NULL;

PARSE_HEADER_VALUE_END:
	*len = it - start - 2;
	*value = malloc(*len + 1);
	memcpy(*value, start, *len);
	(*value)[*len] = '\0';
	return it;
}

static char* http_parse_header(char* start, hashtable_t* headers)
{
	enum header_machine_state { PARSING_NAME, PARSING_COLON, PARSING_OWS, PARSING_VALUE };
	enum header_machine_state state = PARSING_NAME;

	char* header_name = NULL;
	size_t header_name_len;
	char* header_value = NULL;
	size_t header_value_len;

	char* it = start;
	while (*it) {
		switch (state) {
			case PARSING_NAME:
				it = http_parse_token(it, &header_name, &header_name_len);
				if (!it) {
					last_parser_error = HTTP_HEADER_EXPECTED;
					return NULL;
				}
				state++;
				break;

			case PARSING_COLON:
				if (*it++ != ':') {
					free(header_name);
					last_parser_error = HTTP_COLON_EXPECTED;
					return NULL;
				}
				state++;
				break;

			case PARSING_OWS:
				it = http_parse_ows(it);
				state++;
				break;

			case PARSING_VALUE:
				it = http_parse_header_value(it, &header_value, &header_value_len); 
				if (!it) {
					free(header_name);
					last_parser_error = HTTP_HEADER_VALUE_EXPECTED;
					return NULL;
				}
				cup_to_upper(header_name);
				hashtable_set(headers, header_name, header_value, header_value_len + 1);
				goto PARSE_HEADER_END;
		}
	}

	if (header_name) free(header_name);
	last_parser_error = HTTP_END_OF_CONTENT;
	return NULL;

PARSE_HEADER_END:
	return it;
}


static char* http_parse_headers(char* start, http_request_t* req)
{
	req->headers = hashtable_alloc();
	uint8_t parsing_lf = 0;
	char* it = start;
	while (*it) {
		if (parsing_lf) {
			if (*it == '\n') {
				it++;
				goto PARSE_HEADERS_END;
			}
			else {
				hashtable_free(req->headers); // THIS MAY LEAK MEMORY
				last_parser_error = HTTP_CRLF_EXPECTED;
				return NULL;
			}
		}
		else if (*it == '\r') {
				parsing_lf = 1;
				it++;
		}
		else {
			it = http_parse_header(it, req->headers);
			if (!it) {
				hashtable_free(req->headers); // THIS LEAKS MEMORY!
				last_parser_error = HTTP_HEADER_EXPECTED;
				return NULL;
			}
		}
	}

	hashtable_free(req->headers); // THIS MAY LEAK MEMORY
	last_parser_error = HTTP_END_OF_CONTENT;
	return NULL;

PARSE_HEADERS_END:
	return it;
}

char* http_parse_body(char* start, http_request_t* req)
{
	char* body_len_str = hashtable_get(req->headers, "CONTENT-LENGTH");
	if (!body_len_str || !strcmp(body_len_str, "0")) return start;

	size_t body_len = atoi(body_len_str);
	if (!body_len) {
		last_parser_error = HTTP_INVALID_BODY_LENGTH;
		return NULL;
	}

	// THIS MAY SEGFAULT - INCOMPLETE BODY
	req->body_len = body_len;
	req->body = malloc(body_len);
	memcpy(req->body, start, body_len);
	return start + body_len;
}

http_request_t* http_parse_request(char* buffer, size_t len)
{
	http_request_t* req = malloc(sizeof(http_request_t));
	buffer = http_parse_start_line(buffer, req);
	if (buffer) buffer = http_parse_headers(buffer, req);
	if (buffer) buffer = http_parse_body(buffer, req);
	return req;
}
