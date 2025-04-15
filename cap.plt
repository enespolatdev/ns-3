ARG1 = name_output
ARG2 = name_file
ARG3 = title

set terminal png size 640,480
set output ARG1
set xlabel "Time (s)"
set ylabel "Congestion Window (bytes)"
set title ARG3
plot ARG2 using 1:2 title ARG3 with linespoints
exit