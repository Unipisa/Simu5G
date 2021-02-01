#!/usr/bin/gnuplot -persist

set terminal postscript enhanced color 
set view map
set size ratio 0.5
set size 0.7,0.7
unset surface

set key top right box opaque font ",14" title "k"

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


# --- PLOT STARTING POINTS --- #

set output 'avgStartingPoints.ps'
set ylabel "Number of starting points"
set key left invert reverse Left
plot [*:*] [0:*] "avg_starting_points.csv" using 1:2 with linespoints ls 2 t col, "" using 1:2:3 with yerrorbars ls 12 notitle,\
                     "" using 1:4 with linespoints ls 7 t col, "" using 1:4:5 with yerrorbars ls 17 notitle,\
                     "" using 1:6 with linespoints ls 6 t col, "" using 1:6:7 with yerrorbars ls 16 notitle,\
                     "" using 1:10 with linespoints ls 4 t col, "" using 1:10:11 with yerrorbars ls 14 notitle,\
                     "" using 1:14 with linespoints ls 3 t col, "" using 1:14:15 with yerrorbars ls 13 notitle,\
                     "" using 1:18 with linespoints ls 8 t col, "" using 1:18:19 with yerrorbars ls 18 notitle
                      

# --- PLOT REQUIRED HOPS --- #                      
                      
set output 'avgHopCount.ps'
set ylabel "Number of hops"
set key left invert reverse Left
plot [*:*] [0:*] "avg_hop_count.csv" using 1:2 with linespoints ls 2 t col, "" using 1:2:3 with yerrorbars ls 12 notitle,\
                     "" using 1:4 with linespoints ls 7 t col, "" using 1:4:5 with yerrorbars ls 17 notitle,\
                     "" using 1:6 with linespoints ls 6 t col, "" using 1:6:7 with yerrorbars ls 16 notitle,\
                     "" using 1:10 with linespoints ls 4 t col, "" using 1:10:11 with yerrorbars ls 14 notitle,\
                     "" using 1:14 with linespoints ls 3 t col, "" using 1:14:15 with yerrorbars ls 13 notitle,\
                     "" using 1:18 with linespoints ls 8 t col, "" using 1:18:19 with yerrorbars ls 18 notitle
                     
# --- PLOT MAX HOPS MOD --- #                      
                      
set output 'avgHopLimitMod.ps'
set ylabel "Number of hops"
set key left invert reverse Left
plot [*:*] [0:*] "avg_hop_limit_mod.csv" using 1:2 with linespoints ls 2 t col, "" using 1:2:3 with yerrorbars ls 12 notitle,\
                     "" using 1:4 with linespoints ls 7 t col, "" using 1:4:5 with yerrorbars ls 17 notitle,\
                     "" using 1:6 with linespoints ls 6 t col, "" using 1:6:7 with yerrorbars ls 16 notitle,\
                     "" using 1:10 with linespoints ls 4 t col, "" using 1:10:11 with yerrorbars ls 14 notitle,\
                     "" using 1:14 with linespoints ls 3 t col, "" using 1:14:15 with yerrorbars ls 13 notitle,\
                     "" using 1:18 with linespoints ls 8 t col, "" using 1:18:19 with yerrorbars ls 18 notitle
                     
# --- PLOT D2D GRANTS --- #                      
                      
set output 'avgPerHopD2DGrants.ps'
set ylabel "Average number of D2D Grants per hop"
set key left invert reverse Left
plot [*:*] [0:*] "avg_d2d_grants.csv" using 1:2 with linespoints ls 2 t col, "" using 1:2:3 with yerrorbars ls 12 notitle,\
                     "" using 1:4 with linespoints ls 7 t col, "" using 1:4:5 with yerrorbars ls 17 notitle,\
                     "" using 1:6 with linespoints ls 6 t col, "" using 1:6:7 with yerrorbars ls 16 notitle,\
                     "" using 1:10 with linespoints ls 4 t col, "" using 1:10:11 with yerrorbars ls 14 notitle,\
                     "" using 1:14 with linespoints ls 3 t col, "" using 1:14:15 with yerrorbars ls 13 notitle,\
                     "" using 1:18 with linespoints ls 8 t col, "" using 1:18:19 with yerrorbars ls 18 notitle
         
