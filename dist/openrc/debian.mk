##############
#   Debian   #
##############

DARCHI = all
DEBIANDIR = $(PKGNAME)-$(VERSION)_$(DARCHI)
DEBIANPKG = $(DEBIANDIR).deb

DEBIANDEPS = sskit

$(DEBIANDIR)/DEBIAN:
	mkdir -p -m 0775 $@

copyright: ../../copyright
	cp $^ $@

$(DEBIANDIR)/DEBIAN/copyright: copyright $(DEBIANDIR)/DEBIAN
	@echo Format: https://www.debian.org/doc/packaging-manuals/copyright-format/1.0/ > $@
	@echo Upstream-Name: $(PKGNAME) >> $@
	@echo "Upstream-Contact: Manuel Domínguez López <$(MAIL)>" >> $@
	@echo Source: $(URL) >> $@
	@echo License: $(LICENSE) >> $@
	@echo >> $@
	@echo 'Files: *' >> $@
	@echo "Copyright: $(YEAR) $(AUTHOR) <$(MAIL)>" >> $@
	@echo License: $(LICENSE) >> $@
	cat $< >> $@


$(DEBIANDIR)/DEBIAN/control: $(DEBIANDIR)/DEBIAN
	echo 'Package: $(PKGNAME)' > $@
	echo 'Version: $(VERSION)' >> $@
	echo 'Architecture: $(DARCHI)' >> $@
	echo 'Depends: $(DEBIANDEPS)' >> $@
	echo 'Description: $(DESCRIPTION)' >> $@
	echo 'Section: openrc-world' >> $@
	echo 'Priority: optional' >> $@
	echo 'Maintainer: $(AUTHOR) <$(MAIL)>' >> $@
	echo 'Homepage: $(URL)' >> $@
	echo 'Installed-Size: 1' >> $@

$(DEBIANDIR)/DEBIAN/README: ../../README.md $(DEBIANDIR)/DEBIAN
	cp $< $@

pkg_debian: $(DEBIANPKG)
$(DEBIANPKG): $(DEBIANDIR)
	dpkg-deb --build --root-owner-group $(DEBIANDIR)

$(DEBIANDIR): $(DEBIANDIR)/DEBIAN/control $(DEBIANDIR)/DEBIAN/copyright $(DEBIANDIR)/DEBIAN/README makefile
	make install DESTDIR=$(DEBIANDIR)
	sed -i "s/Installed-Size:.*/Installed-Size:\ $$(du -ks $(DEBIANDIR) | cut -f1)/" $<

clean_debian:
	rm -rf control copyright DEBIAN DEBIANTEMP $(DEBIANDIR)

purge_debian: clean_debian
	rm -f $(DEBIANPKG)
