OBJS =  profile_base_address.o
#level_profile.o priority_profile.o register_profile.o  
#CXX = /group/brooks/shao/System/bin/g++
DEBUG =
#DEBUG = -g
CFLAGS = -Wall -c $(DEBUG) -O2 -fPIC -std=c++0x -I/group/brooks/shao/System/include/ \
         -I/group/brooks/shao/System/include/igraph \
         -I../common-funcs/
LFLAGS = -Wall $(DEBUG) -L/group/brooks/shao/System/lib/ \
         -L../lib/\
         -lcommonfuncs -ligraph -lz -lgzstream
libprofiling : $(OBJS)
	$(CXX) -shared -fPIC -o ../lib/libprofiling.so $(OBJS)

level_profile.o : level_profile.h level_profile.cpp
	$(CXX) $(CFLAGS) level_profile.cpp
  
priority_profile.o : priority_profile.h priority_profile.cpp
	$(CXX) $(CFLAGS) priority_profile.cpp

register_profile.o : register_profile.h register_profile.cpp
	$(CXX) $(CFLAGS) register_profile.cpp

profile_base_address.o : profile_base_address.h profile_base_address.cpp
	$(CXX) $(CFLAGS) profile_base_address.cpp

clean:
	rm *.o



