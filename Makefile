OBJS = aladdin.o
LIBS = libutils.so libdddgbuild.so libmachinemodel.so
EXE = aladdin
export DEBUG = -DDEBUG 
#-DDDEBUG
export CFLAGS = -c $(DEBUG) -O2 -fPIC -std=c++0x -I/group/brooks/shao/System/include/ \
         -I$(ALADDIN_HOME)/utils/ \
         -I$(ALADDIN_HOME)/dddg-builder/ \
         -I$(ALADDIN_HOME)/machine-model/ \
				 -I$(BOOST_ROOT)
export LFLAGS = $(DEBUG) -L/group/brooks/shao/System/lib/ \
         -L$(ALADDIN_HOME)/lib/\
         -L$(BOOST_ROOT)/bin.v2/libs/regex/build/gcc-4.8.1/release/threading-multi \
         -L$(BOOST_ROOT)/bin.v2/libs/graph/build/gcc-4.8.1/release/threading-multi -lboost_graph \
         -lutils -ldddgbuild -lmachinemodel -lz 

all : $(EXE)

$(EXE) : $(LIBS) $(OBJS) 
	$(CXX) -o $(EXE) $(OBJS) $(LFLAGS) 
	rm *.o

libutils.so :
	cd utils && $(MAKE)

libdddgbuild.so :
	cd dddg-builder && $(MAKE)

libmachinemodel.so :
	cd machine-model && $(MAKE)

aladdin.o : aladdin.cpp
	$(CXX) $(CFLAGS) aladdin.cpp

clean:
	rm lib/*
	rm aladdin


