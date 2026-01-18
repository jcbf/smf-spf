VERSION = $(shell grep -E '^\#define\s+VERSION' smf-spf.c | cut -d\" -f2)

CC = gcc
PREFIX = /usr/local
SBINDIR = $(PREFIX)/sbin
DATADIR = /var/run/smfs
CONFDIR = /etc/mail/smfs
USER = smfs
GROUP = smfs
CFLAGS = -O2 -D_REENTRANT -fomit-frame-pointer -Isrc -I/usr/local/include

# Utility module source files
UTIL_SRCS = src/utils/string_utils.c src/utils/logging.c src/utils/memory.c src/utils/ip_utils.c
UTIL_OBJS = $(UTIL_SRCS:.c=.o)

# Config module source files
CONFIG_SRCS = src/config/config.c
CONFIG_OBJS = $(CONFIG_SRCS:.c=.o)

# Unit test files
UNIT_TEST_SRCS = tests/unit/test_string_utils.c tests/unit/test_ip_utils.c tests/unit/test_memory.c tests/unit/test_logging.c tests/unit/test_config.c
UNIT_TEST_OBJS = $(UNIT_TEST_SRCS:.c=.o)
UNIT_TEST_RUNNER = tests/unit/run_unit_tests.o

# Check framework flags
CHECK_CFLAGS = $(shell pkg-config --cflags check)
CHECK_LDFLAGS = $(shell pkg-config --libs check)

# All object files
OBJS = smf-spf.o $(UTIL_OBJS) $(CONFIG_OBJS)

# Linux
LDFLAGS = -lmilter -lpthread -L/usr/lib/libmilter -L/usr/local/lib -lspf2

# FreeBSD
#LDFLAGS = -lmilter -pthread -L/usr/local/lib -lspf2

# Solaris
#LDFLAGS = -lmilter -lpthread -lsocket -lnsl -lresolv -lspf2

# Sendmail v8.11
#LDFLAGS += -lsmutil

all: smf-spf

smf-spf: $(OBJS)
	$(CC) -o smf-spf $(OBJS) $(LDFLAGS)
	strip smf-spf

smf-spf.o: smf-spf.c
	$(CC) $(CFLAGS) -c smf-spf.c

# Pattern rule for utility modules
src/utils/%.o: src/utils/%.c src/utils/%.h
	$(CC) -O2 -D_REENTRANT -fomit-frame-pointer -Isrc -c $< -o $@

# Pattern rule for config module
src/config/%.o: src/config/%.c src/config/%.h
	$(CC) -O2 -D_REENTRANT -fomit-frame-pointer -Isrc -c $< -o $@

coverage: clean
	$(CC) $(CFLAGS) -c smf-spf.c -coverage
	$(foreach src,$(UTIL_SRCS),$(CC) $(CFLAGS) -c $(src) -coverage -o $(src:.c=.o);)
	$(CC) -o smf-spf $(OBJS) $(LDFLAGS) -lgcov
	strip smf-spf

showcov:
	lcov --directory . --capture --output-file coverage.info
	lcov --remove coverage.info 'tests/*' '/usr/*' --output-file coverage.info
	genhtml coverage.info --output-directory out
	lcov --list coverage.info

clean:
	rm -f smf-spf.o smf-spf smf.spf.gcno sample coverage.info smf-spf.gc*
	rm -f $(UTIL_OBJS) src/utils/*.gcno src/utils/*.gcda
	rm -f $(CONFIG_OBJS) src/config/*.gcno src/config/*.gcda
	rm -f $(UNIT_TEST_OBJS) $(UNIT_TEST_RUNNER) tests/unit/run_unit_tests
	rm -rf ./out

# Unit test compilation rules
tests/unit/%.o: tests/unit/%.c
	$(CC) -O2 -D_REENTRANT -Isrc -Isrc/utils -Isrc/config $(CHECK_CFLAGS) -c $< -o $@

# Unit test runner
tests/unit/run_unit_tests: $(UNIT_TEST_OBJS) $(UNIT_TEST_RUNNER) $(UTIL_OBJS) $(CONFIG_OBJS)
	$(CC) -o $@ $(UNIT_TEST_OBJS) $(UNIT_TEST_RUNNER) $(UTIL_OBJS) $(CONFIG_OBJS) $(CHECK_LDFLAGS)

# Run unit tests
unit-tests: tests/unit/run_unit_tests
	./tests/unit/run_unit_tests

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

test: coverage
	tests/bin/run_tests.sh


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
