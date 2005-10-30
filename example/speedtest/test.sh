gcc test.c -I/usr/local//include -L/usr/local//lib -lmpd -g -pg -o test
time ./test  > /dev/null
