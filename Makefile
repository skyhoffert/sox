#
# Makefile
#

all: soxc.c main.sox soxdev.c
	gcc -o soxc soxc.c
	./soxc main.sox -o main

	gcc -o devsox soxdev.c
