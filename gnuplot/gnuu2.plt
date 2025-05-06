set terminal pdf
set output "plot2.pdf"
set title "Impulse Style"
set xlabel "time"
set ylabel "amplitude"
plot "data.txt" using 1:2 with impulses title "Ramp Signal", \
     "data.txt" using 1:3 with impulses title "DC Signal", \
     "data.txt" using 1:4 with impulses title "Random Signal"
