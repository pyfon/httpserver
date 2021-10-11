CXX = g++
CXXFLAGS = -std=c++17 -g -Wall
SRCFILES = $(wildcard *.cpp)
EXECUTABLE = bin/webserver

build:
	$(CXX) $(CXXFLAGS) -o $(EXECUTABLE) $(SRCFILES)
