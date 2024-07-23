CC = clang
CFLAGS = -g #-Wall -Werror
CFILES=$(shell find . -type f -name "*.c")
INCLUDES = -Isrc $(shell find src/lib -type d -exec echo -I{} \;)
assembly=clox

default: run

run: build 
	./bin/clox

build:
	$(CC) $(CFLAGS) $(INCLUDES) $(CFILES) -o bin/$(assembly)

clean:
	rm -rf ./bin/*
