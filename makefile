SOURCES = src/sstd.c src/ssmk.c src/sscl.c src/ssct.c #src/ssls.c
SERVICES = src/sstd.service
CONFS = src/sstab
MANDOC1 = man/sstools.1.md
MANDOC5 = man/sstab.5.md

NAME = $(shell grep -m1 PROGRAM $(firstword $(SOURCES)) | cut -d\" -f2)
EXECUTABLE = $(shell grep -m1 EXECUTABLE $(firstword $(SOURCES)) | cut -d\" -f2)
PKGNAME = $(shell grep -m1 PKGNAME $(firstword $(SOURCES)) | cut -d\" -f2)
DESCRIPTION = $(shell grep -m1 DESCRIPTION $(firstword $(SOURCES)) | cut -d\" -f2)
VERSION = $(shell grep -m1 VERSION $(firstword $(SOURCES)) | cut -d\" -f2)
AUTHOR = $(shell grep -m1 AUTHOR $(firstword $(SOURCES)) | cut -d\" -f2)
MAIL := $(shell grep -m1 MAIL $(firstword $(SOURCES)) | cut -d\" -f2 | tr '[A-Za-z]' '[N-ZA-Mn-za-m]')
URL = $(shell grep -m1 URL $(firstword $(SOURCES)) | cut -d\" -f2)
LICENSE = $(shell grep -m1 LICENSE $(firstword $(SOURCES)) | cut -d\" -f2)

PREFIX = '/usr'
DESTDIR = ''

CFLAGS = -lbtrfsutil -std=c11 -Os -Wall -Wextra -pedantic

BINARIES = $(notdir $(basename $(SOURCES)))

MAN1 = $(basename $(MANDOC1))
MAN5 = $(basename $(MANDOC5))
ZMAN1 = $(addsuffix .gz, $(MAN1))
ZMAN5 = $(addsuffix .gz, $(MAN5))
ZMAN = $(ZMAN1) $(ZMAN5)

INSTALLED_BINARIES = $(addprefix $(DESTDIR)$(PREFIX)/bin/,$(BINARIES))
INSTALLED_SERVICES = $(addprefix $(DESTDIR)$(PREFIX)/lib/systemd/system/,$(notdir $(SERVICES)))
INSTALLED_CONFS = $(addprefix $(DESTDIR)/etc/,$(notdir $(CONFS)))
INSTALLED_MAN1 = $(addprefix $(DESTDIR)$(PREFIX)/share/man/man1/, $(notdir $(ZMAN1)))
INSTALLED_MAN5 = $(addprefix $(DESTDIR)$(PREFIX)/share/man/man5/, $(notdir $(ZMAN5)))
INSTALLED_MANS = $(INSTALLED_MAN1) $(INSTALLED_MAN5)

ELFS = $(addsuffix .elf,$(addprefix src/,$(BINARIES)))

PKGEXT=.pkg.tar.zst
ARCHPKG = $(PKGNAME)-$(VERSION)-1-$(shell uname -m)$(PKGEXT)

all: elf zman

install: install_elf LICENSE README.md
	install -Dm 644 LICENSE $(DESTDIR)$(PREFIX)/share/licenses/$(PKGNAME)/COPYING
	install -Dm 644 README.md $(DESTDIR)$(PREFIX)/share/doc/$(PKGNAME)/README

arch_install: install systemd_arch_install_services install_confs install_manuals

%.elf: %.c
	$(CC) $^ -o $@ $(CFLAGS)

elf: $(ELFS)

elf_opti: CFLAGS = -std=c11 -march=native -mtune=native -Wall -pedantic -static -O2
elf_opti: $(ELFS)
install_elf_opti: elf_opti install

debug: CFLAGS = -Wall -ggdb3
debug: src/$(PKGNAME)

install_elf: $(INSTALLED_BINARIES)
$(DESTDIR)$(PREFIX)/bin/%: src/%.elf
	install -dm 755 $(DESTDIR)$(PREFIX)/bin/
	install -Dm 755 $^ $@

systemd_arch_install_services: $(INSTALLED_SERVICES)
$(DESTDIR)$(PREFIX)/lib/systemd/system/%.service: src/%.service
	install -dm 755 $(DESTDIR)$(PREFIX)/lib/systemd/system/
	install -Dm 644 $^ $@

install_confs: $(INSTALLED_CONFS)
$(DESTDIR)/etc/sstab: src/sstab
	install -dm 755 $(DESTDIR)/etc/
	install -Dm 644 $^ $@

install_manuals: $(INSTALLED_MANS)
$(DESTDIR)$(PREFIX)/share/man/man1/%.1.gz: man/%.1.gz
	install -dm 755 $(DESTDIR)$(PREFIX)/share/man/man1/
	install -Dm 644 $^ $@
$(DESTDIR)$(PREFIX)/share/man/man5/%.5.gz: man/%.5.gz
	install -dm 755 $(DESTDIR)$(PREFIX)/share/man/man5/
	install -Dm 644 $^ $@

uninstall:
	rm -f $(INSTALLED_BINARIES)
	rm -f $(PREFIX)/share/licenses/$(PKGNAME)/LICENSE
	rm -f $(PREFIX)/share/doc/$(PKGNAME)/README

arch_uninstall: uninstall arch_uninstall_services

arch_uninstall_services:
	rm -f $(DESTDIR)$(PREFIX)/lib/systemd/system/$(EXECUTABLE)d.service

arch_clean:
	rm -rf pkg
	rm -f PKGBUILD
	rm -f $(PKGNAME)-*$(PKGEXT)

clean: arch_clean
	rm -rf $(ELFS)
	rm -rf $(ZMAN)

purge: clean
	rm -f $(MAN1)

arch_pkg: $(ARCHPKG)
$(ARCHPKG): PKGBUILD makefile LICENSE README.md $(SOURCES) $(SERVICES) $(CONFS)
	makepkg -df PKGDEST=./ BUILDDIR=./ PKGEXT='$(PKGEXT)'
	@echo
	@echo Package done!
	@echo You can install it as root with:
	@echo pacman -U $@

PKGBUILD:
	echo '# Maintainer: Manuel Domínguez López <$(MAIL)>' > $@
	echo '_pkgver_year=2018' >> $@
	echo '_pkgver_month=07' >> $@
	echo '_pkgver_day=26' >> $@
	echo 'pkgname=$(PKGNAME)' >> $@
	echo 'pkgver=$(VERSION)' >> $@
	echo 'pkgrel=1' >> $@
	echo 'pkgdesc="$(DESCRIPTION)"' >> $@
	echo 'arch=("i686" "x86_64")' >> $@
	echo 'makedepends=("btrfs-progs")' >> $@
	echo 'url="$(URL)"' >> $@
	echo 'source=()' >> $@
	echo 'license=("$(LICENSE)")' >> $@
	echo 'backup=(etc/sstab)' >> $@
	echo 'build() {' >> $@
	echo 'cd $$startdir' >> $@
	echo 'make' >> $@
	echo '}' >> $@
	echo 'package() {' >> $@
	echo 'cd $$startdir' >> $@
	echo 'make arch_install DESTDIR=$$pkgdir' >> $@
	echo '}' >> $@

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
	gzip -k $^

zman5: $(ZMAN5)
man/%.5.gz: man/%.5
	gzip -k $^

.PHONY: clean arch_clean uninstall
