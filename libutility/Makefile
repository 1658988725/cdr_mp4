include ../config.mk
include ../server.mk

TARGET = libutility.a

all : list.o 

	$(AR) -r $(TARGET) *.o
	cp $(TARGET) $(MYLIB)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $< 

clean :
	/bin/rm -f *.o
	/bin/rm -f $(TARGET)
