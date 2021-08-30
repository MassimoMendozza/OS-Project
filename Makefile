CFLAGS 		= -g -w -std=c89 -pedantic -D_POSIX_C_SOURCE=200112L
#-Wall -Wextra -Wpedantic -Wconversion
LIBRARY_DEPS = -lpthread -lncursesw
COMMON_DEPS 	= src/*.h Makefile
GRID_SIZE = -D _GRID_WIDTH=60 -D _GRID_HEIGHT=20
COMMON_BUILD_DEP = build/BinSemaphores.o build/taxi.o build/source.o build/shmUtils.o
BUILD_DIR = build
BIN_DIR = bin
MKDIR_P = mkdir -p

all: dirstruct bin/master bin/taxi bin/source

dirstruct: $(BUILD_DIR) $(BIN_DIR)

$(BUILD_DIR):
	$(MKDIR_P) $(BUILD_DIR)

$(BIN_DIR):
	$(MKDIR_P) $(BIN_DIR)

build/%.o: src/%.c $(COMMON_DEPS)
	$(CC) $(GRID_SIZE) $(CFLAGS) -c $< -o $@

bin/master: build/master.o  $(COMMON_DEPS)
	$(CC) -o bin/master build/master.o  $(COMMON_BUILD_DEPS)  $(LIBRARY_DEPS)

bin/taxi: build/taxi.o
	$(CC) -o bin/taxi build/taxi.o $(COMMON_BUILD_DEPS)  $(LIBRARY_DEPS)

bin/source: build/source.o
	$(CC) -o bin/source build/source.o $(COMMON_BUILD_DEPS)  $(LIBRARY_DEPS)

clean:
	$(RM) -r -d build/
	$(RM) -r -d bin/

run: all
	./bin/master