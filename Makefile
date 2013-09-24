#
# Makefile for VEIP Prototype.
#
# Copyright(C) 2012, Open Cloud Demonstration Experiments Taskforce.
#

CC = gcc
CFLAG = -g
PROG = veip
#PROG = send
#PROG = recv
OBJS = $(PROG).o subr.o abuf.o pbuf.o debug.o
LIBDIR =
LIBS =
INCDIR = -I.
INCL = veip.h
CORES = core core.* *.core a.out *~ log
DEFS = -DLINUX

default: $(PROG)

install:

clean:
	rm -f $(PROG) $(OBJS) $(CORES)

$(PROG): $(OBJS)
	$(CC) $(CFLAG) -o $(PROG) $(OBJS) $(DEFS) $(INCDIR) $(LIBDIR) $(LIBS)

.c.o:
	$(CC) $(CFLAG) $(DEFS) $(INCDIR) -c $<

$(PROG).o: $(INCL)
subr.o: $(INCL)
abuf.o: $(INCL)
pbuf.o: $(INCL)
debug.o: $(INCL)
