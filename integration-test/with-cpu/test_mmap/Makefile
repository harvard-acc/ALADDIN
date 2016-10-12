.PHONY: all clean

SRCS = test_mmap.c
EXEC = test_mmap

all: $(EXEC)

$(EXEC): $(SRCS)
	gcc -static -std=c99 -g -o $(EXEC) $(SRCS)

clean:
	rm -rf $(EXEC)
