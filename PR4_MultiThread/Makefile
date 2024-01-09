all:
	gcc -g -Wall -o pr4 pr4.c
	gcc -g -Wall -o pr4_p pr4_p.c -lpthread


clean:
	rm pr4 pr4_p

pr4:
	gcc -g -Wall -o pr4 pr4.c

pr4_p:
	gcc -g -Wall -o pr4_p pr4_p.c -lpthread

test:
	./pr4 test.txt 2 1> serial.txt
	./pr4_p test.txt 2 1 > parallel.txt
	diff -y serial.txt parallel.txt


