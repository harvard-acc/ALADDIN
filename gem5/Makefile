# Builds libgem5aladdin.a.

.PHONY: lib clean

SRCS = aladdin_sys_connection.cpp aladdin_sys_constants.cpp
OBJS = $(patsubst %.cpp, %.o, $(SRCS))
CFLAGS += -O2 -std=c++0x -c
TARGET = libgem5aladdin.a

lib: $(OBJS)
	ar rvs $(TARGET) $(OBJS)

%.o: %.cpp
	$(CXX) $(CFLAGS) $< -o $@

clean:
	rm -rf *.o $(TARGET)
