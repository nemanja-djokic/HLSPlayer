PROGRAM = app
CC = g++
CFLAGS = -g3 -Wall -Werror -Wno-deprecated-declarations -Wno-maybe-uninitialized -Wno-unused-function

INCLUDE = 
LIBS = -lSDL2main -lSDL2 -lcurl -lavdevice -lavformat -lavfilter -lavcodec -lswscale -lavutil -lpthread -lz -lswresample
LIBDIR = 

SRC = $(wildcard *.cpp)

OBJ = $(patsubst %.cpp, %.o, $(SRC))

all: $(PROGRAM)

$(PROGRAM): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBDIR) $(LIBS)

%.o: %.cpp
	$(CC) -c -o $@ $< $(CFLAGS) $(INCLUDE)

clean: 
	rm -f $(OBJ) $(PROGRAM)
