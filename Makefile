RPMS_DIR=rpm/
VERSION := $(shell cat version)

DIST_DOM0 ?= fc13

LIBDIR ?= /usr/lib64
USRLIBDIR ?= /usr/lib
SYSLIBDIR ?= /lib
DATADIR ?= /usr/share
PA_VER ?= $(shell pkg-config --modversion libpulse | cut -d "-" -f 1 || echo 0.0)

help:
	@echo "Vanir GUI main Makefile:" ;\
	    echo "make rpms                 <--- make all rpms and sign them";\
	    echo "make rpms-dom0            <--- create binary rpms for dom0"; \
	    echo "make rpms-vm              <--- create binary rpms for appvm"; \
	    echo; \
	    echo "make clean                <--- clean all the binary files";\
	    echo "make update-repo-current  <-- copy newly generated rpms to vanir yum repo";\
	    echo "make update-repo-current-testing <-- same, but for -current-testing repo";\
	    echo "make update-repo-unstable <-- same, but to -testing repo";\
	    echo "make update-repo-installer -- copy dom0 rpms to installer repo"
	    @exit 0;

appvm: gui-agent/vanir-gui gui-common/vanir-gui-runuser \
	xf86-input-mfndev/src/.libs/vanir_drv.so \
	xf86-video-dummy/src/.libs/dummyqbs_drv.so pulse/module-vchan-sink.so \
	xf86-vanir-common/libxf86-vanir-common.so

gui-agent/vanir-gui:
	(cd gui-agent; $(MAKE))

gui-common/vanir-gui-runuser:
	(cd gui-common; $(MAKE))

xf86-input-mfndev/src/.libs/vanir_drv.so: xf86-vanir-common/libxf86-vanir-common.so
	(cd xf86-input-mfndev && ./autogen.sh && ./configure && make)

xf86-video-dummy/src/.libs/dummyqbs_drv.so: xf86-vanir-common/libxf86-vanir-common.so
	(cd xf86-video-dummy && ./autogen.sh && make)

pulse/module-vchan-sink.so:
	rm -f pulse/pulsecore
	ln -s pulsecore-$(PA_VER) pulse/pulsecore
	$(MAKE) -C pulse module-vchan-sink.so

xf86-vanir-common/libxf86-vanir-common.so:
	$(MAKE) -C xf86-vanir-common libxf86-vanir-common.so

