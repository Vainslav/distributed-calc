CC = clang
CFLAGS = -std=c99 -Wall -pedantic -g
SRC_DIR := pa1
BUILD_DIR := build
SOURCES := $(wildcard $(SRC_DIR)/*.c)
HEADERS := $(wildcard $(SRC_DIR)/*.h)
OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCES))
APP := $(BUILD_DIR)/bin/app

.PHONY: all app lint clean run archive test

all: app

app: $(APP)

$(APP): $(OBJECTS)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

lint:
	$(CC) -std=c99 -Wall -pedantic -fsyntax-only $(SOURCES)

run: app
	./$(APP) $(ARGS)

archive:
	tar -czf pa1.tar.gz $(SOURCES) $(HEADERS)

clean:
	$(RM) $(OBJECTS) $(APP) $(BUILD_DIR)/test_ipc

$(BUILD_DIR)/test_ipc: tests/test_ipc.c $(SRC_DIR)/ipc.c $(HEADERS)
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) tests/test_ipc.c $(SRC_DIR)/ipc.c -o $@

test: app lint $(BUILD_DIR)/test_ipc archive
	timeout 10 ./$(BUILD_DIR)/test_ipc
	python3 tests/verify.py
