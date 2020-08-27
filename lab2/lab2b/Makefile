#NAME: William Chong
#EMAIL: williamchong256@gmail.com
#ID: 205114665

.SILENT:

default: 
	gcc -pthread -lprofiler SortedList.c lab2_list.c -g -o lab2_list -Wall -Wextra

tests: default
	chmod 755 ./data_generator.sh
	./data_generator.sh
#	-./lab2_list --threads=1 --iterations=1 --lists=4 --yield=id >> lab2b_list.csv
#	-./lab2_list --threads=1 --iterations=2 --lists=4 --yield=id >> lab2b_list.csv
#	-./lab2_list --threads=1 --iterations=4 --lists=4 --yield=id >> lab2b_list.csv
#	-./lab2_list --threads=1 --iterations=8 --lists=4 --yield=id >> lab2b_list.csv
#	-./lab2_list --threads=1 --iterations=16 --lists=4 --yield=id >> lab2b_list.csv
#	-./lab2_list --threads=4 --iterations=1 --lists=4 --yield=id >> lab2b_list.csv
#	-./lab2_list --threads=4 --iterations=2 --lists=4 --yield=id >> lab2b_list.csv
#	-./lab2_list --threads=4 --iterations=4 --lists=4 --yield=id >> lab2b_list.csv
#	-./lab2_list --threads=4 --iterations=8 --lists=4 --yield=id >> lab2b_list.csv
#	-./lab2_list --threads=4 --iterations=16 --lists=4 --yield=id >> lab2b_list.csv
#	-./lab2_list --threads=8 --iterations=1 --lists=4 --yield=id >> lab2b_list.csv
#	-./lab2_list --threads=8 --iterations=2 --lists=4 --yield=id >> lab2b_list.csv
#	-./lab2_list --threads=8 --iterations=4 --lists=4 --yield=id >> lab2b_list.csv
#	-./lab2_list --threads=8 --iterations=8 --lists=4 --yield=id >> lab2b_list.csv
#	-./lab2_list --threads=8 --iterations=16 --lists=4 --yield=id >> lab2b_list.csv
#	-./lab2_list --threads=12 --iterations=1 --lists=4 --yield=id >> lab2b_list.csv
#	-./lab2_list --threads=12 --iterations=2 --lists=4 --yield=id >> lab2b_list.csv
#	-./lab2_list --threads=12 --iterations=4 --lists=4 --yield=id >> lab2b_list.csv
#	-./lab2_list --threads=12 --iterations=8 --lists=4 --yield=id >> lab2b_list.csv
#	-./lab2_list --threads=12 --iterations=16 --lists=4 --yield=id >> lab2b_list.csv
#	-./lab2_list --threads=16 --iterations=1 --lists=4 --yield=id >> lab2b_list.csv
#	-./lab2_list --threads=16 --iterations=2 --lists=4 --yield=id >> lab2b_list.csv
#	-./lab2_list --threads=16 --iterations=4 --lists=4 --yield=id >> lab2b_list.csv
#	-./lab2_list --threads=16 --iterations=8 --lists=4 --yield=id >> lab2b_list.csv
#	-./lab2_list --threads=16 --iterations=16 --lists=4 --yield=id >> lab2b_list.csv

profile: default 
	rm -f profile.gperf profile.out
	LD_PRELOAD=/u/ma/ugrad/hongen/gperftools/lib/libprofiler.so.1
	CPUPROFILE=./raw.gperf ./lab2_list --iterations=1000 --threads=12 --sync=s
	pprof --text ./lab2_list ./raw.gperf > profile.out
	pprof --list=threadFunction ./lab2_list ./raw.gperf >> profile.out
	rm -f ./raw.gperf

graphs: tests
	chmod 755 ./lab2_list.gp
	./lab2_list.gp

dist: default profile graphs
	tar -cvzf lab2b-205114665.tar.gz lab2_list.c SortedList.c SortedList.h Makefile README *.png lab2b_list.csv data_generator.sh lab2_list.gp profile.out

clean:
	rm -f lab2_list *.tar.gz
