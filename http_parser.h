#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include <stdint.h>
#include "hashtable.h"

#include "arena.h"

#define HTTP_MAX_METHOD_LEN 8
#define HTTP_MAX_TARGET_LEN 256

#define HTTP_SUCCESS				0x00
#define HTTP_EMPTY_TOKEN			0x01
#define HTTP_EMPTY_METHOD			0x02
#define HTTP_METHOD_TOO_LARGE		0x03
#define HTTP_WHITESPACE_EXPECTED	0x04
#define HTTP_CRLF_EXPECTED			0x05
#define HTTP_TARGET_EXPECTED		0x06
#define HTTP_EMPTY_TARGET			0x07
#define HTTP_TARGET_TOO_LONG		0x08
#define HTTP_END_OF_CONTENT			0x09
#define HTTP_VERSION_EXPECTED		0x0A
#define HTTP_HEADER_EXPECTED		0x0B
#define HTTP_COLON_EXPECTED			0x0C
#define HTTP_INVALID_HEADER_BYTE	0x0D
#define HTTP_HEADER_VALUE_EXPECTED	0x0E
#define HTTP_INVALID_BODY_LENGTH	0x0F
#define HTTP_OOM					0x10

typedef struct {
	char* name;
	char* value;
} Http_Header;

typedef struct {
	Http_Header* items;
	size_t count;
	size_t capacity;
} Http_Header_Array;

typedef struct 
{
	// Start Line
	char method[HTTP_MAX_METHOD_LEN + 1];
	char target[HTTP_MAX_TARGET_LEN + 1];
	uint8_t major_version;
	uint8_t minor_version;

	//hashtable_t* headers;
	Http_Header_Array headers;
	uint8_t* body;
	size_t body_len;
} Http_Request;

//http_request_t* http_parse_request(char* buffer, size_t len);
uint8_t http_parse_request(char* buffer, size_t len, Http_Request* request, Arena* arena);
void http_get_error_str(uint8_t error, char* buffer, size_t len);

#endif
