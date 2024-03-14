###############
#### SSKit ####
###############

SOURCES = src/sskd.c src/ssmk.c src/sscl.c src/ssct.c src/ssls.c
HEADERS = src/sskit.h
CONFS = src/sstab
MANDOC1 = man/sskit.1.md
MANDOC5 = man/sstab.5.md

NAME = $(shell grep -m1 PROGRAM $(firstword $(HEADERS)) | cut -d\" -f2)
EXECUTABLE = $(shell grep -m1 EXECUTABLE $(firstword $(HEADERS)) | cut -d\" -f2)
PKGNAME = $(shell grep -m1 PKGNAME $(firstword $(HEADERS)) | cut -d\" -f2)
DESCRIPTION = $(shell grep -m1 PKGDESCRIPTION $(firstword $(HEADERS)) | cut -d\" -f2)
VERSION = $(shell grep -m1 VERSION $(firstword $(HEADERS)) | cut -d\" -f2)
AUTHOR = $(shell grep -m1 AUTHOR $(firstword $(HEADERS)) | cut -d\" -f2)
MAIL := $(shell grep -m1 MAIL $(firstword $(HEADERS)) | cut -d\" -f2 | tr '[A-Za-z]' '[N-ZA-Mn-za-m]')
URL = $(shell grep -m1 URL $(firstword $(HEADERS)) | cut -d\" -f2)
LICENSE = $(shell grep -m1 LICENSE $(firstword $(HEADERS)) | cut -d\" -f2)

PREFIX = 'usr'
DESTDIR = ''

CFLAGS = -lbtrfsutil -std=c11 -Os -Wall -Wextra -pedantic

BINARIES = $(notdir $(basename $(SOURCES)))

MAN1 = $(basename $(MANDOC1))
MAN5 = $(basename $(MANDOC5))
ZMAN1 = $(addsuffix .gz, $(MAN1))
ZMAN5 = $(addsuffix .gz, $(MAN5))
ZMAN = $(ZMAN1) $(ZMAN5)

INSTALLED_BINARIES = $(addprefix $(DESTDIR)/$(PREFIX)/bin/,$(BINARIES))
INSTALLED_CONFS = $(addprefix $(DESTDIR)/etc/,$(notdir $(CONFS)))
INSTALLED_MAN1 = $(addprefix $(DESTDIR)/$(PREFIX)/share/man/man1/, $(notdir $(ZMAN1)))
INSTALLED_MAN5 = $(addprefix $(DESTDIR)/$(PREFIX)/share/man/man5/, $(notdir $(ZMAN5)))
INSTALLED_MANS = $(INSTALLED_MAN1) $(INSTALLED_MAN5)

ELFS = $(addsuffix .elf,$(addprefix src/,$(BINARIES)))


all: elf zman

install: install_elf LICENSE README.md install_confs install_manuals
	install -Dm 644 LICENSE $(DESTDIR)/$(PREFIX)/share/licenses/$(PKGNAME)/COPYING
	install -Dm 644 README.md $(DESTDIR)/$(PREFIX)/share/doc/$(PKGNAME)/README

%.elf: %.c $(HEADERS)
	$(CC) $< -o $@ $(CFLAGS)

elf: $(ELFS)

elf_opti: CFLAGS = -std=c11 -march=native -mtune=native -Wall -pedantic -static -O2
elf_opti: $(ELFS)
install_elf_opti: elf_opti install

debug: CFLAGS = -Wall -ggdb3
debug: src/$(PKGNAME)


install_elf: $(INSTALLED_BINARIES)
$(DESTDIR)/$(PREFIX)/bin/%: src/%.elf
	install -dm 755 $(DESTDIR)/$(PREFIX)/bin/
	install -Dm 755 $^ $@


install_confs: $(INSTALLED_CONFS)
$(DESTDIR)/etc/sstab: src/sstab
	install -dm 755 $(DESTDIR)/etc/
	install -Dm 644 $^ $@

install_manuals: $(INSTALLED_MANS)
$(DESTDIR)/$(PREFIX)/share/man/man1/%.1.gz: man/%.1.gz
	install -dm 755 $(DESTDIR)/$(PREFIX)/share/man/man1/
	install -Dm 644 $^ $@
$(DESTDIR)/$(PREFIX)/share/man/man5/%.5.gz: man/%.5.gz
	install -dm 755 $(DESTDIR)/$(PREFIX)/share/man/man5/
	install -Dm 644 $^ $@


uninstall: uninstall_manuals
	rm -f $(INSTALLED_BINARIES)
	rm -f /$(PREFIX)/share/licenses/$(PKGNAME)/LICENSE
	rm -f /$(PREFIX)/share/doc/$(PKGNAME)/README


uninstall_manuals:
	rm -f $(INSTALLED_MANS)


clean: clean_arch clean_debian clean_ocs clean_dinit clean_systemd
	rm -f $(ELFS)
	rm -f $(ZMAN)

purge: clean purge_arch purge_debian purge_ocs purge_dinit purge_systemd

mrproper: purge
	rm -rf $(MAN1) $(MAN5)


man: man1 man5
zman: zman1 zman5

man1: $(MAN1)
man/%.1: man/%.1.md
	pandoc $^ -s -t man -o $@

man5: $(MAN5)
man/%.5: man/%.5.md
	pandoc $^ -s -t man -o $@

zman1: $(ZMAN1)
man/%.1.gz: man/%.1
	gzip -kf $^

zman5: $(ZMAN5)
man/%.5.gz: man/%.5
	gzip -kf $^

include arch.mk
include debian.mk
include ocs.mk

pkg_dinit_arch:
	cd dist/dinit; make pkg_arch
pkg_dinit_debian:
	cd dist/dinit; make pkg_debian
pkg_systemd_arch:
	cd dist/systemd; make pkg_arch
pkg_systemd_debian:
	cd dist/systemd; make pkg_debian

clean_dinit:
	cd dist/dinit; make clean
clean_systemd:
	cd dist/systemd; make clean
purge_dinit:
	cd dist/dinit; make purge
purge_systemd:
	cd dist/systemd; make purge

pkg: pkg_arch pkg_debian pkg_ocs pkg_dinit_arch pkg_dinit_debian pkg_systemd_arch pkg_dinit_debian

.PHONY: clean arch_clean uninstall
