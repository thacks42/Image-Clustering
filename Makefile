CC=g++
CFLAGS=-g -c -O3 -Wall -std=c++1z -march=native
LDFLAGS=-lsfml-graphics -lsfml-window -lsfml-system
SOURCES=main.cpp image_clusterer.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=clusterer

all: $(SOURCES) $(EXECUTABLE)
    
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm $(OBJECTS)
