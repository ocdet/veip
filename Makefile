# Copyright (c) 2012, 2013 Open Cloud Demonstration Experiments Taskforce.
# All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License. You may obtain
# a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.
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
