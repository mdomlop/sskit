SOURCE=src/makesnap.c
NAME = $(shell grep -m1 PROGRAM $(SOURCE) | cut -d\" -f2)
EXECUTABLE = $(shell grep -m1 EXECUTABLE $(SOURCE) | cut -d\" -f2)
DESCRIPTION = $(shell grep -m1 DESCRIPTION $(SOURCE) | cut -d\" -f2)
VERSION = $(shell grep -m1 VERSION $(SOURCE) | cut -d\" -f2)
AUTHOR = $(shell grep -m1 AUTHOR $(SOURCE) | cut -d\" -f2)
MAIL := $(shell grep -m1 MAIL $(SOURCE) | cut -d\" -f2 | tr '[A-Za-z]' '[N-ZA-Mn-za-m]')
URL = $(shell grep -m1 URL $(SOURCE) | cut -d\" -f2)
LICENSE = $(shell grep -m1 LICENSE $(SOURCE) | cut -d\" -f2)


PREFIX = '/usr'
DESTDIR = ''

ARCHPKG = $(EXECUTABLE)-$(VERSION)-1-$(shell uname -m).pkg.tar.xz

CFLAGS = -Os -Wall -std=c11 -pedantic -static

src/$(EXECUTABLE): src/$(EXECUTABLE).c

all: elf

elf: src/$(EXECUTABLE)

debug: CFLAGS = -Wall -ggdb3
debug: src/$(EXECUTABLE)

install: src/$(EXECUTABLE) LICENSE README.md
	install -Dm 755 src/$(EXECUTABLE) $(DESTDIR)$(PREFIX)/bin/$(EXECUTABLE)
	install -Dm 644 LICENSE $(DESTDIR)$(PREFIX)/share/licenses/$(EXECUTABLE)/COPYING
	install -Dm 644 README.md $(DESTDIR)$(PREFIX)/share/doc/$(EXECUTABLE)/README

uninstall:
	rm -f $(PREFIX)/bin/$(EXECUTABLE)
	rm -f $(PREFIX)/share/licenses/$(EXECUTABLE)/LICENSE

arch_clean:
	rm -rf pkg
	rm -f PKGBUILD
	rm -f $(ARCHPKG)

clean: arch_clean
	rm -rf src/$(EXECUTABLE)

arch_pkg: $(ARCHPKG)
$(ARCHPKG): PKGBUILD makefile src/$(EXECUTABLE).c LICENSE README.md
	sed -i "s|pkgname=.*|pkgname=$(EXECUTABLE)|" PKGBUILD
	sed -i "s|pkgver=.*|pkgver=$(VERSION)|" PKGBUILD
	sed -i "s|pkgdesc=.*|pkgdesc='$(DESCRIPTION)'|" PKGBUILD
	sed -i "s|url=.*|url='$(URL)'|" PKGBUILD
	sed -i "s|license=.*|license=('$(LICENSE)')|" PKGBUILD
	makepkg -df
	@echo
	@echo Package done!
	@echo You can install it as root with:
	@echo pacman -U $@


PKGBUILD:
	echo '# Maintainer: Manuel Domínguez López <mdomlop at gmail dot com>' > $@
	echo '_pkgver_year=2018' >> $@
	echo '_pkgver_month=07' >> $@
	echo '_pkgver_day=26' >> $@
	echo 'pkgname=makesnap' >> $@
	echo 'pkgver=0.1a' >> $@
	echo 'pkgrel=1' >> $@
	echo 'pkgdesc="Make and manage snapshots in a Btrfs filesystem."' >> $@
	echo 'arch=("i686" "x86_64")' >> $@
	echo 'url="https://github.com/mdomlop/makesnap"' >> $@
	echo 'source=()' >> $@
	echo 'license=("GPLv3+")' >> $@
	echo 'build() {' >> $@
	echo 'cd $$startdir' >> $@
	echo 'make' >> $@
	echo '}' >> $@
	echo 'package() {' >> $@
	echo 'cd $$startdir' >> $@
	echo 'make install DESTDIR=$$pkgdir' >> $@
	echo '}' >> $@

.PHONY: clean arch_clean uninstall
