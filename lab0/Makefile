#NAME: William Chong
#EMAIL: williamchong256@gmail.com
#ID: 205114665

.SILENT:

default: 
	gcc -std=c99 lab0.c -g -o lab0 -Wall -Wextra

check: default 
	./smokecheck.sh

clean:
	rm -f lab0 *.tar.gz *.txt

dist: default
	tar -cvzf lab0-205114665.tar.gz lab0.c Makefile README backtrace.png breakpoint.png smokecheck.sh

