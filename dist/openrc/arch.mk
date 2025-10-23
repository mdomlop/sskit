############
#   Arch   #
############


#ARCHI = $(shell uname -m)
ARCHI = any
PKGEXT=.pkg.tar.zst
ARCHPKG = $(PKGNAME)-$(VERSION)-1-$(ARCHI)$(PKGEXT)


PKGBUILD:
	echo '# Maintainer: $(AUTHOR) <$(MAIL)>' > $@
	echo '_pkgver_year=2018' >> $@
	echo '_pkgver_month=07' >> $@
	echo '_pkgver_day=26' >> $@
	echo 'pkgname=$(PKGNAME)' >> $@
	echo 'pkgver=$(VERSION)' >> $@
	echo 'pkgrel=1' >> $@
	echo 'pkgdesc="$(DESCRIPTION)"' >> $@
	echo 'arch=("$(ARCHI)")' >> $@
	echo 'url="$(URL)"' >> $@
	echo 'source=()' >> $@
	echo 'license=("$(LICENSE)")' >> $@
	echo 'depends=("sskit")' >> $@
	echo 'groups=("openrc-world")' >> $@
	echo 'package() {' >> $@
	echo 'cd $$startdir' >> $@
	echo 'make install DESTDIR=$$pkgdir' >> $@
	echo '}' >> $@

pkg_arch: $(ARCHPKG)
$(ARCHPKG): PKGBUILD makefile sskd $(HEADERS)
	makepkg -df PKGDEST=./ BUILDDIR=./ PKGEXT='$(PKGEXT)'
	@echo
	@echo Package done!
	@echo You can install it as root with:
	@echo pacman -U $@

clean_arch:
	rm -rf pkg
	rm -rf src
	rm -f PKGBUILD

purge_arch: clean_arch
	rm -f $(PKGNAME)-*$(PKGEXT)
