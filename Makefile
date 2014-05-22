OBJS = aladdin.o
CFLAGS = -Wall -c $(DEBUG) -O2 -fPIC -std=c++0x -I/group/brooks/shao/System/include/ \
         -I/group/brooks/shao/System/include/igraph \
         -I./common-funcs/ \
         -I./dddg-builder/ \
         -I./profiling/ \
         -I./machine-model/ \
         -I./profiling/init-stats/ 
LFLAGS = -Wall $(DEBUG) -L/group/brooks/shao/System/lib/ \
         -L./lib/\
         -lcommonfuncs -ldddgbuild -lprofiling -ltraceprofiler \
				 -ligraph -luncore -lz -lgzstream 
				 #-lmethodddg -ldramsim -lscheduling -ltransformation 

aladdin : $(OBJS)
	$(CXX) -o ./aladdin $(OBJS) $(LFLAGS) 
	rm *.o

aladdin.o : aladdin.cpp
	$(CXX) $(CFLAGS) aladdin.cpp
  
clean:
	rm aladdin



