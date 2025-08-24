cc = gcc
target = ssu_cleanupd
header = header.h
objects = main.o util.o show.o add.o modify.o remove.o help.o daemon.o
cflags = -g

$(target) : $(header) $(objects)
	$(cc) -o $(target) $(objects) $(cflags)

add : cflags += -DADD
add : $(header) $(objects)
	$(cc) -o $(target) $(objects) $(cflags)

modify : cflags += -DMODIFY
modify : $(header) $(objects)
	$(cc) -o $(target) $(objects) $(cflags)

.c.o : 
	$(cc) -c -o $@ $< $(cflags)

clean : 
	rm -rf $(target) $(objects)
	rm -rf test_arranged

tar :
	tar -cvf $(target).tar *.c *.h makefile
