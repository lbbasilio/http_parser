rm main.exe
gcc -g -c -o bin\http_parser.o http_parser.c
gcc -g -c -o bin\main.o main.c
gcc -g -o main.exe bin\http_parser.o bin\main.o
