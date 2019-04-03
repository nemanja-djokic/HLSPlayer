PROGRAM = app
CC = g++
CFLAGS = -Wall -Werror

INCLUDE = 
LIBS = -lSDL2main -lSDL2 -lcurl
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
