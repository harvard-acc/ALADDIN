OBJS = file_func.o iljit_func.o generic_func.o graph_func.o

#CXX = /group/brooks/shao/System/bin/g++
DEBUG =
#DEBUG = -g
CFLAGS = -Wall -fPIC -O2 -std=c++0x -c $(DEBUG) -I/group/brooks/shao/System/include/ -I/group/brooks/shao/System/include/igraph 
LFLAGS = -Wall -std=c++0x $(DEBUG) -L/group/brooks/shao/System/lib/ -ligraph -lz -lgzstream

#file_func : $(OBJS) 
#	$(CXX) -o ../bin/file_func $(OBJS) $(LFLAGS)  
#	rm *.o
libcommonfuncs : $(OBJS)
	$(CXX) -shared -o ../lib/libcommonfuncs.so $(OBJS)

file_func.o : file_func.h file_func.cpp
	$(CXX) $(CFLAGS) file_func.cpp

iljit_func.o : iljit_func.h iljit_func.cpp
	$(CXX) $(CFLAGS) iljit_func.cpp

generic_func.o : generic_func.h generic_func.cpp
	$(CXX) $(CFLAGS) generic_func.cpp

graph_func.o : graph_func.h graph_func.cpp
	$(CXX) $(CFLAGS) graph_func.cpp

clean:
	rm *.o

