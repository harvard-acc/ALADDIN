OBJS = profile_base_address.o traceProfiler.o 
#level_profile.o priority_profile.o register_profile.o  
#CXX = /group/brooks/shao/System/bin/g++
DEBUG =
#DEBUG = -g
#CFLAGS = -Wall -c $(DEBUG) -O2 -fPIC -std=c++0x -I/group/brooks/shao/System/include/ \
         #-I/group/brooks/shao/System/include/igraph \
         #-I../common-funcs/
#LFLAGS = -Wall $(DEBUG)  -L/group/brooks/shao/System/lib/ \
         #-L../lib/\
         #-lcommonfuncs -ligraph -lz -lgzstream
#CFLAGS = -c $(DEBUG) -O2 -fPIC -std=c++0x -I/group/brooks/shao/System/include/ \
         #-I../common-funcs/ -I$(BOOST_ROOT)
#LFLAGS =  $(DEBUG)  -L/group/brooks/shao/System/lib/ \
         #-L../lib/\
         #-L$(BOOST_ROOT)/bin.v2/libs/regex/build/gcc-4.8.1/release/threading-multi \
         #-L$(BOOST_ROOT)/bin.v2/libs/graph/build/gcc-4.8.1/release/threading-multi -lboost_graph \
         #-lcommonfuncs -lz -lgzstream

libprofiling : $(OBJS)
	$(CXX) -shared -fPIC -o ../lib/libprofiling.so $(OBJS)
	rm *.o

profile_base_address.o : profile_base_address.h profile_base_address.cpp
	$(CXX) $(CFLAGS) profile_base_address.cpp

traceProfiler.o : traceProfiler.h traceProfiler.cpp
	$(CXX) $(CFLAGS) traceProfiler.cpp

clean:
	rm *.o



