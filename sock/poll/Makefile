
SRCS := poll.c
OBJS := $(SRCS:.c=.o)
DEPS := $(SRCS:.c=.dep)
APP := a.out
CC := gcc

all: $(APP)


$(APP): $(OBJS)
	$(CC) $^ -o $@

include $(DEPS)

%.dep: %.c
	$(CC) -M $(CPPFLAGS) $< > $@.$$$$;                  \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

clean:
	rm -rf $(OBJS) $(APP)
