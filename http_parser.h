#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include <stdint.h>

uint8_t http_get_last_error();
void http_get_last_error_str(char* buffer, size_t len);
char* http_parse_request(char* buffer, size_t len);

#endif
