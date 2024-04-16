MANDOC1 = man/sskit.1.md
MANDOC5 = man/sstab.5.md

MAN1 = $(basename $(MANDOC1))
MAN5 = $(basename $(MANDOC5))
ZMAN1 = $(addsuffix .gz, $(MAN1))
ZMAN5 = $(addsuffix .gz, $(MAN5))
ZMAN = $(ZMAN1) $(ZMAN5)

INSTALLED_MAN1 = $(addprefix $(DESTDIR)/$(PREFIX)/share/man/man1/, $(notdir $(ZMAN1)))
INSTALLED_MAN5 = $(addprefix $(DESTDIR)/$(PREFIX)/share/man/man5/, $(notdir $(ZMAN5)))
INSTALLED_MANS = $(INSTALLED_MAN1) $(INSTALLED_MAN5)

install_manuals: $(INSTALLED_MANS)
$(DESTDIR)/$(PREFIX)/share/man/man1/%.1.gz: man/%.1.gz
	install -dm 755 $(DESTDIR)/$(PREFIX)/share/man/man1/
	install -Dm 644 $^ $@
$(DESTDIR)/$(PREFIX)/share/man/man5/%.5.gz: man/%.5.gz
	install -dm 755 $(DESTDIR)/$(PREFIX)/share/man/man5/
	install -Dm 644 $^ $@

uninstall_manuals:
	rm -f $(INSTALLED_MANS)

clean_man:
	rm -f $(ZMAN)
mrproper: purge
	rm -rf $(MAN1) $(MAN5)

man: man1 man5
zman: zman1 zman5

man1: $(MAN1)
man/%.1: man/%.1.md
	sed -i "/^footer: sskit/c\footer: sskit $(VERSION)" $^
	sed -i "/^date: /c\date: $(shell date -I)" $^
	pandoc $^ -s -t man -o $@

man5: $(MAN5)
man/%.5: man/%.5.md
	sed -i "/^footer: sstab/c\footer: sskit $(VERSION)" $^
	sed -i "/^date: /c\date: $(shell date -I)" $^
	pandoc $^ -s -t man -o $@

zman1: $(ZMAN1)
man/%.1.gz: man/%.1
	gzip -kf $^

zman5: $(ZMAN5)
man/%.5.gz: man/%.5
	gzip -kf $^
