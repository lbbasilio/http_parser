#include <stdio.h>

#include "http_parser.h"

int main()
{
	char error[50];
	http_get_last_error_str(error, sizeof(error));
	printf("HTTP error %d: %s\n", http_get_last_error(), error);
	return 0;
}
