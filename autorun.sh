# $1 = path of file
# $2 = command line arguments
# $3 = file name
# $4 = file name for png image (output file name)
# $5 = title

./ns3 run "$1" -- "$2" > "$3" 2>&1
gnuplot -e "name_output='$4'; name_file='$3'; title='$5';" cap.plt