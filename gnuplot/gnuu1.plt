set terminal pdf
set output "plot1.pdf"
set title "Lines + Points (linespoints)"
set xlabel "time"
set ylabel "amplitude"
plot "data.txt" using 1:2 with linespoints title "Ramp Signal" lw 2, \
     "data.txt" using 1:3 with linespoints title "DC Signal" lw 2, \
     "data.txt" using 1:4 with linespoints title "Random Signal" lw 2
