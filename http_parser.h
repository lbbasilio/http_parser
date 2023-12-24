#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include <stdint.h>
#include "hashtable.h"

typedef struct 
{
	// Start Line
	char method[8];
	char target[256];
	uint8_t major_version;
	uint8_t minor_version;

	hashtable_t* headers;
	uint8_t* body;
	size_t body_len;
} http_request_t;

uint8_t http_get_last_error();
void http_get_last_error_str(char* buffer, size_t len);
char* http_parse_request(char* buffer, size_t len);

#endif