set output 'avgTotD2DGrants.ps'
set ylabel "Average number of D2D Grants\nper broadcast"
set key right bottom invert reverse Left
plot [*:*] [0:*] "avg_tot_d2d_grants.csv" using 1:2 with linespoints ls 2 t col smooth csplines, "" using 1:2:3 with yerrorbars ls 12 notitle,\
                     "" using 1:4 with linespoints ls 7 t col smooth csplines, "" using 1:4:5 with yerrorbars ls 17 notitle,\
                     "" using 1:6 with linespoints ls 6 t col smooth csplines, "" using 1:6:7 with yerrorbars ls 16 notitle,\
                     "" using 1:10 with linespoints ls 4 t col smooth csplines, "" using 1:10:11 with yerrorbars ls 14 notitle,\
                     "" using 1:14 with linespoints ls 3 t col smooth csplines, "" using 1:14:15 with yerrorbars ls 13 notitle,\
                     "" using 1:18 with linespoints ls 8 t col smooth csplines, "" using 1:18:19 with yerrorbars ls 18 notitle
                     
set output 'avgMaxD2DGrantsPerHop.ps'
set ylabel "Max number of D2D Grants per round"
set key left top invert reverse Left
plot [*:*] [0:*] "avg_max_d2d_grants.csv" using 1:2 with linespoints ls 2 t col, "" using 1:2:3 with yerrorbars ls 12 notitle,\
                     "" using 1:4 with linespoints ls 7 t col, "" using 1:4:5 with yerrorbars ls 17 notitle,\
                     "" using 1:6 with linespoints ls 6 t col, "" using 1:6:7 with yerrorbars ls 16 notitle,\
                     "" using 1:10 with linespoints ls 4 t col, "" using 1:10:11 with yerrorbars ls 14 notitle,\
                     "" using 1:14 with linespoints ls 3 t col, "" using 1:14:15 with yerrorbars ls 13 notitle,\
                     "" using 1:18 with linespoints ls 8 t col, "" using 1:18:19 with yerrorbars ls 18 notitle
                     
                     

                     
# --- PLOT MAX REL PATH TIME --- #                      
                      
set output 'avgMaxRelPathTime.ps'
set ylabel "Time for computing mr paths [ms]"
set key left invert reverse Left
plot [*:*] [0:*] "avg_maxrelpaths_time.csv" using 1:2 with linespoints ls 2 t col, "" using 1:2:3 with yerrorbars ls 12 notitle,\
                     "" using 1:4 with linespoints ls 7 t col, "" using 1:4:5 with yerrorbars ls 17 notitle,\
                     "" using 1:6 with linespoints ls 6 t col, "" using 1:6:7 with yerrorbars ls 16 notitle,\
                     "" using 1:10 with linespoints ls 4 t col, "" using 1:10:11 with yerrorbars ls 14 notitle,\
                     "" using 1:14 with linespoints ls 3 t col, "" using 1:14:15 with yerrorbars ls 13 notitle,\
                     "" using 1:18 with linespoints ls 8 t col, "" using 1:18:19 with yerrorbars ls 18 notitle
           
# --- PLOT MAX REL PATH TIME PARALLELIZED --- #                      
                      
