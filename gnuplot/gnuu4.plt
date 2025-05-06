set terminal pdf
set output "plot4.pdf"
set title "Step Plot"
set xlabel "time"
set ylabel "amplitude"
plot "data.txt" using 1:2 with steps title "Ramp Signal", \
     "data.txt" using 1:3 with steps title "DC Signal", \
     "data.txt" using 1:4 with steps title "Random Signal"
