CC = gcc
CFLAGS = -Wall -Wextra
TARGET = terminal-rogue

SOURCES = main.c auth.c create_map.c game.c menu.c save_load.c scoreboard.c startup.c
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -lncursesw -lm -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)
