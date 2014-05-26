OBJS = traceProfiler.o 
#CXX = /group/brooks/shao/System/bin/g++
DEBUG =
#DEBUG = -g
CFLAGS = -Wall -c $(DEBUG) -O2 -fPIC -std=c++0x -I/group/brooks/shao/System/include/ \
         -I/group/brooks/shao/System/igraph/include/igraph \
         -I../../common-funcs/
LFLAGS = -Wall $(DEBUG) -L/group/brooks/shao/System/lib/ \
         -L/group/brooks/shao/System/igraph/lib/ \
         -L../../lib/\
         -lcommonfuncs -ligraph -lz -lgzstream
libtraceprofiler : $(OBJS)
	$(CXX) -shared -fPIC -o ../..//lib/libtraceprofiler.so $(OBJS)
	rm *.o

traceProfiler.o : traceProfiler.h traceProfiler.cpp
	$(CXX) $(CFLAGS) traceProfiler.cpp
  
clean:
	rm *.o



