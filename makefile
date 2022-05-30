SOURCES = src/makesnap.c
NAME = $(shell grep -m1 PROGRAM $(SOURCES) | cut -d\" -f2)
EXECUTABLE = $(shell grep -m1 EXECUTABLE $(SOURCES) | cut -d\" -f2)
DESCRIPTION = $(shell grep -m1 DESCRIPTION $(SOURCES) | cut -d\" -f2)
VERSION = $(shell grep -m1 VERSION $(SOURCES) | cut -d\" -f2)
AUTHOR = $(shell grep -m1 AUTHOR $(SOURCES) | cut -d\" -f2)
MAIL := $(shell grep -m1 MAIL $(SOURCES) | cut -d\" -f2 | tr '[A-Za-z]' '[N-ZA-Mn-za-m]')
URL = $(shell grep -m1 URL $(SOURCES) | cut -d\" -f2)
LICENSE = $(shell grep -m1 LICENSE $(SOURCES) | cut -d\" -f2)
PKGNAME = $(EXECUTABLE)

PREFIX = '/usr'
DESTDIR = ''

CFLAGS = -Os -Wall -std=c11 -pedantic -static

INSTALLED_BINARIES = $(addprefix $(DESTDIR)$(PREFIX)/bin/,$(EXECUTABLE))

PKGEXT=.pkg.tar.zst
ARCHPKG = $(PKGNAME)-$(VERSION)-1-$(shell uname -m)$(PKGEXT)

ELFS = $(addsuffix .elf,$(addprefix src/,$(EXECUTABLE)))

all: elf

%.elf: %.c
	$(CC) $^ -o $@ $(CFLAGS)

elf: $(ELFS)

elf_opti: CFLAGS = -std=c11 -march=native -mtune=native -Wall -pedantic -static -O2
elf_opti: $(ELFS)
install_elf_opti: elf_opti install

debug: CFLAGS = -Wall -ggdb3
debug: src/$(EXECUTABLE)

install_elf: $(INSTALLED_BINARIES)
$(DESTDIR)$(PREFIX)/bin/%: src/%.elf
	install -dm 755 $(DESTDIR)$(PREFIX)/bin/
	install -Dm 755 $^ $@

install: install_elf LICENSE README.md arch_install_services
	install -Dm 644 LICENSE $(DESTDIR)$(PREFIX)/share/licenses/$(EXECUTABLE)/COPYING
	install -Dm 644 README.md $(DESTDIR)$(PREFIX)/share/doc/$(EXECUTABLE)/README

arch_install_services: src/$(EXECUTABLE)@.service src/$(EXECUTABLE)@.timer
	install -Dm 644 src/$(EXECUTABLE)@.service $(DESTDIR)$(PREFIX)/lib/systemd/system/$(EXECUTABLE)@.service
	install -Dm 644 src/$(EXECUTABLE)@.timer $(DESTDIR)$(PREFIX)/lib/systemd/system/$(EXECUTABLE)@.timer

uninstall:
	rm -f $(INSTALLED_BINARIES)
	rm -f $(PREFIX)/share/licenses/$(PKGNAME)/LICENSE
	rm -f $(PREFIX)/share/doc/$(PKGNAME)/README
	rm -f $(DESTDIR)$(PREFIX)/lib/systemd/system/$(EXECUTABLE)@.service
	rm -f $(DESTDIR)$(PREFIX)/lib/systemd/system/$(EXECUTABLE)@.timer

arch_clean:
	rm -rf pkg
	rm -f PKGBUILD
	rm -f $(ARCHPKG)

clean: arch_clean
	rm -rf $(ELFS)

arch_pkg: $(ARCHPKG)
$(ARCHPKG): PKGBUILD makefile LICENSE README.md $(SOURCES)
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
	echo 'pkgname=$(EXECUTABLE)' >> $@
	echo 'pkgver=$(VERSION)' >> $@
	echo 'pkgrel=1' >> $@
	echo 'pkgdesc="$(DESCRIPTION)"' >> $@
	echo 'arch=("i686" "x86_64")' >> $@
	echo 'url="$(URL)"' >> $@
	echo 'source=()' >> $@
	echo 'license=("$(LICENSE)")' >> $@
	echo 'build() {' >> $@
	echo 'cd $$startdir' >> $@
	echo 'make' >> $@
	echo '}' >> $@
	echo 'package() {' >> $@
	echo 'cd $$startdir' >> $@
	echo 'make install DESTDIR=$$pkgdir' >> $@
	echo '}' >> $@

.PHONY: clean arch_clean uninstall
