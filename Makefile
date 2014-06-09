OBJS = aladdin.o
LIBS = libcommonfuncs.so libdddgbuild.so libprofiling.so libtraceprofiler.so libuncore.so
EXE = aladdin
CFLAGS = -c $(DEBUG) -O2 -fPIC -std=c++0x -I/group/brooks/shao/System/include/ \
         -I./common-funcs/ \
         -I./dddg-builder/ \
         -I./profiling/ \
         -I./machine-model/ \
         -I./profiling/init-stats/  -I$(BOOST_ROOT)
LFLAGS = $(DEBUG) -L/group/brooks/shao/System/lib/ \
         -L./lib/\
         -L$(BOOST_ROOT)/bin.v2/libs/regex/build/gcc-4.8.1/release/threading-multi \
         -L$(BOOST_ROOT)/bin.v2/libs/graph/build/gcc-4.8.1/release/threading-multi -lboost_graph \
         -lcommonfuncs -ldddgbuild -lprofiling -ltraceprofiler \
				 -luncore -lz -lgzstream 
				 #-lmethodddg -ldramsim -lscheduling -ltransformation 

all : $(EXE)

$(EXE) : $(LIBS) $(OBJS) 
	$(CXX) -o $(EXE) $(OBJS) $(LFLAGS) 
	rm *.o

libcommonfuncs.so :
	cd common-funcs && $(MAKE)

libdddgbuild.so :
	cd dddg-builder && $(MAKE)

libprofiling.so :
	cd profiling && $(MAKE)

libtraceprofiler.so :
	cd profiling/init-stats && $(MAKE)

libuncore.so :
	cd machine-model && $(MAKE)

aladdin.o : aladdin.cpp
	$(CXX) $(CFLAGS) aladdin.cpp

clean:
	rm aladdin
	rm lib/*



