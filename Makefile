CPP= g++
CPFLAGS= -std=c++11 

all:		client

client:
	$(CPP) $(CPFLAGS) model.cpp -o model -lm -lsimlib
run:
	./model



