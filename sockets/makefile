CC = g++
CFLAGS = -g -Wall -Wextra -std=c++17 -Iinclude
EXEC = main

all : main.cc 
	$(CC) $(CFLAGS) -o $(EXEC).out $^

#build/terminal.o: src/terminal.cc include/terminal.h
#	$(CC) $(CFLAGS) -c -o $@ $<

clean :
	rm -rf $(EXEC).out
#	rm build/*.o

.PHONY : clean