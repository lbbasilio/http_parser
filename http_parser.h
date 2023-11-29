#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include <stdint.h>


typedef struct 
{
	char method[8];
	char target[256];
	uint8_t major_version;
	uint8_t minor_version;
} http_start_line_t;

uint8_t http_get_last_error();
void http_get_last_error_str(char* buffer, size_t len);
char* http_parse_request(char* buffer, size_t len);

#endif
