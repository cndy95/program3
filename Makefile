PROG = viewer

CC = g++
CFLAGS = -g -Iinclude -I/usr/local/include/glm-0.9.8.4
LIBS = -lglfw3 -lSOIL -framework OpenGL -framework cocoa -framework IOKit -framework CoreVideo
OBJS = $(PROG).o LoadShaders.o gl3w.o loadObj.o


all: $(PROG)

gl3w.o: gl3w.c
	gcc -c $(CFLAGS) -o $@ $<

.cpp.o:
	$(CC) -c $(CFLAGS) -o $@ $<

$(PROG): $(OBJS)
	$(CC) -g -o $@ $^ $(LIBS)

clean:
	rm -f *.o $(PROG)
