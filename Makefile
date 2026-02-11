CC = gcc
CFLAGS = -Wall -Wextra $(shell sdl2-config --cflags)
LDFLAGS = -lGL -lGLU -lglut -lIL -lm $(shell sdl2-config --libs) -lSDL2_mixer

TARGET = snake_bizarre
SRC = snake_bizarre.c

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: clean
