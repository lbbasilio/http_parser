rm main.exe
rm bin/*.o
gcc -g -c -o bin\strutils.o cup\strutils\strutils.c -I.
gcc -g -c -o bin\vector.o cup\vector\vector.c -I.
gcc -g -c -o bin\hashtable.o cup\hashtable\hashtable.c -I.
gcc -g -c -o bin\http_parser.o http_parser.c -I.
gcc -g -c -o bin\main.o main.c -I.
gcc -g -o main.exe bin\strutils.o bin\vector.o bin\hashtable.o bin\http_parser.o bin\main.o
