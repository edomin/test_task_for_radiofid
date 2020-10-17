CC = gcc
LD = gcc
CFLAGS = -Wall -Wextra -Wshadow -Wno-unused-parameter -Wno-unused-variable -Werror -Og -ggdb3
LDFLAGS = -lpthread

program: main.o common.o
	$(LD) $(LDFLAGS) -o program main.o common.o

main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o main.o

server: server.o common.o
	$(LD) $(LDFLAGS) -o server server.o common.o

server.o: server.c
	$(CC) $(CFLAGS) -c server.c -o server.o

common.o: common.c
	$(CC) $(CFLAGS) -c common.c -o common.o

run: program
	./program

# Run in another terminal
run_server: server
	./server

# Run with superuser:
# $ sudo make show_syslog
# Probably works on my PC only, because syslog file may be varyous on vary
# distros
show_syslog:
	cat /var/log/syslog

clean:
	-rm ./program ./server ./main.o ./server.o
