CC = gcc
CFLAGS = -Wall -pedantic -std=c99 -pthread -D _GNU_SOURCE

SRC_DIR=src
INC_DIR=include
OBJ_DIR=obj
TEST_DIR=test

SRC=$(wildcard $(SRC_DIR)/*.c)
OBJ=$(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC))
INC= -I$(INC_DIR)

farm: $(OBJ)
	$(CC) $(CFLAGS) $(INC) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(INC_DIR)/*.h
	$(CC) -g $(CFLAGS) $(INC) -c $< -o $@

generafile: generafile.o
	$(CC) $(CFLAGS) -o $@ $^

generafile.o: $(TEST_DIR)/generafile.c
	$(CC) -g $(CFLAGS) -c $< -o $@

test: generafile farm
	./$(TEST_DIR)/test.sh

clean:
	rm -f ./*.dat ./*.o ./*.txt generafile
	rm -rf testdir
	rm -f $(OBJ_DIR)/*.o farm farm.sck
