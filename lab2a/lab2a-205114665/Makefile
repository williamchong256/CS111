#NAME: William Chong
#EMAIL: williamchong256@gmail.com
#ID: 205114665

.SILENT:

default: build

build:
	gcc -pthread lab2_add.c -g -o lab2_add -Wall -Wextra
	gcc -pthread SortedList.c lab2_list.c -g -o lab2_list -Wall -Wextra

tests: build
	chmod 755 ./data_generator.sh
	./data_generator.sh

graphs: tests
	chmod 755 ./lab2_add.gp
	chmod 755 ./lab2_list.gp
	./lab2_add.gp
	./lab2_list.gp

dist: build graphs
	tar -cvzf lab2a-205114665.tar.gz lab2_add.c lab2_list.c SortedList.c SortedList.h Makefile README *.png *.csv data_generator.sh *.gp

clean:
	rm -f lab2_add lab2_list *.tar.gz
