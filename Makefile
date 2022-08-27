CXX		 = g++-10 -std=c++17  #-DNO_DEFAULT_MAPPING
CXXFLAGS = -Wall
LDFLAGS 	= -pthread
OPTFLAGS	= -O3 -finline-functions -DNDEBUG
CPPFLAGS = $(shell pkg-config --cflags opencv4)
LDLIBS   = $(shell pkg-config --libs opencv4)

TARGETS = main

.PHONY: all clean cleanall
.SUFFIXES: .cpp 


%: %.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(OPTFLAGS) $(CPPFLAGS) $(LDLIBS) -o $@ $<

all		: $(TARGETS)

clean		: 
	rm -f $(TARGETS)
cleanall	: clean
	\rm -f *.o *~
