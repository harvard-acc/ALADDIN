OBJS = file_func.o opcode_func.o generic_func.o 

libutils : $(OBJS)
	$(CXX) -shared -o ../lib/libutils.so $(OBJS)
	rm *.o

file_func.o : file_func.h file_func.cpp
	$(CXX) $(CFLAGS) file_func.cpp

opcode_func.o : opcode_func.h opcode_func.cpp
	$(CXX) $(CFLAGS) opcode_func.cpp

generic_func.o : generic_func.h generic_func.cpp
	$(CXX) $(CFLAGS) generic_func.cpp

clean:
	rm *.o

