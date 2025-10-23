dist: pkg_dinit_arch pkg_dinit_debian pkg_systemd_arch pkg_systemd_debian

dist_clean: clean_dinit clean_systemd
dist_purge: purge_dinit purge_systemd

pkg_dinit_arch:
	cd dist/dinit; make pkg_arch
pkg_dinit_debian:
	cd dist/dinit; make pkg_debian
pkg_systemd_arch:
	cd dist/systemd; make pkg_arch
pkg_systemd_debian:
	cd dist/systemd; make pkg_debian

clean_dinit:
	cd dist/dinit; make clean
clean_systemd:
	cd dist/systemd; make clean
purge_dinit:
	cd dist/dinit; make purge
purge_systemd:
	cd dist/systemd; make purge
