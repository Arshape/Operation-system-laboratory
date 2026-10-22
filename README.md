Компиляция: 
gcc -O2 -pthread -o consistently consistently.c 
./consistently 10000000  
для параллельной версии:
 gcc -O2 -pthread -o parallel parallel.c 
 ./parallel -n 10000000 -t 8 
