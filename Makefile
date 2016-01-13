CC=gcc
CXX=g++
CFLAGS=-c -Wall
CXXFLAGS=-c -Wall
# Since hidapi lib is different on Linux, using libusb, we are
# linking according to OS version here. This is not very correct
# since we should use autotools instead. We will align it further.
UNAME := $(shell uname)
ifeq ($(UNAME), Linux)
LDFLAGS=-lmicrohttpd -lhidapi-libusb
endif
ifeq ($(UNAME), Darwin)
LDFLAGS=-lmicrohttpd -lhidapi
endif

INCLUDES=-Isrc/include
CSOURCES=src/WebServer.c
CXXSOURCES= src/CHidApi.cpp  src/Commands.cpp
OBJECTS=$(CXXSOURCES:.cpp=.o) $(CSOURCES:.c=.o)
EXECUTABLE=HIDWebServer

all: $(CXXSOURCES) $(CSOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CXX) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@

clean:
	rm -rf src/*o $(EXECUTABLE)
