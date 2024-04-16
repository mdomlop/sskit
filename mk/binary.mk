NAME = $(shell grep -m1 PROGRAM $(firstword $(INFO)) | cut -d\" -f2)
EXECUTABLE = $(shell grep -m1 EXECUTABLE $(firstword $(INFO)) | cut -d\" -f2)
DESCRIPTION = $(shell grep -m1 DESCRIPTION $(firstword $(INFO)) | cut -d\" -f2)
VERSION = $(shell grep -m1 VERSION $(firstword $(INFO)) | cut -d\" -f2)
AUTHOR = $(shell grep -m1 AUTHOR $(firstword $(INFO)) | cut -d\" -f2)
MAIL := $(shell grep -m1 MAIL $(firstword $(INFO)) | cut -d\" -f2 | tr '[A-Za-z]' '[N-ZA-Mn-za-m]')
URL = $(shell grep -m1 URL $(firstword $(INFO)) | cut -d\" -f2)
LICENSE = $(shell grep -m1 LICENSE $(firstword $(INFO)) | cut -d\" -f2)
PKGNAME = $(shell grep -m1 PKGNAME $(firstword $(INFO)) | cut -d\" -f2)
PKGDESCRIPTION = $(shell grep -m1 PKGDESCRIPTION $(firstword $(INFO)) | cut -d\" -f2)

PREFIX = '/usr/local'
DESTDIR = ''

# Requires:
# Debian: libbtrfsutil-dev
# Arch: libbtrfsutil

#CFLAGS = -O2 -Wall -ansi -pedantic -static --std=c18
#CFLAGS := -O2 -Wall -ansi -pedantic
CFLAGS := -std=c11 -Os -Wall -Wextra -pedantic
#CFLAGS_OPTI := $(CFLAGS) -march=native -mtune=native -O3
#CFLAGS_STATIC := $(CFLAGS) -static
#CFLAGS_OPTI_STATIC := $(CFLAGS_OPTI) -static
#CFLAGS_DEBUG := -Wall -ggdb3

LDLIBS = -lbtrfsutil

BINARIES = $(notdir $(basename $(SOURCES)))
INSTALLED_BINARIES = $(addprefix $(DESTDIR)/$(PREFIX)/bin/,$(BINARIES))

DOCS = $(basename README.md changelog.md)
INSTALLED_DOCS = $(addprefix $(DESTDIR)/$(PREFIX)/share/doc/$(PKGNAME)/,$(DOCS))
INSTALLED_LICENSE = $(DESTDIR)$(PREFIX)/share/licenses/$(PKGNAME)/COPYING

ELFS = $(addsuffix .elf,$(addprefix source/,$(BINARIES)))


default: elf zman
debug: elf_debug

all_opti: elf_opti
all_static: elf_static
all_opti_static: elf_opti_static
all_bin: default all_opti all_static all_opti_static debug
all_pkg: pkg_arch pkg_debian pkg_ocs pkg_termux
all: all_bin all_pkg

elf: $(ELFS)

%.elf: %.c
	$(CC) $< -o $@ $(CFLAGS) $(LDLIBS)


# Markdown to share/doc/
$(DESTDIR)/$(PREFIX)/share/doc/$(PKGNAME)/%: %.md
	install -dm 755 $(DESTDIR)/$(PREFIX)/share/doc/$(PKGNAME)/
	install -Dm 644 $^ $@

$(INSTALLED_LICENSE): LICENSE
	install -Dm 644 $^ $@

install_elf: $(INSTALLED_BINARIES) install_all_docs install_bin_dir install_conf install_manuals
$(DESTDIR)/$(PREFIX)/bin/%: source/%.elf
	install -dm 755 $(DESTDIR)/$(PREFIX)/bin/
	install -Dm 755 $^ $@


install: install_elf install_conf install_all_docs

install_all_docs: install_docs install_license
install_docs: $(INSTALLED_DOCS)
install_license: $(INSTALLED_LICENSE)
install_bin_dir:
	install -dm 755 $(DESTDIR)/$(PREFIX)/bin/

uninstall:
	rm -f $(INSTALLED_BINARIES)
	rm -f $(INSTALLED_DOCS)
	rm -f $(INSTALLED_LICENSE)

clean: clean_arch clean_debian clean_ocs clean_man
	rm -f $(ELFS)

purge: clean purge_arch purge_debian purge_ocs

.PHONY: clean arch_clean uninstall
