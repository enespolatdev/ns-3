set terminal pdf
set output "compare.pdf"
set title "Receive Rate and Packets Received
set xlabel "Time (seconds)"
set ylabel "Receive Rate"
plot "DSR.csv" using 1:2 with lines title "DSR" lw 3, "DSDV.csv" using 1:2 with lines title "DSDV" lw 3

set xlabel "Time (seconds)"
set ylabel "Packats Received"
plot "DSR.csv" using 1:3 with lines title "DSR" lw 3, "DSDV.csv" using 1:3 with lines title "DSDV" lw 3
