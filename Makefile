# Compiler
CC = clang

# Automatically find sources
CFILES = $(shell cd src && find -L * -type f -name '*.c')

# Get object files
OBJECTS = $(addprefix bin/, $(CFILES:.c=.o))

# Flags
CFLAGS =
OUT = bin/xwm

all: $(OUT)

run: all
	@Xephyr :1 & sleep 0.1
	@DISPLAY=:1 ./$(OUT)

$(OUT): $(OBJECTS)
	@$(CC) $(OBJECTS) -lX11 -o $(OUT)

bin/%.o: src/%.c
	@mkdir -p bin/
	@echo " CC "$<
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(OUT)