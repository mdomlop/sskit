
ARCHI = $(shell uname -m)
PKGEXT=.pkg.tar.zst
ARCHPKG = $(PKGNAME)-$(VERSION)-1-$(ARCHI)$(PKGEXT)

PKGBUILD:
	echo '# Maintainer: Manuel Domínguez López <$(MAIL)>' > $@
	echo '_pkgver_year=2018' >> $@
	echo '_pkgver_month=07' >> $@
	echo '_pkgver_day=26' >> $@
	echo 'pkgname=$(PKGNAME)' >> $@
	echo 'pkgver=$(VERSION)' >> $@
	echo 'pkgrel=1' >> $@
	echo 'pkgdesc="$(DESCRIPTION)"' >> $@
	#echo 'arch=("any")' >> $@
	echo 'arch=("i686" "x86_64")' >> $@
	echo 'makedepends=("btrfs-progs")' >> $@
	#echo 'changelog=ChangeLog' >> $@
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
	echo 'make install DESTDIR=$$pkgdir' >> $@
	echo '}' >> $@

pkg_arch: $(ARCHPKG)
$(ARCHPKG): PKGBUILD makefile LICENSE README.md $(SOURCES) $(SYSTEMD) $(CONFS)
	makepkg -df PKGDEST=./ BUILDDIR=./ PKGEXT='$(PKGEXT)'
	@echo
	@echo Package done!
	@echo You can install it as root with:
	@echo pacman -U $@

clean_arch:
	rm -rf pkg
	rm -f PKGBUILD
	rm -f $(PKGNAME)-*$(PKGEXT)
