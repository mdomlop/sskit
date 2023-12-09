####################
#### SSTD inits ####
####################

HEADERS = ../../src/sskit.h

PKGNAME = sskit-dinit
PREFIX = 'etc/dinit.d'
DESTDIR = ''

DESCRIPTION = Dinit service for sstd (part of sskit)
VERSION = 1
AUTHOR = $(shell grep -m1 AUTHOR $(firstword $(HEADERS)) | cut -d\" -f2)
MAIL := $(shell grep -m1 MAIL $(firstword $(HEADERS)) | cut -d\" -f2 | tr '[A-Za-z]' '[N-ZA-Mn-za-m]')
URL = $(shell grep -m1 URL $(firstword $(HEADERS)) | cut -d\" -f2)
LICENSE = $(shell grep -m1 LICENSE $(firstword $(HEADERS)) | cut -d\" -f2)

all: pkg_arch pkg_debian

install: sstd
	install -dm 755 $(DESTDIR)/$(PREFIX)/
	install -Dm 644 $^ $(DESTDIR)/$(PREFIX)/


clean: clean_arch clean_debian
	rm -rf $(DESTDIR)/$(PREFIX)/$(PKGNAME)/

purge: purge_arch purge_debian

include arch.mk
include debian.mk

.PHONY: clean install all
