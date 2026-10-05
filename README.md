 Компиляция:
gcc -std=c11 -Wall -Wextra -O2 -o reverse reverse.c
gcc -std=c11 -Wall -Wextra -O2 -o client client.c

Интерактивный ввод: ./client ./reverse out.txt
Ввод из входного файла и запись в выходной файл: ./client ./reverse out.txt < in.txt
