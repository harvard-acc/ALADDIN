OBJS = file_func.o opcode_func.o generic_func.o 

#CXX = /group/brooks/shao/System/bin/g++
#DEBUG = -DDEBUG
#DEBUG = -g
#export CFLAGS = -c $(DEBUG) -O2 -fPIC -std=c++0x -I/group/brooks/shao/System/include/ \
         #-I$(ALADDIN_HOME)/utils/ \
         #-I$(ALADDIN_HOME)/dddg-builder/ \
         #-I$(ALADDIN_HOME)/machine-model/ \
				 #-I$(BOOST_ROOT)
#export LFLAGS = $(DEBUG) -L/group/brooks/shao/System/lib/ \
         #-L$(ALADDIN_HOME)/lib/\
         #-L$(BOOST_ROOT)/bin.v2/libs/regex/build/gcc-4.8.1/release/threading-multi \
         #-L$(BOOST_ROOT)/bin.v2/libs/graph/build/gcc-4.8.1/release/threading-multi -lboost_graph \
         #-lutils -ldddgbuild -lmachinemodel -lz 
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

