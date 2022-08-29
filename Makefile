ifndef FF_ROOT 
FF_ROOT		= ${HOME}/fastflow
endif

CXX		 = g++-10 -std=c++17
INCLUDES = -I $(FF_ROOT)
CXXFLAGS = -Wall -g # -DNO_DEFAULT_MAPPING -DBLOCKING_MODE -DFF_BOUNDED_BUFFER

LDFLAGS  = -pthread
OPTFLAGS = -O3 -finline-functions -DNDEBUG

CPPFLAGS = $(shell pkg-config --cflags opencv4)
LDLIBS   = $(shell pkg-config --libs opencv4)
#CPPFLAGS = `pkg-config --cflags opencv4`
#LDLIBS   = `pkg-config --libs opencv4`

TARGETS  = main

.PHONY: all clean cleanall
.SUFFIXES: .cpp 


%: %.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(OPTFLAGS) $(CPPFLAGS) $(LDLIBS) -o $@ $<

all		: $(TARGETS)

clean	: 
	rm -f $(TARGETS)
cleanall	: clean
	\rm -f *.o *~
