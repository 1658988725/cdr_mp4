include ../config.mk
include ../server.mk

TARGET = libmp4dir.a

all : parsemp4file.o parsemp4dir.o muxermp4.o 

	$(AR) -r $(TARGET) *.o
	cp $(TARGET) $(MYLIB)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

clean :
	/bin/rm -f *.o
	/bin/rm -f $(TARGET)