rpms: rpms-dom0 rpms-vm
	rpm --addsign rpm/x86_64/*$(VERSION)*.rpm
	(if [ -d rpm/i686 ] ; then rpm --addsign rpm/i686/*$(VERSION)*.rpm; fi)

rpms-vm:
	rpmbuild --define "_rpmdir rpm/" -bb rpm_spec/gui-vm.spec

tar:
	git archive --format=tar --prefix=vanir-gui/ HEAD -o vanir-gui.tar

clean:
	(cd common && $(MAKE) clean)
	(cd gui-agent && $(MAKE) clean)
	(cd gui-common && $(MAKE) clean)
	$(MAKE) -C pulse clean
	$(MAKE) -C xf86-vanir-common clean
	(cd xf86-input-mfndev; if [ -e Makefile ] ; then \
		$(MAKE) distclean; fi; ./bootstrap --clean || echo )


install: install-rh-agent install-pulseaudio

install-rh-agent: appvm install-common
	install -D appvm-scripts/etc/init.d/vanir-gui-agent \
		$(DESTDIR)/etc/init.d/vanir-gui-agent
	install -m 0644 -D appvm-scripts/vanir-gui-agent.service \
		$(DESTDIR)/$(SYSLIBDIR)/systemd/system/vanir-gui-agent.service
	install -m 0644 -D appvm-scripts/etc/sysconfig/desktop \
		$(DESTDIR)/etc/sysconfig/desktop
	install -m 0755 -D appvm-scripts/etc/X11/xinit/xinitrc.d/vanir-keymap.sh \
		$(DESTDIR)/etc/X11/xinit/xinitrc.d/vanir-keymap.sh
	install -D appvm-scripts/etc/X11/xinit/xinitrc.d/20qt-x11-no-mitshm.sh \
		$(DESTDIR)/etc/X11/xinit/xinitrc.d/20qt-x11-no-mitshm.sh
	install -D appvm-scripts/etc/X11/xinit/xinitrc.d/20qt-gnome-desktop-session-id.sh \
		$(DESTDIR)/etc/X11/xinit/xinitrc.d/20qt-gnome-desktop-session-id.sh

install-xfce:
	install -D appvm-scripts/etc/X11/xinit/xinitrc.d/50-xfce-desktop.sh \
		$(DESTDIR)/etc/X11/xinit/xinitrc.d/50-xfce-desktop.sh

install-debian: appvm install-common install-pulseaudio
	install -d $(DESTDIR)/etc/X11/Xsession.d
	install -m 0755 appvm-scripts/etc/X11/xinit/xinitrc.d/vanir-keymap.sh \
		$(DESTDIR)/etc/X11/Xsession.d/90vanir-keymap
	install -m 0644 appvm-scripts/etc/X11/Xsession.d/* $(DESTDIR)/etc/X11/Xsession.d/
	install -d $(DESTDIR)/etc/xdg
	install -m 0644 appvm-scripts/etc/xdg-debian/* $(DESTDIR)/etc/xdg
	install -m 0644 -D appvm-scripts/vanir-gui-agent.service \
		$(DESTDIR)/$(SYSLIBDIR)/systemd/system/vanir-gui-agent.service

install-pulseaudio:
	install -D pulse/start-pulseaudio-with-vchan \
		$(DESTDIR)/usr/bin/start-pulseaudio-with-vchan
	install -m 0644 -D pulse/vanir-default.pa \
		$(DESTDIR)/etc/pulse/vanir-default.pa
	install -D pulse/module-vchan-sink.so \
		$(DESTDIR)$(LIBDIR)/pulse-$(PA_VER)/modules/module-vchan-sink.so
	install -m 0644 -D appvm-scripts/etc/tmpfiles.d/vanir-pulseaudio.conf \
		$(DESTDIR)/$(USRLIBDIR)/tmpfiles.d/vanir-pulseaudio.conf
	install -m 0644 -D appvm-scripts/etc/xdgautostart/vanir-pulseaudio.desktop \
		$(DESTDIR)/etc/xdg/autostart/vanir-pulseaudio.desktop

install-common:
	install -D gui-agent/vanir-gui $(DESTDIR)/usr/bin/vanir-gui
	install -D gui-common/vanir-gui-runuser $(DESTDIR)/usr/bin/vanir-gui-runuser
	install -D appvm-scripts/usrbin/vanir-session \
		$(DESTDIR)/usr/bin/vanir-session
	install -D appvm-scripts/usrbin/vanir-run-xorg \
		$(DESTDIR)/usr/bin/vanir-run-xorg
	install -D appvm-scripts/usrbin/vanir-change-keyboard-layout \
		$(DESTDIR)/usr/bin/vanir-change-keyboard-layout
	install -D appvm-scripts/usrbin/vanir-set-monitor-layout \
		$(DESTDIR)/usr/bin/vanir-set-monitor-layout
	install -D xf86-vanir-common/libxf86-vanir-common.so \
		$(DESTDIR)$(LIBDIR)/libxf86-vanir-common.so
	install -D xf86-input-mfndev/src/.libs/vanir_drv.so \
		$(DESTDIR)$(LIBDIR)/xorg/modules/drivers/vanir_drv.so
	install -D xf86-video-dummy/src/.libs/dummyqbs_drv.so \
		$(DESTDIR)$(LIBDIR)/xorg/modules/drivers/dummyqbs_drv.so
	install -m 0644 -D appvm-scripts/etc/X11/xorg-vanir.conf.template \
		$(DESTDIR)/etc/X11/xorg-vanir.conf.template
	install -m 0644 -D appvm-scripts/etc/profile.d/vanir-gui.sh \
		$(DESTDIR)/etc/profile.d/vanir-gui.sh
	install -m 0644 -D appvm-scripts/etc/profile.d/vanir-gui.csh \
		$(DESTDIR)/etc/profile.d/vanir-gui.csh
	install -m 0644 -D appvm-scripts/etc/profile.d/vanir-session.sh \
		$(DESTDIR)/etc/profile.d/vanir-session.sh
	install -m 0644 -D appvm-scripts/etc/tmpfiles.d/vanir-session.conf \
		$(DESTDIR)/$(USRLIBDIR)/tmpfiles.d/vanir-session.conf
	install -m 0644 -D appvm-scripts/etc/securitylimits.d/90-vanir-gui.conf \
		$(DESTDIR)/etc/security/limits.d/90-vanir-gui.conf
ifneq ($(shell lsb_release -is), Ubuntu)
	install -m 0644 -D appvm-scripts/etc/xdg/Trolltech.conf \
		$(DESTDIR)/etc/xdg/Trolltech.conf
endif
	install -m 0644 -D appvm-scripts/vanir-gui-vm.gschema.override \
		$(DESTDIR)$(DATADIR)/glib-2.0/schemas/20_qubes-gui-vm.gschema.override
	install -m 0644 -D appvm-scripts/etc/vanir-rpc/vanir.SetMonitorLayout \
		$(DESTDIR)/etc/vanir-rpc/vanir.SetMonitorLayout
	install -D window-icon-updater/icon-sender \
		$(DESTDIR)/usr/lib/vanir/icon-sender
	install -m 0644 -D window-icon-updater/vanir-icon-sender.desktop \
		$(DESTDIR)/etc/xdg/autostart/vanir-icon-sender.desktop
	install -D -m 0644 appvm-scripts/usr/lib/sysctl.d/30-vanir-gui-agent.conf \
		$(DESTDIR)/usr/lib/sysctl.d/30-vanir-gui-agent.conf
	install -D -m 0644 appvm-scripts/lib/udev/rules.d/70-master-of-seat.rules \
		$(DESTDIR)/$(SYSLIBDIR)/udev/rules.d/70-master-of-seat.rules
ifeq ($(shell lsb_release -is), Debian)
	install -D -m 0644 appvm-scripts/etc/pam.d/vanir-gui-agent.debian \
		$(DESTDIR)/etc/pam.d/vanir-gui-agent
else ifeq ($(shell lsb_release -is), Ubuntu)
	install -D -m 0644 appvm-scripts/etc/pam.d/vanir-gui-agent.debian \
		$(DESTDIR)/etc/pam.d/vanir-gui-agent
else ifeq ($(shell lsb_release -is), Arch)
	install -D -m 0644 appvm-scripts/etc/pam.d/vanir-gui-agent.archlinux \
		$(DESTDIR)/etc/pam.d/vanir-gui-agent
else
	install -D -m 0644 appvm-scripts/etc/pam.d/vanir-gui-agent \
		$(DESTDIR)/etc/pam.d/vanir-gui-agent
endif
	install -d $(DESTDIR)/var/log/vanir
