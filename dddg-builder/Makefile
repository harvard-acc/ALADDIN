OBJS = dddg.o 
#CXX = /group/brooks/shao/System/bin/g++
DEBUG =
export CFLAGS = -c $(DEBUG) -O2 -fPIC -std=c++0x -I/group/brooks/shao/System/include/ \
         -I$(ALADDIN_HOME)/utils/ \
         -I$(ALADDIN_HOME)/dddg-builder/ \
         -I$(ALADDIN_HOME)/profiling/ \
         -I$(ALADDIN_HOME)/machine-model/ \
				 -I$(BOOST_ROOT)
export LFLAGS = $(DEBUG) -L/group/brooks/shao/System/lib/ \
         -L$(ALADDIN_HOME)/lib/\
         -L$(BOOST_ROOT)/bin.v2/libs/regex/build/gcc-4.8.1/release/threading-multi \
         -L$(BOOST_ROOT)/bin.v2/libs/graph/build/gcc-4.8.1/release/threading-multi -lboost_graph \
         -lutils -ldddgbuild -lmachinemodel -lz 
#DEBUG = -g
libdddgbuild : $(OBJS)
	$(CXX) -shared -fPIC -o ../lib/libdddgbuild.so $(OBJS)
	rm *.o

dddg.o : dddg.h dddg.cpp
	$(CXX) $(CFLAGS) dddg.cpp
  
clean:
	rm *.o



