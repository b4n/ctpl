## 
## 
## Copyright (C) 2007-2009 Colomban "Ban" Wendling <ban@herbesfolles.org>
##
## This program is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>.
## 
##


APPNAME = ctpl
EXEC    = $(APPNAME)

CC ?= gcc


PKGS = libmb glib-2.0

#~ LIBMB_DIR     = ../libs/libmb/
#~ LIBMB_CFLAGS  = -I$(LIBMB_DIR)src
#~ LIBMB_LDFLAGS = $(LIBMB_DIR)src/mb.o #-L$(LIBMB_DIR) -lmb
SRCDIR = src/

CCFLAGS = 
CFLAGS  = -g -ansi -pedantic -W -Wall -Wunused -Wunreachable-code -Werror-implicit-function-declaration \
          $(shell pkg-config --cflags $(PKGS)) \
          #$(LIBMB_CFLAGS)
LDFLAGS = $(shell pkg-config --libs $(PKGS)) \
          #$(LIBMB_LDFLAGS)

SRCS = $(wildcard $(SRCDIR)*.c)
OBJS = $(SRCS:.c=.o)


all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.c %.h Makefile
	$(CC) -c $(CCFLAGS) $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJS)

distclean: clean
	rm -f $(EXEC)
