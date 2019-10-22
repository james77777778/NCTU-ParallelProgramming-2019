all:pi_parallel.cpp
	g++ -pthread -std=c++11 -O2 -s pi_parallel.cpp -o pi
serial:pi_serial.cpp
	g++ -pthread -std=c++11 -O2 -s pi_serial.cpp -o pi_serial
clean:
	rm -f pi
	rm -f pi_serial