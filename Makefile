CXX = g++
CXXFLAGS = -std=c++17 -Wall
SRCFILES = $(wildcard *.cpp)
EXEDIR = bin
EXECUTABLE = $(EXEDIR)/webserver
LIBS = -lpthread

build:
	[ -d $(EXEDIR) ] || mkdir $(EXEDIR)
	$(CXX) $(CXXFLAGS) -o $(EXECUTABLE) $(SRCFILES) $(LIBS)
