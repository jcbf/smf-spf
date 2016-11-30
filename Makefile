VERSION ?= 2.3

CC = gcc
PREFIX = /usr/local
SBINDIR = $(PREFIX)/sbin
DATADIR = /var/run/smfs
CONFDIR = /etc/mail/smfs
USER = smfs
GROUP = smfs
CFLAGS = -O2 -D_REENTRANT -fomit-frame-pointer -I/usr/local/include 

# Linux
LDFLAGS = -lmilter -lpthread -L/usr/lib/libmilter -L/usr/local/lib -lspf2

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

coverage:
	$(CC) $(CFLAGS) -c smf-spf.c -coverage
	$(CC) -o smf-spf smf-spf.o $(LDFLAGS)  -lgcov
	strip smf-spf
clean:
	rm -f smf-spf.o smf-spf smf.spf.gcno sample coverage.info smf-spf.gcno

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




#
# Making Docker stuff.
#

DOCKER_IMAGE_NAME := smf-spf/smf-spf
DOCKER_TAGS := $(VERSION),latest


# Helper definitions
comma := ,
empty :=
space := $(empty) $(empty)
eq = $(if $(or $(1),$(2)),$(and $(findstring $(1),$(2)),\
                                $(findstring $(2),$(1))),1)



# Build Docker image.
#
# Usage:
#	make docker-image [no-cache=(yes|no)] [VERSION=]

no-cache ?= no
no-cache-arg = $(if $(call eq, $(no-cache), yes), --no-cache, $(empty))

docker-image:
	docker build -t $(DOCKER_IMAGE_NAME):$(VERSION) ./



# Tag Docker image with given tags.
#
# Usage:
#	make docker-tags [VERSION=] [DOCKER_TAGS=t1,t2,...]

tags:
	$(foreach tag, $(subst $(comma), $(space), $(DOCKER_TAGS)), \
		docker tag $(DOCKER_IMAGE_NAME):$(VERSION) $(IMAGE_NAME):$(tag) ;)


# Manually push Docker images to Docker Hub.
#
# Usage:
#	make docker-push [DOCKER_TAGS=t1,t2,...]

docker-push:
	$(foreach tag, $(subst $(comma), $(space), $(DOCKER_TAGS)), \
		docker push $(DOCKER_IMAGE_NAME):$(tag) ;)
