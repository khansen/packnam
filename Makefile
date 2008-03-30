INSTALL = install
CFLAGS = -Wall -g
LFLAGS =
OBJS = packnam.o

prefix = /usr/local
datarootdir = $(prefix)/share
datadir = $(datarootdir)
exec_prefix = $(prefix)
bindir = $(exec_prefix)/bin
infodir = $(datarootdir)/info
mandir = $(datarootdir)/man

packnam: $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o packnam

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

install: packnam
	$(INSTALL) -m 0755 packnam $(bindir)
	$(INSTALL) -m 0444 packnam.1 $(mandir)/man1

clean:
	rm -f $(OBJS) packnam packnam.exe

.PHONY: clean install
