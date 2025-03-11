rm main.exe
rm bin/*.o
gcc -g -c -o bin\http_parser.o http_parser.c -I.
gcc -g -c -o bin\main.o main.c -I.
gcc -g -o main.exe bin\http_parser.o bin\main.o
