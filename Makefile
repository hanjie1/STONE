# Makefile for stone decoder
#

SIMPLE_INC	= simple
SIMPLE_LIB      = simple

EVIO_INC	?= $(CODA)/Linux-x86_64/include
EVIO_LIB	?= $(CODA)/Linux-x86_64/lib

ROOTLIBS	= $(shell root-config --libs)
ROOTGLIBS	= $(shell root-config --glibs)
ROOTINC		= $(shell root-config --incdir)

INCLUDES	= -I$(ROOTINC) -I$(EVIO_INC) -I$(SIMPLE_INC)
CXXFLAGS	= -g -Wall -std=c++11 $(INCLUDES)

LIBS	        = $(ROOTLIBS) -L${SIMPLE_LIB} -lsimple
GLIBS         	= $(ROOTGLIBS) -L/usr/lib64 -lX11

ALL_LIBS	= -L${EVIO_LIB} -levio $(LIBS) $(GLIBS) $(ROOTLIBS)

SRC		= stone.C
HEAD		= $(SRC:.C=.h)
DEPS		= $(SRC:.C=.d)
DECODE_OBJS	= $(SRC:.C=.o)

all: stone

stone: $(DECODE_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $< $(ALL_LIBS)

clean:
	rm -f ${DECODE_OBJS} ${DEPS} *~ stone


.SUFFIXES:
.SUFFIXES: .c .cc .cpp .C .o .d

%.o: %.C
	$(CXX) $(CXXFLAGS) -c $<

%.d: %.C
	@echo Creating dependencies for $<
	@$(SHELL) -ec '$(CXX) -MM $(CXXFLAGS) -c $< \
	| sed '\''s/\($*\)\.o[ :]*/\1.o $@ : /g'\'' > $@; \
	[ -s $@ ] || rm -f $@'

-include $(DEPS)
