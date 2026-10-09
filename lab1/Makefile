CC      	= gcc
CFLAGS  	= -g
RM      	= rm -f

SRC_DIR 	:= pa1
BUILD_DIR 	:= build
SOURCES 	:= $(shell find $(SRC_DIR)/ -type f -name '*.c')
OBJECTS 	:= $(SOURCES:%.c=$(BUILD_DIR)/%.o)
OBJ_DIRS	:= $(dir $(OBJECTS))

SRC = $(BUILD_DIR)/app

all: app

app: create_dirs $(OBJECTS)
	$(CC) $(CFLAGS) -o $(SRC) $(OBJECTS)

$(BUILD_DIR)/%.o: %.c
	echo $@
	$(CC) $(CFLAGS) -c $< -o $@

create_dirs:
	mkdir -p $(OBJ_DIRS)

lint:
	clang -std=c99 -Wall -pedantic $(SOURCES)

clean:
	$(RM) -rf $(OBJECTS) $(BUILD_DIR) $(SRC)

run: app
	./$(SRC) $(ARGS)