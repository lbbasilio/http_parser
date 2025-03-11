#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "http_parser.h"
#include "dynamic_array.h"
#include "arena.h"

static char* error_strs[] = 
{
	"No errors found",
	"Found empty token",
	"Found empty method",
	"HTTP method too large",
	"Whitespace expected",
	"CRLF expected",
	"Target URI expected",
	"Found empty target URI",
	"Target URI too long",
	"Request content ended unexpectedly",
	"HTTP Version expected",
	"HTTP Header expected",
	"Colon expected",
	"Invalid byte in header value",
	"Header value expected",
	"Invalid body length (Content-Length header)",
	"Out Of Memory",
	"Unknown error"
};

void http_get_error_str(uint8_t error, char* buffer, size_t len)
{
	if (!buffer)
		return;

	if (error >= sizeof(error_strs) / sizeof(char*))
		error = sizeof(error_strs) / sizeof(char*) - 1;

	memset(buffer, 0, len);
	strncpy(buffer, error_strs[error], len);
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

static uint8_t http_parse_token(char** ptr, char** tok, size_t* len, Arena* arena)
{
	char* start = *ptr;
	char* it = start;
	while (*it && http_is_tchar(*it)) ++it;

	*len = it - start;
	if (*len == 0)
		return HTTP_EMPTY_TOKEN;

	*tok = arena_alloc(arena, *len + 1);
	if (*tok == NULL)
		return HTTP_OOM;

	memcpy(*tok, start, *len);
	(*tok)[*len] = '\0';
	*ptr = it;
	return HTTP_SUCCESS;
}

static uint8_t http_parse_method(char** ptr, char method[HTTP_MAX_METHOD_LEN + 1])
{
	char* start = *ptr;
	char* it = start;
	while (*it && http_is_tchar(*it)) ++it;

	size_t len = it - start;
	if (len == 0) 
		return HTTP_EMPTY_METHOD;

	if (len > HTTP_MAX_METHOD_LEN)
		return HTTP_METHOD_TOO_LARGE;

	memcpy(method, start, len);
	method[len] = '\0';
	*ptr = it;
	return HTTP_SUCCESS;
}

static uint8_t http_parse_target(char** ptr, char target[HTTP_MAX_TARGET_LEN + 1])
{
	char* start = *ptr;
	char* it = start;
	if (*it++ != '/') 
		return HTTP_TARGET_EXPECTED;

	while (*it && http_is_uri_char(*it)) ++it;

	size_t len = it - start;
	if (len == 0)
		return HTTP_EMPTY_TARGET;
		
	if (len > HTTP_MAX_TARGET_LEN) 
		return HTTP_TARGET_TOO_LONG;

	memcpy(target, start, len);
	target[len] = '\0';
	*ptr = it;
	return HTTP_SUCCESS;
}

static uint8_t http_parse_version(char** ptr, uint8_t* major, uint8_t* minor)
{
	char* start = *ptr;
	char* it = start;
	if (strncmp(it, "HTTP/", 5) != 0)
		return HTTP_VERSION_EXPECTED;

	it += 5;
	start = it;
	while (*it && http_is_digit(*it)) ++it;

	char major_str[4];
	size_t len = it - start;
	if (!len || len >= sizeof(major_str))
		return HTTP_VERSION_EXPECTED;

	memcpy(major_str, start, len); 
	major_str[len] = '\0';

	if (*it++ != '.') 
		return HTTP_VERSION_EXPECTED;

	start = it;
	while (*it && http_is_digit(*it)) ++it;

	char minor_str[4];
	len = it - start;
	if (!len || len >= sizeof(minor_str))
		return HTTP_VERSION_EXPECTED;

	memcpy(minor_str, start, len); 
	minor_str[len] = '\0';

	*major = atoi(major_str);
	*minor = atoi(minor_str);
	*ptr = it;
	return HTTP_SUCCESS;
}

static uint8_t http_parse_start_line(char** ptr, Http_Request* req, Arena* arena)
{
	enum start_line_machine_state { PARSING_METHOD, FIRST_WHITESPACE, PARSING_TARGET, SECOND_WHITESPACE, PARSING_VERSION, CRLF };
	enum start_line_machine_state state = PARSING_METHOD;

	uint8_t status = HTTP_SUCCESS;
	char* start = *ptr;
	char* it = start;
	while (*it) {
		switch (state) {
			case PARSING_METHOD:
				status = http_parse_method(&it, req->method);
				if (status)
					return status;
				state++;
				break;

			case FIRST_WHITESPACE:
				if (*it++ != ' ')
					return HTTP_WHITESPACE_EXPECTED;
				state++;
				break;

			case PARSING_TARGET:
				status = http_parse_target(&it, req->target);
				if (status)
					return status;
				state++;
				break;

			case SECOND_WHITESPACE:
				if (*it++ != ' ') 
					return HTTP_WHITESPACE_EXPECTED;
				state++;
				break;

			case PARSING_VERSION:
				status = http_parse_version(&it, &req->major_version, &req->minor_version);
				if (status)
					return status;
				state++;
				break;

			case CRLF:
				if (*it++ != '\r' || !*it ||*it++ != '\n')
					return HTTP_CRLF_EXPECTED;

				*ptr = it;
				return HTTP_SUCCESS;
		}
	}

	*ptr = it;
	return HTTP_END_OF_CONTENT;
}

static void http_parse_ows(char** ptr)
{
	char* it = *ptr;
	while (*it && http_is_whitespace(*it)) it++;
	*ptr = it;
}

static uint8_t http_parse_header_value(char** ptr, char** value, size_t* len, Arena* arena)
{
	*value = NULL;
	*len = 0;

	uint8_t parsing_lf = 0;
	char* start = *ptr;
	char* it = start;
	while (*it) {
		if (parsing_lf) {
			if (*it == '\n') {
				it++;
				goto PARSE_HEADER_VALUE_END;
			}
			else 
				return HTTP_CRLF_EXPECTED;
		}
		else {
			if (*it == '\r') 
				parsing_lf = 1;
			else if (!http_is_vchar(*it) && !http_is_obs_data(*it))
				return HTTP_INVALID_HEADER_BYTE;
			it++;
		}
	}

	*ptr = it;
	return HTTP_END_OF_CONTENT;

PARSE_HEADER_VALUE_END:
	*len = it - start - 1;
	*value = arena_alloc(arena, *len + 1);
	memcpy(*value, start, *len);
	(*value)[*len] = '\0';
	*ptr = it;
	return HTTP_SUCCESS;

}

static uint8_t http_parse_header(char** ptr, Http_Header_Array* headers, Arena* arena)
{
	enum header_machine_state { PARSING_NAME, PARSING_COLON, PARSING_OWS, PARSING_VALUE };
	enum header_machine_state state = PARSING_NAME;

	char* header_name = NULL;
	size_t header_name_len;
	char* header_value = NULL;
	size_t header_value_len;

	uint8_t status = HTTP_SUCCESS;
	char* start = *ptr;
	char* it = start;
	while (*it) {
		switch (state) {
			case PARSING_NAME:
				status = http_parse_token(&it, &header_name, &header_name_len, arena);
				if (status)
					return HTTP_HEADER_EXPECTED;
				state++;
				break;

			case PARSING_COLON:
				if (*it++ != ':')
					return HTTP_COLON_EXPECTED;
				state++;
				break;

			case PARSING_OWS:
				http_parse_ows(&it);
				state++;
				break;

			case PARSING_VALUE:
				status = http_parse_header_value(&it, &header_value, &header_value_len, arena); 
				if (status)
					return HTTP_HEADER_VALUE_EXPECTED;

				Http_Header header = {
					.name = header_name,
					.value = header_value
				};
				da_append(headers, header, arena);

				*ptr = it;
				return HTTP_SUCCESS;
		}
	}

	*ptr = it;
	return HTTP_END_OF_CONTENT;
}

static uint8_t http_parse_headers(char** ptr, Http_Request* req, Arena* arena)
{
	uint8_t parsing_lf = 0;
	char* it = *ptr;
	while (*it) {
		if (parsing_lf) {
			if (*it == '\n') {
				*ptr = ++it;
				return HTTP_SUCCESS;
			}
			else
				return HTTP_CRLF_EXPECTED;
		}
		else if (*it == '\r') {
			parsing_lf = 1;
			it++;
		}
		else {
			uint8_t status = http_parse_header(&it, &req->headers, arena);
			if (status)
				return status;
		}
	}

	return HTTP_END_OF_CONTENT;
}

static uint8_t http_parse_body(char** ptr, Http_Request* req, Arena* arena)
{
	size_t body_len = 0;
	for (int i = 0; i < req->headers.count; ++i) {
		if (!strcasecmp(req->headers.items[i].name, "CONTENT-LENGTH")) {
			body_len = atoi(req->headers.items[i].value);
			break;
		}
	}

	if (body_len == 0)
		return HTTP_INVALID_BODY_LENGTH;

	req->body_len = body_len;
	req->body = arena_alloc(arena, body_len);
	memcpy(req->body, *ptr, body_len);
	*ptr += body_len;
	return HTTP_SUCCESS;
}

uint8_t http_parse_request(char* buffer, size_t len, Http_Request* request, Arena* arena)
{
	uint8_t status = HTTP_SUCCESS;
	unsigned char* checkpoint = arena_checkpoint(arena);

	status = http_parse_start_line(&buffer, request, arena);
	if (status) 
		goto HTTP_PARSE_ERROR;

	status = http_parse_headers(&buffer, request, arena);
	if (status) 
		goto HTTP_PARSE_ERROR;

	status = http_parse_body(&buffer, request, arena);
	if (status) 
		goto HTTP_PARSE_ERROR;

	return HTTP_SUCCESS;

HTTP_PARSE_ERROR:
	memset(request, 0, sizeof(Http_Request));
	arena_rollback(arena, checkpoint);
	return status;
}
