CC = gcc
PREFIX = /usr/local
SBINDIR = $(PREFIX)/sbin
DATADIR = /var/run/smfs
CONFDIR = /etc/mail/smfs
USER = smfs
GROUP = smfs
CFLAGS = -O2 -D_REENTRANT -fomit-frame-pointer -I/usr/local/include

# Linux
LDFLAGS = -lmilter -lpthread -L/usr/local/lib -lspf2

# FreeBSD
#LDFLAGS = -lmilter -pthread -L/usr/local/lib -lspf2

# Solaris
#LDFLAGS = -lmilter -lpthread -lsocket -lnsl -lresolv -lspf2

# Sendmail v8.11
#LDFLAGS += -lsmutil

all: smf-spf

smf-spf: smf-spf.o
	$(CC) -o smf-spf smf-spf.o $(LDFLAGS)
	strip smf-spf

smf-spf.o: smf-spf.c
	$(CC) $(CFLAGS) -c smf-spf.c

clean:
	rm -f smf-spf.o smf-spf

install:
	@./install.sh
	@cp -f -p smf-spf $(SBINDIR)
	@if test ! -d $(DATADIR); then \
	mkdir -m 700 $(DATADIR); \
	chown $(USER):$(GROUP) $(DATADIR); \
	fi
	@if test ! -d $(CONFDIR); then \
	mkdir -m 755 $(CONFDIR); \
	fi
	@if test ! -f $(CONFDIR)/smf-spf.conf; then \
	cp -p smf-spf.conf $(CONFDIR)/smf-spf.conf; \
	else \
	cp -p smf-spf.conf $(CONFDIR)/smf-spf.conf.new; \
	fi
	@echo Please, inspect and edit the $(CONFDIR)/smf-spf.conf file.
