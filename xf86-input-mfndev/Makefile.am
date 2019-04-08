AUTOMAKE_OPTIONS = foreign

# Ensure headers are installed below $(prefix) for distcheck
DISTCHECK_CONFIGURE_FLAGS = --with-sdkdir='$${includedir}/xorg'

SUBDIRS = src

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = xorg-vanir.pc

EXTRA_DIST = ChangeLog

MAINTAINERCLEANFILES=ChangeLog

.PHONY: ChangeLog

ChangeLog:
	$(CHANGELOG_CMD)

dist-hook: ChangeLog
