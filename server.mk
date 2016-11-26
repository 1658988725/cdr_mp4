ifeq ($(PLATFORM),UBUNTU64)
	CROSS_COMPILE=
	CC=$(CROSS_COMPILE)gcc
	CXX=$(CROSS_COMPILE)g++
	AR=$(CROSS_COMPILE)ar
	RANLIB= $(CROSS_COOMPILE)ranlib
	STRIP=$(CROSS_COMPILE)strip
	HOME=$(shell pwd)
	export HOME
	INC = -I. -I$(HOME)/include -I$(HOME)/include/mp4v2
	CFLAGS = $(INC) -I. -I../include  -std=c99 -pedantic -Wall -Werror -lpthread	-D_GNU_SOURCE
	MYLIB = ../lib 
	OPENSOURCELIB = ../opensourcelib_pc
	LDFLAGS = -L. -L$(MYLIB)  -L$(OPENSOURCELIB)
endif

ifeq ($(PLATFORM),HISI3516CV200)
	CROSS_COMPILE=arm-hisiv300-linux-
	CC=$(CROSS_COMPILE)gcc
	CXX=$(CROSS_COMPILE)g++
	AR=$(CROSS_COMPILE)ar
	RANLIB= $(CROSS_COOMPILE)ranlib
	STRIP=$(CROSS_COMPILE)strip
	HOME=$(shell pwd)
	export HOME
	INC = -I. -I$(HOME)/include -I$(HOME)/include/mp4v2
	CFLAGS = $(INC) -I. -I../include -std=c99 -pedantic -Wall -Werror -lpthread	-D_GNU_SOURCE
	MYLIB = ../lib 
	OPENSOURCELIB = ../opensourcelib_hisi
	LDFLAGS = -L. -L$(MYLIB)  -L$(OPENSOURCELIB)
endif

ifeq ($(RELEASE),y)
	CFLAGS += -O3
else
	CFLAGS += -DDEBUG
endif
