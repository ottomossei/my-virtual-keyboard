CXX = g++
CXXFLAGS = -std=c++11

all: MyVirtualKeyboard

MyVirtualKeyboard: main.o
	$(CXX) $(CXXFLAGS) -o MyVirtualKeyboard main.o

main.o: main.cpp
	$(CXX) $(CXXFLAGS) -c main.cpp

clean:
	rm -f *.o MyVirtualKeyboard
