
EXT_CFLAGS := -g -O2 -Wall \
		-fPIC

SRC_DIRS := 

SRCS_C := \
	doip_client.c \
	uds_client.c \
	uds_file_linux.c

LDFLAGS +=

SRCS_C += $(foreach d, $(SRC_DIRS), $(wildcard $d/*.c))

OBJS_C := $(SRCS_C:.c=.o)

uds: uds_main.o libuds.so
	$(CC) -o $@ $^ $(LDFLAGS)

libuds.so: $(OBJS_ASM) $(OBJS_C) $(OBJS_CXX)
	$(CC) -shared -Wl,-soname,$@ -o $@ $^ -lpl

install:
	install -d $(DESTDIR)/usr/bin
	install -m 0755 uds $(DESTDIR)/usr/bin
	install -d $(DESTDIR)/usr/lib
	install -m 0644 libuds.so $(DESTDIR)/usr/lib
	install -d $(DESTDIR)/usr/include/uds
	install -m 0644 *.h $(DESTDIR)/usr/include/uds

clean:
	$(RM) uds_main.o uds libuds.so
	$(RM) $(OBJS_ASM) $(OBJS_C) $(OBJS_CXX)
	$(RM) $(OBJS_C:.o=.d) $(OBJS_CXX:.o=.d)

ifeq ($(findstring clean, $(MAKECMDGOALS)),)
-include $(SRCS_C:.c=.d) uds_main.d
endif

$(OBJS_C): %.o : %.c
	$(CC) -c -MD $(CFLAGS) $(EXT_CFLAGS) $< -o $@
 