ifeq (,$(PROJECT_ROOT))
PROJECT_ROOT = $(shell pwd)/../..
endif
include ../../COMMON/common.mk

CC	= g++

bin    = $(out)/peerio_srv
objdir = $(objroot)/FWT/server/
c_ext  = c

all: $(out) $(objdir) $(bin)
$(out) $(objdir):
	mkdir -p $@

clean:
	rm -fr  $(objdir) $(bin)

search_wildcard  s := $(addsuffix /*.c,.)
objs := $(patsubst   %.c,$(objdir)%.o,$(wildcard   $(search_wildcard  s)))

$(bin): $(objs)
	$(CXX)  $^ -o $@ -lpthread

$(objs): $(objdir)%.o: %.c
	$(CXX) -c -o $@  -MD $(addprefix -I ,.) $<

include $(wildcard   *.d)


