CONFS = source/sstab
INSTALLED_CONFS = $(addprefix $(DESTDIR)/etc/,$(notdir $(CONFS)))
install_conf: $(INSTALLED_CONFS)

$(DESTDIR)/etc/%: source/%
	install -dm 755 $(dir $@)
	install -Dm 644 $^ $@
