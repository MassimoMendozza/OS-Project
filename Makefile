CFLAGS 		= -g -std=c89 -pedantic -Wall -Wextra -Wpedantic -Wconversion
COMMON_DEPS 	= src/*.h Makefile
COMMON_BUILD_DEP = build/TaxiCab.o build/BinSemaphores.o
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
	$(CC) $(CFLAGS) -c $< -o $@

bin/master: build/master.o build/TaxiCab.o build/BinSemaphores.o $(COMMON_DEPS)
	$(CC) -o bin/master build/master.o $(COMMON_BUILD_DEPS)

bin/taxi: build/taxi.o build/TaxiCab.o build/BinSemaphores.o $(COMMON_DEPS)
	$(CC) -o bin/taxi build/taxi.o $(COMMON_BUILD_DEPS)

bin/source: build/source.o build/TaxiCab.o build/BinSemaphores.o $(COMMON_DEPS)
	$(CC) -o bin/source build/source.o $(COMMON_BUILD_DEPS)

clean:
	$(RM) -r -d build/
	$(RM) -r -d bin/

run: all
	./bin/master