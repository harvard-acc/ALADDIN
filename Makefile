OBJS = aladdin.o
LIBS = libcommonfuncs.so libdddgbuild.so libprofiling.so libtraceprofiler.so libuncore.so
EXE = aladdin
CFLAGS = -Wall -c $(DEBUG) -O2 -fPIC -std=c++0x -I/group/brooks/shao/System/include/ \
         -I/group/brooks/shao/System/igraph/include/igraph \
         -I./common-funcs/ \
         -I./dddg-builder/ \
         -I./profiling/ \
         -I./machine-model/ \
         -I./profiling/init-stats/ 
LFLAGS = -Wall $(DEBUG) -L/group/brooks/shao/System/igraph/lib/ \
         -L/group/brooks/shao/System/lib/ \
         -L./lib/\
         -lcommonfuncs -ldddgbuild -lprofiling -ltraceprofiler \
				 -ligraph -luncore -lz -lgzstream 
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



