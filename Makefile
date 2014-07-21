OBJS = aladdin.o
LIBS = libutils.so libdddgbuild.so libmachinemodel.so
EXE = aladdin
export CFLAGS = -c $(DEBUG) -O2 -fPIC -std=c++0x\
         -I$(ALADDIN_HOME)/utils/ \
         -I$(ALADDIN_HOME)/dddg-builder/ \
         -I$(ALADDIN_HOME)/machine-model/ \
         -I$(BOOST_ROOT)
export LFLAGS = $(DEBUG) \
         -L$(ALADDIN_HOME)/lib/\
         -lutils -ldddgbuild -lmachinemodel -lz \
         -L$(BOOST_ROOT)/stage/lib  -lboost_graph -lboost_regex

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


