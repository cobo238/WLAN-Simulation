main: main.o event.o
	g++ -Wall -W -g main.o event.o -o main

main.o: main.cpp
	g++ -Wall -W -g -c main.cpp

event.o: event.cpp event.h
	g++ -Wall -W -g -c event.cpp

clean:
	rm -f main main.o event.o
