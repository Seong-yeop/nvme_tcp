NAME=nvme_tcp
CC=gcc
CFLAGS=-pthread -Iinclude -DLOG_USE_COLOR

HDR=include/types.h \
    include/log.h \
    include/nvme.h \
    include/transport.h \
    include/discovery.h \
    include/admin.h \
    include/io.h

OBJ=obj/log.o \
    obj/transport.o \
    obj/nvme.o \
    obj/discovery.o \
    obj/admin.o \
    obj/io.o

$(shell mkdir -p obj)

obj/%.o: src/%.c $(HDR)
	$(CC) $(CFLAGS) -c -o $@ $<

nvme_tcp: src/main.c $(OBJ)
	$(CC) $(CFLAGS) -o $(NAME) $^

clean:
	rm -r obj/ $(NAME)

