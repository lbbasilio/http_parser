#include <stdio.h>
#include <string.h>

#include "http_parser.h"

#define ARENA_IMPLEMENTATION
#include "arena.h"

void test_http_parser(char* buffer)
{
	Http_Request req = {0};
	Arena arena = arena_create(0x10000);
	uint8_t status = http_parse_request(buffer, strlen(buffer), &req, &arena);
	if (status) {
		char error[50];
		http_get_error_str(status, error, sizeof(error));
		printf("HTTP error %d: %s\n", status, error);
	}
	else {
		printf("Success!\n");
	}
	arena_destroy(&arena);
}

int main()
{
	//test_http_parser("GET AOISDFJSFG");
	//test_http_parser("POST AOISDFJSFG");
	//test_http_parser("OPTIONS asdfoidasio");
	//test_http_parser("CONNECT");
	//test_http_parser("CONNECTASR");
	//test_http_parser(" CONNECT");
	//test_http_parser("\tCONNECT");
	//test_http_parser("GET /hello.txt HTTP1.0\r\n");
	//test_http_parser("GET /hello.txt HTTP1.4\r\n");
	//test_http_parser("GET /hello.txt HTTP1.n\r\n");
	//test_http_parser("GET /hello.txt HTTP1.0 \r\n");
	//test_http_parser("GET /hello.txt HTsP1.0\r\n");
	//test_http_parser("GET /hello.txt HTTP1.4\r\n");
	//test_http_parser("GET /hello.txt HtTP1.n\r\n");
	//test_http_parser("GET /hello.txt HTTP178.1 \r\n");
	//test_http_parser("GET /hello.txt HTTP0178.1 \r\n");

	//test_http_parser("GET /hello.txt HTTP1.1\r\nHost: localhost;\r\nUser-Agent: FakeFox\r\n");
	//test_http_parser("GET /hello.txt HTTP1.1\r\nHost: localhost;\r\nUser-Agent: FakeFox\r\n\r\n");
	//test_http_parser("GET /hello.txt HTTP1.1\r\nHost: localhost;\r\nUser-Agent: FakeFox\r\nHello body!");
	test_http_parser("GET /hello.txt HTTP/1.1\r\nHost: localhost;\r\nUser-Agent: FakeFox\r\nContent-Length: 12\r\n\r\nHello world!");

	return 0;
}
