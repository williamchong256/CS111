#NAME: William Chong
#EMAIL: williamchong256@gmail.com
#ID: 205114665

.SILENT:

default: client server

client:
	gcc -g -Wall -Wextra -lz lab1b-client.c -o lab1b-client

server:
	gcc -g -Wall -Wextra -lz lab1b-server.c -o lab1b-server

clean:
	rm -f *.o *.tar.gz lab1b-client lab1b-server

dist:
	tar -cvzf lab1b-205114665.tar.gz lab1b-client.c lab1b-server.c Makefile README 
