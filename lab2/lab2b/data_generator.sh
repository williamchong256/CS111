#!/bin/bash

#NAME: William Chong
#EMAIL: williamchong256@gmail.com
#ID: 205114665


rm -f lab2b_list.csv
touch lab2b_list.csv

#output for graph 1

#throughput for mutex and sync, with 1000 iterations
#for i in 1, 2, 4, 8, 12, 16, 24
#do
#	./lab2_list --threads $i --iterations 1000 --sync=m >> lab2b_list.csv
#done

#for i in 1, 2, 4, 8, 12, 16, 24
#do
#	./lab2_list --threads $i --iterations 1000 --sync=s >> lab2b_list.csv
#done

#4 lists, yield=id, no sync
for i in 1, 4, 8, 12, 16
do
	for j in 1, 2, 4, 8, 16
	do
		./lab2_list --threads $i --iterations $j --lists=4 --yield=id >> lab2b_list.csv 2>/dev/null
	done
done

#4 lists, yield=id, sync = m,
for i in 1, 4, 8, 12, 16
do
	for j in 10, 20, 40, 80
	do 
		./lab2_list --threads $i --iterations $j --lists=4 --yield=id --sync=m >> lab2b_list.csv
	done
done

#4 lists, yield=id, sync=s
for i in 1, 4, 8, 12, 16
do
	for j in 10, 20, 40, 80
	do 
		./lab2_list --threads $i --iterations $j --lists=4 --yield=id --sync=s >> lab2b_list.csv
	done
done



#no yields, 1000 iterations, sync-m
for i in 1, 2, 4, 8, 12, 16, 24
do
	for j in 1, 4, 8, 16
	do 
		./lab2_list --threads $i --iterations=1000 --lists $j --sync=m >> lab2b_list.csv
	done
done

#no yields, 1000 iterations, sync=s
for i in 1, 2, 4, 8, 12, 16, 24
do
        for j in 1, 4, 8, 16 
        do
                ./lab2_list --threads $i --iterations=1000 --lists $j --sync=s >> lab2b_list.csv
        done
done





