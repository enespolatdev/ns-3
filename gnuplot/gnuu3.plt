set terminal pdf
set output "plot3.pdf"
set title "Points Only"
set xlabel "time"
set ylabel "amplitude"
plot "data.txt" using 1:2 with points title "Ramp Signal", \
     "data.txt" using 1:3 with points title "DC Signal", \
     "data.txt" using 1:4 with points title "Random Signal"
