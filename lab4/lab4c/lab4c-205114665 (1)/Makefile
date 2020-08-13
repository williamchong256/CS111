#NAME: William Chong
#EMAIL: williamchong256@gmail.com
#ID: 205114665

.SILENT:

default:
	gcc -std=c99 -lmraa -Wall -Wextra -g lab4c_tcp.c -o lab4c_tcp -lm
	gcc -o lab4c_tls -std=c99 -g -Wall -Wextra -lmraa -lcrypto -lssl -lm lab4c_tls.c

clean:
	rm -f lab4c_tcp lab4c_tls *.tar.gz

dist:
	tar -cvzf lab4c-205114665.tar.gz lab4c_tcp.c  lab4c_tls.c Makefile README
