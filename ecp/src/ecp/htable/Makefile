include ../common.mk
CFLAGS += -std=gnu89

obj = htable.o hashtable.o hashtable_itr.o


%.o: %.c
	$(CC) $(CFLAGS) -c $<

all: $(obj)
	$(AR) rcs libecpht.a $(obj)

clean:
	rm -f *.o *.a
