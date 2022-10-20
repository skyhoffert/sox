#
# Makefile
#

all: soxc.c main.sox
	gcc -o soxc soxc.c
	./soxc main.sox -o main
