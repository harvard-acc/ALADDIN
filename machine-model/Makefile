OBJS = Datapath.o Scratchpad.o
export CFLAGS = -c $(DEBUG) -O2 -fPIC -std=c++0x -I/group/brooks/shao/System/include/ \
         -I$(ALADDIN_HOME)/common-funcs/ \
         -I$(ALADDIN_HOME)/dddg-builder/ \
         -I$(ALADDIN_HOME)/machine-model/ \
				 -I$(BOOST_ROOT)
export LFLAGS = $(DEBUG) -L/group/brooks/shao/System/lib/ \
         -L$(ALADDIN_HOME)/lib/\
         -L$(BOOST_ROOT)/bin.v2/libs/regex/build/gcc-4.8.1/release/threading-multi \
         -L$(BOOST_ROOT)/bin.v2/libs/graph/build/gcc-4.8.1/release/threading-multi -lboost_graph \
         -lcommonfuncs -ldddgbuild -lmachinemodel -lz 
#bus.o L1Dcache.o L2Dcache.o MC_dram.o\
       asicCore.o
#CXX = /group/brooks/shao/System/bin/g++
#DEBUG = -DDEBUG
#CFLAGS = -c $(DEBUG) -O2 -fPIC -std=c++0x -I/group/brooks/shao/System/include/ \
         #-I../common-funcs/ -I$(BOOST_ROOT)
#LFLAGS =  $(DEBUG)  -L/group/brooks/shao/System/lib/ \
         #-L../lib/\
         #-L$(BOOST_ROOT)/bin.v2/libs/regex/build/gcc-4.8.1/release/threading-multi \
         #-L$(BOOST_ROOT)/bin.v2/libs/graph/build/gcc-4.8.1/release/threading-multi -lboost_graph \
         #-lcommonfuncs -lz -lgzstream
libmachinemodel : $(OBJS)
	$(CXX) -shared -fPIC -o ../lib/libmachinemodel.so $(OBJS)
	rm *.o

Datapath.o : Datapath.h Datapath.cpp
	$(CXX) $(CFLAGS) Datapath.cpp

Scratchpad.o : Scratchpad.h Scratchpad.cpp
	$(CXX) $(CFLAGS) Scratchpad.cpp

clean:
	rm *.o



