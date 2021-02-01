#!/usr/bin/gnuplot -persist

set terminal postscript enhanced color 
set view map
set size ratio 0.5
set size 0.7,0.7
unset surface

set key top right box opaque font ",14" title "RBs"

set grid back lc rgb "gray" lw 1 lt 1

set style line 1 dt 1 lw 2 pt 3 ps 1.2 lc rgb "#00BB00"
set style line 2 dt 2 lw 2 pt 7 ps 1.2 lc rgb "red"
set style line 3 dt 3 lt 5 lw 2 pt 9 ps 1.2 lc rgb "blue"
set style line 4 dt 4 lw 2 pt 2 ps 1.2 lc rgb "brown"
set style line 5 dt 5 lw 2 pt 4 ps 1.2 lc rgb "#777777"
set style line 6 dt 6 lw 2 pt 5 ps 1.2 lc rgb "orange"
set style line 7 dt 7 lw 2 pt 1 ps 1.2 lc rgb "black"
set style line 8 dt 8 lw 2 pt 6 ps 1.2 lc rgb "purple"

set style line 11 lw 2 lt 1 pt 3 ps 1.2 lc rgb "#00BB00"
set style line 12 lw 2 lt 1 pt 7 ps 1.2 lc rgb "red"
set style line 13 lw 2 lt 1 pt 9 ps 1.2 lc rgb "blue"
set style line 14 lw 2 lt 1 pt 2 ps 1.2 lc rgb "brown"
set style line 15 lw 2 lt 1 pt 4 ps 1.2 lc rgb "#777777"
set style line 16 lw 2 lt 1 pt 5 ps 1.2 lc rgb "orange"
set style line 17 lw 2 lt 1 pt 1 ps 1.2 lc rgb "black"
set style line 18 lw 2 lt 1 pt 6 ps 1.2 lc rgb "purple"


set datafile missing "NaN"
set datafile separator "\t"

# --- format axis --- #
set xtics font ",14"
set ytics font ",14"
set xlabel "N" font ",14"
set ylabel font ",14" offset 1.5


# --- PLOT --- #

set output 'num=0_acked.ps'
set ylabel "Response ratio"
set xlabel "Number of simulated UEs"
plot [*:25] [0:1.2] "numerology=0-acked.csv" using 1:2 with linespoints ls 2 t col, \
                     "" using 1:3 with linespoints ls 7 t col,\
                     "" using 1:4 with linespoints ls 6 t col,\
                     "" using 1:5 with linespoints ls 4 t col
                      

set output 'num=0_rtt.ps'
set ylabel "RTT [ms]"
set xlabel "Number of simulated UEs"
plot [*:25] [0:70] "numerology=0-rtt.csv" using 1:2 with linespoints ls 2 t col, "" using 1:2:3 with yerrorbars ls 12 notitle,\
                     "" using 1:4 with linespoints ls 7 t col, "" using 1:4:5 with yerrorbars ls 17 notitle,\
                     "" using 1:6 with linespoints ls 6 t col, "" using 1:6:7 with yerrorbars ls 16 notitle,\
                     "" using 1:8 with linespoints ls 4 t col, "" using 1:8:9 with yerrorbars ls 14 notitle