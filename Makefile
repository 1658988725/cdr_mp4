##subdirs = libpthread libmp4dir 
subdirs = libutility libmp4dir 
subdirs += main
HOME=$(shell pwd)
export HOME
all:
	@for dir in $(subdirs); do make -C $$dir||exit $$?; done

clean:
	@for dir in $(subdirs); do make -C $$dir clean; done
	rm -f lib/*.a
	rm -f lib/*.so

