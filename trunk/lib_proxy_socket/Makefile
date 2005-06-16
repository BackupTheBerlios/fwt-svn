PROXY_SOCKET_FUNCTIONS = -DPROXY_SOCKET_FUNCTIONS
OVER_TCP	       = -DOVER_TCP
AUTH	               = -DAUTH

ifeq (,$(PROJECT_ROOT))
PROJECT_ROOT = $(shell pwd)/../..
endif

include ../../COMMON/common.mk

ifneq ($(AUTH),)
	BORZOI		= ../../External/borzoi_c
	IBORZOI		= -I$(BORZOI)
	LIBBORZOI	= $(LIBDIR)/libborzoi.a
endif

sources := arraylist.c common.c error_handler.c hashmap.c input_params.c \
           protocol.c server_loop.c vect.c proxy_socket.c

objdir  = $(objroot)/FWT/ProxySocket$(dirsuffix)
libname = $(lib)/lib_proxy_socket$(suffix).a

ifneq (,$(AUTH))
  sources += authenticating.c
endif

objs = $(patsubst %.c, $(objdir)/%.o, $(sources))
deps = $(patsubst %.c, $(objdir)/%.d, $(sources))

all: $(lib) $(objdir) $(libname)
$(lib) $(objdir):
	mkdir -p $@
clean:
	rm -fr  $(objdir)*.d $(objdir)*.o $(libname)

$(libname): $(objs)
	@echo Create $@
	@ar rc $@ $^
	@ranlib $@

$(deps) : $(objdir)

$(objs): $(objdir)/%.o : %.c
	@echo Compile $<
	$(CC) -g -c $< -o $@ $(OVER_TCP) $(AUTH) $(PROXY_SOCKET_FUNCTIONS) $(IBORZOI) $(addprefix -I ,.)

include $(deps)

$(deps): $(objdir)/%.d : %.c
	@echo Make dep for $<
	$(CC) $< -M $(OVER_TCP) $(AUTH) $(PROXY_SOCKET_FUNCTIONS) $(IBORZOI) $(addprefix -I ,.) > $@
