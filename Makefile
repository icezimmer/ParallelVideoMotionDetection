ifndef FF_ROOT
FF_ROOT		= $(HOME)/fastflow
endif

CXX		= g++-10 -std=c++17  #-DNO_DEFAULT_MAPPING
INCLUDES	= -I $(FF_ROOT) 
CXXFLAGS  	= -Wall `pkg-config --cflags opencv4`

LDFLAGS 	= -pthread
OPTFLAGS	= -O3 -finline-functions -DNDEBUG

TARGETS		=  main


.PHONY: all clean cleanall
.SUFFIXES: .cpp 


%: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(OPTFLAGS) -o $@ $< $(LDFLAGS)

all		: $(TARGETS)

clean		: 
	rm -f $(TARGETS)
cleanall	: clean
	\rm -f *.o *~
