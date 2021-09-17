
EXT_CFLAGS := -g -O2 -Wall -fPIC \
		-Iapi

SRC_DIRS := \
	linux \
	posix

SRCS_C := \
	$(wildcard *.c)

SRCS_C += $(foreach d, $(SRC_DIRS), $(wildcard $d/*.c))

OBJS_C := $(SRCS_C:.c=.o)

#libpl.a: $(OBJS_ASM) $(OBJS_C) $(OBJS_CXX)
#	$(AR) cr $@ $^

libpl.so: $(OBJS_ASM) $(OBJS_C) $(OBJS_CXX)
	$(CC) -shared -Wl,-soname,$@ -o $@ $^ -lpthread

install:
	install -d $(DESTDIR)/usr/lib
	install -m 0644 libpl.so $(DESTDIR)/usr/lib
	install -d $(DESTDIR)/usr/include
	install -m 0644 api/* $(DESTDIR)/usr/include

clean:
	$(RM) libpl.so
	$(RM) $(OBJS_ASM) $(OBJS_C) $(OBJS_CXX)
	$(RM) $(OBJS_C:.o=.d) $(OBJS_CXX:.o=.d)

ifeq ($(findstring clean, $(MAKECMDGOALS)),)
-include $(SRCS_C:.c=.d)
endif

$(OBJS_C): %.o : %.c
	$(CC) -c -MD $(CFLAGS) $(EXT_CFLAGS) $< -o $@
 