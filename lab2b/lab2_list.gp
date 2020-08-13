#! /usr/bin/gnuplot
#
# purpose:
#	 generate data reduction graphs for the multi-threaded list project
#
# input: lab2b_list.csv
#	1. test name
#	2. # threads
#	3. # iterations per thread
#	4. # lists
#	5. # operations performed (threads x iterations x (ins + lookup + delete))
#	6. run time (ns)
#	7. run time per operation (ns)
#	8. lock time per operation
#
# output:
#	lab2b_1.png ... throughput vs. number of threads for mutex and spin-lock sync
#	lab2b_2.png ... mean time per mutex wait and mean time per op for mutex-sync.
#	lab2b_3.png ... successful iterations vs. threads for each sync method.
#	lab2b_4.png ... throughput vs. number of threads for mutex synchronized 
#		partitioned lists.
#	lab2b_5.png ... throughput vs. number of threads for spin-lock-sync 
#		partitioned lists.
# Note:
#	Managing data is simplified by keeping all of the results in a single
#	file.  But this means that the individual graphing commands have to
#	grep to select only the data they want.
#
#	Early in your implementation, you will not have data for all of the
#	tests, and the later sections may generate errors for missing data.
#

# general plot parameters
set terminal png
set datafile separator ","

set title "Graph 1: Throughput vs Number of Threads."
set xlabel "Number of Threads"
set logscale x 2
set ylabel "Throughput (operations/second)"
set logscale y 10
set output 'lab2b_1.png'

# grep out only un-protected, non-yield results
plot \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'with mutex' with linespoints lc rgb 'red', \
     "< grep 'list-none-s,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'with spin-lock' with linespoints lc rgb 'green'


set title "Graph-2: Wait-for-lock Time and Average Operation Time (Mutex Sync)"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Time (ns)"
set logscale y 10
set output 'lab2b_2.png'

plot \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):($7) \
	title 'Mean Time per Op.' with linespoints lc rgb 'red', \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):($8) \
	title 'Mean Time per Mutex Wait' with linespoints lc rgb 'green'


set title "Graph-3: Partitioned List With and Without Sync"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "# of Successful Iterations"
set logscale y 10
set output 'lab2b_3.png'

plot \
    "< grep 'list-id-none,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
	title 'No Synchronization' with points lc rgb 'red', \
    "< grep 'list-id-m,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
	title 'Mutex' with points lc rgb 'green', \
    "< grep 'list-id-s,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
	title 'Spinlock' with points lc rgb 'blue'

  


#throughput vs. number of threads for mutex synchronized partitioned lists.

set title "Graph-4: Throughput for Mutex-Synchronized Partitioned List"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Throughput (ops/sec)"
set logscale y 10
set output 'lab2b_4.png'

plot \
    "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'lists=1' with linespoints lc rgb 'red', \
    "< grep 'list-none-m,[0-9]*,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'lists=4' with linespoints lc rgb 'green', \ 
    "< grep 'list-none-m,[0-9]*,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'lists=8' with linespoints lc rgb 'blue', \
    "< grep 'list-none-m,[0-9]*,1000,12,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'lists=8' with linespoints lc rgb 'orange'




#throughput vs. number of threads for spinlock synchronized partitioned lists.

set title "Graph-5: Throughput for Spinlock-Synchronized Partitioned List"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Throughput (ops/sec)"
set logscale y 10
set output 'lab2b_5.png'

plot \
    "< grep 'list-none-s,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'lists=1' with linespoints lc rgb 'red', \
    "< grep 'list-none-s,[0-9]*,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'lists=4' with linespoints lc rgb 'green', \ 
    "< grep 'list-none-s,[0-9]*,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'lists=8' with linespoints lc rgb 'blue', \
    "< grep 'list-none-s,[0-9]*,1000,12,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'lists=8' with linespoints lc rgb 'orange'



