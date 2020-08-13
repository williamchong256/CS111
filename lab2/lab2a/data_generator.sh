#!/bin/bash

#NAME: William Chong
#EMAIL: williamchong256@gmail.com
#ID: 205114665

rm -f lab2_add.csv
touch lab2_add.csv

rm -f lab2_list.csv
touch lab2_list.csv

#add-none
for k in 1
do

	for i in 1, 2, 4, 8, 12
	do
		for j in 10, 20, 40, 80, 100, 250, 500, 750, 1000, 4000, 6000, 10000, 50000, 100000
		do
			./lab2_add --threads $i --iterations $j >> lab2_add.csv
		done
	done
done

#add-yield-none
for i in 1, 2, 4, 8, 12
do
	for j in 10, 20, 40, 80, 100, 250, 500, 750, 1000, 4000, 6000, 10000, 50000, 100000
	do
		./lab2_add --threads $i --iterations $j --yield >> lab2_add.csv
	done
done

#add-yield-m
for i in 1, 2, 4, 8, 12
do
	./lab2_add --threads $i --iterations=10000 --yield --sync=m >> lab2_add.csv
done

#add-yield-s
for i in 1, 2, 4, 8, 12
do
        ./lab2_add --threads $i --iterations=1000 --yield --sync=s >> lab2_add.csv
done

#add-yield-c
for i in 1, 2, 4, 8, 12
do
        ./lab2_add --threads $i --iterations=10000 --yield --sync=c >> lab2_add.csv
done

for i in 1, 2, 4, 8, 12
do
        ./lab2_add --threads $i --iterations=10000 --yield --sync=c >> lab2_add.csv
done

for i in 1, 2, 4, 8, 12
do
        ./lab2_add --threads $i --iterations=10000 --yield --sync=c >> lab2_add.csv
done

#add-m
for i in 1, 2, 4, 8, 12
do
        for j in 10000, 100000
        do
                ./lab2_add --threads $i --iterations $j --sync=m >> lab2_add.csv
        done
done

#add-s
for i in 1, 2, 4, 8, 12
do
        for j in 10000, 100000
        do
                ./lab2_add --threads $i --iterations $j --sync=s >> lab2_add.csv
        done
done

#add-c
for i in 1, 2, 4, 8, 12
do
        for j in 10000, 100000
        do
                ./lab2_add --threads $i --iterations $j --sync=c >> lab2_add.csv
        done
done



#LAB2LIST#########

#single thread
for i in 10, 100, 1000, 10000, 20000
do
	./lab2_list --threads 1 --iterations $i >> lab2_list.csv
done


#multiple threads
for i in 2, 4, 8, 12
do
	for j in 1, 10, 100, 1000
	do
		./lab2_list --threads $i --iterations $j >> lab2_list.csv
	done
done

#yield=i
for i in 2, 4, 8, 12
do 
	for j in 1, 2, 4, 8, 16, 32
	do
		./lab2_list --threads $i --iterations $j --yield=i >> lab2_list.csv
	done
done

#yield=d
for i in 2, 4, 8, 12
do
        for j in 1, 2, 4, 8, 16, 32
        do
                ./lab2_list --threads $i --iterations $j --yield=d >> lab2_list.csv
        done
done

#yield=il
for i in 2, 4, 8, 12
do
        for j in 1, 2, 4, 8, 16, 32
        do
                ./lab2_list --threads $i --iterations $j --yield=il >> lab2_list.csv
        done
done

#yield=dl
for i in 2, 4, 8, 12
do
        for j in 1, 2, 4, 8, 16, 32
        do
                ./lab2_list --threads $i --iterations $j --yield=dl >> lab2_list.csv
        done
done

#yield i with sync m
for i in 2, 4, 8, 12
do
        for j in 1, 2, 4, 8, 16, 32
        do
                ./lab2_list --threads $i --iterations $j --yield=i --sync=m >> lab2_list.csv
        done
done

#yield d with sync m
for i in 2, 4, 8, 12
do
        for j in 1, 2, 4, 8, 16, 32
        do
                ./lab2_list --threads $i --iterations $j --yield=d --sync=m >> lab2_list.csv
        done
done

#yield il with sync m
for i in 2, 4, 8, 12
do
        for j in 1, 2, 4, 8, 16, 32
        do
                ./lab2_list --threads $i --iterations $j --yield=il --sync=m >> lab2_list.csv
        done
done

#yield dl with sync m
for i in 2, 4, 8, 12
do
        for j in 1, 2, 4, 8, 16, 32
        do
                ./lab2_list --threads $i --iterations $j --yield=dl --sync=m >> lab2_list.csv
        done
done



#yield i with sync s
for i in 2, 4, 8, 12
do
        for j in 1, 2, 4, 8, 16, 32
        do
                ./lab2_list --threads $i --iterations $j --yield=i --sync=s >> lab2_list.csv
        done
done

#yield d with sync s
for i in 2, 4, 8, 12
do
        for j in 1, 2, 4, 8, 16, 32
        do
                ./lab2_list --threads $i --iterations $j --yield=d --sync=s >> lab2_list.csv
        done
done

#yield il with sync s
for i in 2, 4, 8, 12
do
        for j in 1, 2, 4, 8, 16, 32
        do
                ./lab2_list --threads $i --iterations $j --yield=il --sync=s >> lab2_list.csv
        done
done

#yield dl with sync m
for i in 2, 4, 8, 12
do
        for j in 1, 2, 4, 8, 16, 32
        do
                ./lab2_list --threads $i --iterations $j --yield=dl --sync=s >> lab2_list.csv
        done
done




for i in 1, 2, 4, 8, 12, 16, 24
do
    ./lab2_list --threads $i --iterations=1000 >> lab2_list.csv
done

for i in 1, 2, 4, 8, 12, 16, 24
do
        ./lab2_list --threads $i --iterations=1000 --sync=m >> lab2_list.csv
done

for i in 1, 2, 4, 8, 12, 16, 24
do
        ./lab2_list --threads $i --iterations=1000 --sync=s >> lab2_list.csv
done

