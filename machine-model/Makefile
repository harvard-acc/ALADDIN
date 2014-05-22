OBJS = Datapath.o Scratchpad.o
#bus.o L1Dcache.o L2Dcache.o MC_dram.o\
       asicCore.o
#CXX = /group/brooks/shao/System/bin/g++
#DEBUG = -DDEBUG
CFLAGS = -Wall -c $(DEBUG) -O2 -fPIC -std=c++0x -I/group/brooks/shao/System/include/ \
         -I/group/brooks/shao/System/include/igraph \
         -I../common-funcs/
LFLAGS = -Wall $(DEBUG) -L/group/brooks/shao/System/lib/ \
         -L../lib/\
         -lcommonfuncs -ligraph -lz -lgzstream -ldramsim
libuncore : $(OBJS)
	$(CXX) -shared -fPIC -o ../lib/libuncore.so $(OBJS)
	rm *.o

Datapath.o : Datapath.h Datapath.cpp
	$(CXX) $(CFLAGS) Datapath.cpp

Scratchpad.o : Scratchpad.h Scratchpad.cpp
	$(CXX) $(CFLAGS) Scratchpad.cpp

clean:
	rm *.o