set output 'avgMaxRelPathTimeParallelized.ps'
set ylabel "Time for computing mr paths  / #UEs [ms]"
set key left invert reverse Left
paral(x,y) = y/x
plot [*:*] [0:*] "avg_maxrelpaths_time.csv" using 1:(paral($1,$2)) with linespoints ls 2 t col, "" using 1:(paral($1,$2)):(paral($1,$3)) with yerrorbars ls 12 notitle,\
                     "" using 1:(paral($1,$4)) with linespoints ls 7 t col, "" using 1:(paral($1,$4)):(paral($1,$5)) with yerrorbars ls 17 notitle,\
                     "" using 1:(paral($1,$6)) with linespoints ls 6 t col, "" using 1:(paral($1,$6)):(paral($1,$7)) with yerrorbars ls 16 notitle,\
                     "" using 1:(paral($1,$10)) with linespoints ls 4 t col, "" using 1:(paral($1,$10)):(paral($1,$11)) with yerrorbars ls 14 notitle,\
                     "" using 1:(paral($1,$14)) with linespoints ls 3 t col, "" using 1:(paral($1,$14)):(paral($1,$15)) with yerrorbars ls 13 notitle,\
                     "" using 1:(paral($1,$18)) with linespoints ls 8 t col, "" using 1:(paral($1,$18)):(paral($1,$19)) with yerrorbars ls 18 notitle

# --- PLOT OPT SOLVING TIME --- #                      
                      
set output 'avgOptSolvingTime.ps'
set ylabel "Time for solving the set covering [ms]"
set key left invert reverse Left
plot [*:*] [0:*] "avg_opt_solving_time.csv" using 1:2 with linespoints ls 2 t col, "" using 1:2:3 with yerrorbars ls 12 notitle,\
                     "" using 1:4 with linespoints ls 7 t col, "" using 1:4:5 with yerrorbars ls 17 notitle,\
                     "" using 1:6 with linespoints ls 6 t col, "" using 1:6:7 with yerrorbars ls 16 notitle,\
                     "" using 1:10 with linespoints ls 4 t col, "" using 1:10:11 with yerrorbars ls 14 notitle,\
                     "" using 1:14 with linespoints ls 3 t col, "" using 1:14:15 with yerrorbars ls 13 notitle,\
                     "" using 1:18 with linespoints ls 8 t col, "" using 1:18:19 with yerrorbars ls 18 notitle
                     
                     
# --- PLOT PROB_DELIVERY_RATIO --- #                      
                      
set output 'avgDeliveryRatioProb.ps'
set ylabel "Prob Delivery Ratio"
set key left invert reverse Left
plot [*:*] [0:1] "avg_delivery_ratio_prob.csv" using 1:2 with linespoints ls 2 t col, "" using 1:2:3 with yerrorbars ls 12 notitle,\
                     "" using 1:4 with linespoints ls 7 t col, "" using 1:4:5 with yerrorbars ls 17 notitle,\
                     "" using 1:6 with linespoints ls 6 t col, "" using 1:6:7 with yerrorbars ls 16 notitle,\
                     "" using 1:10 with linespoints ls 4 t col, "" using 1:10:11 with yerrorbars ls 14 notitle,\
                     "" using 1:14 with linespoints ls 3 t col, "" using 1:14:15 with yerrorbars ls 13 notitle,\
                     "" using 1:18 with linespoints ls 8 t col, "" using 1:18:19 with yerrorbars ls 18 notitle
                     
# --- PLOT DELIVERY_RATIO --- #                      
                      
set output 'avgDeliveryRatio.ps'
set ylabel "Delivery Ratio"
set key left invert reverse Left
plot [*:*] [0:1] "avg_delivery_ratio.csv" using 1:2 with linespoints ls 2 t col, "" using 1:2:3 with yerrorbars ls 12 notitle,\
                     "" using 1:4 with linespoints ls 7 t col, "" using 1:4:5 with yerrorbars ls 17 notitle,\
                     "" using 1:6 with linespoints ls 6 t col, "" using 1:6:7 with yerrorbars ls 16 notitle,\
                     "" using 1:10 with linespoints ls 4 t col, "" using 1:10:11 with yerrorbars ls 14 notitle,\
                     "" using 1:14 with linespoints ls 3 t col, "" using 1:14:15 with yerrorbars ls 13 notitle,\
                     "" using 1:18 with linespoints ls 8 t col, "" using 1:18:19 with yerrorbars ls 18 notitle