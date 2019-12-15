
src = gprof_helper.c
obj = gprof_helper.o

target = ghelper2.so

CC = gcc

DFLAGS = -lpthread -dl
CFLAGS = -fPIC

all: $(target)

$(target): $(obj)
	$(CC) -shared -fPIC $(obj) -o $@ $(DFLAGS)

$(obj): %.o: %.c

.PHONEY: clean

clean:
	rm -rf *.o $(target)
