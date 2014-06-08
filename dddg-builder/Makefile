OBJS = dddg.o 
#CXX = /group/brooks/shao/System/bin/g++
DEBUG =
#DEBUG = -g
#CFLAGS = -Wall -c $(DEBUG) -O2 -fPIC -std=c++0x -I/group/brooks/shao/System/include/ \
         #-I/group/brooks/shao/System/include/igraph \
         #-I../common-funcs/
#LFLAGS = -Wall $(DEBUG) -L/group/brooks/shao/System/lib/ \
         #-L../lib/\
         #-lcommonfuncs -ligraph -lz -lgzstream
CFLAGS = -Wall -c $(DEBUG) -O2 -fPIC -std=c++0x -I/group/brooks/shao/System/include/ \
         -I../common-funcs/
LFLAGS = -Wall $(DEBUG) -L/group/brooks/shao/System/lib/ \
         -L../lib/\
         -lcommonfuncs -lz -lgzstream
libdddgbuild : $(OBJS)
	$(CXX) -shared -fPIC -o ../lib/libdddgbuild.so $(OBJS)
	rm *.o

dddg.o : dddg.h dddg.cpp
	$(CXX) $(CFLAGS) dddg.cpp
  
clean:
	rm *.o



