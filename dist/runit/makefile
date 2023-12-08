PKGNAME_BASE=sstd
PKGNAME=sstd-runit
VERSION=1
DARCHI = $(shell dpkg --print-architecture)
DEBIANDIR = $(PKGNAME)-$(VERSION)_$(DARCHI)
DEBIANPKG = $(DEBIANDIR).deb

DEBIANDEPS = sskit

install: $(DESTDIR)/etc/sv/$(PKGNAME_BASE)/run

$(DESTDIR)/etc/sv/$(PKGNAME_BASE)/run: run
	install -dm 755 $(DESTDIR)/etc/sv/$(PKGNAME_BASE)/
	install -Dm 644 $^ $@

$(DEBIANDIR)/DEBIAN:
	mkdir -p -m 0775 $@

$(DEBIANDIR)/DEBIAN/copyright: ../../copyright $(DEBIANDIR)/DEBIAN
	cp $< $@

$(DEBIANDIR)/DEBIAN/control: control $(DEBIANDIR)/DEBIAN
	cp $< $@

pkg_debian: $(DEBIANPKG)
$(DEBIANPKG): $(DEBIANDIR)
	cp README.md $(DEBIANDIR)/DEBIAN/README
	dpkg-deb --build --root-owner-group $(DEBIANDIR)

$(DEBIANDIR): $(DEBIANDIR)/DEBIAN/control $(DEBIANDIR)/DEBIAN/copyright
	make install DESTDIR=$(DEBIANDIR)
	sed -i "s/Installed-Size:.*/Installed-Size:\ $$(du -ks $(DEBIANDIR) | cut -f1)/" $<
clean_debian:
	rm -rf DEBIAN DEBIANTEMP $(DEBIANDIR) $(DEBIANPKG)
