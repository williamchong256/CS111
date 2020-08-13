#NAME: William Chong
#EMAIL: williamchong256@gmail.com
#ID: 205114665

.SILENT:

default: 
	gcc lab1a.c -g -o lab1a -Wall -Wextra

clean:
	rm -f lab1a *.tar.gz

dist: default
	tar -cvzf lab1a-205114665.tar.gz lab1a Makefile README 
