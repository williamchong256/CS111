#NAME: William Chong
#EMAIL: williamchong256@gmail.com
#ID: 205114665

.SILENT:

default:
	gcc -std=c99 -lmraa -Wall -Wextra -g lab4b.c -o lab4b -lm


check: default
	chmod +x smokecheck.sh
	./smokecheck.sh

clean:
	rm -f lab4b *.tar.gz

dist:
	tar -cvzf lab4b-205114665.tar.gz lab4b.c Makefile README smokecheck.sh
