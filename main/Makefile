include ../config.mk
include ../server.mk

EXTRALIB += -lmp4dir -lutility -lmp4v2 -lstdc++
EXTRALIB += -lpthread -lm 
OBJS = cdr_app.o
TARGET = cdr_app

all : $(TARGET)
$(TARGET) : $(OBJS)
	/bin/rm -f -f $(TARGET)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS) $(EXTRALIB)	
ifeq ($(RELEASE),y)	
	$(STRIP) $(TARGET)
endif	

.PHONY: clean

clean: 
	/bin/rm -f -f $(TARGET)
	/bin/rm -f -f *.o
    