# this is obnoxious:
# -module lets us name the module exactly how we want
# -avoid-version prevents gratuitous .0.0.0 version numbers on the end
# _ladir passes a dummy rpath to libtool so the thing will actually link
# TODO: -nostdlib/-Bstatic/-lgcc platform magic, not installing the .a, etc.

AM_CFLAGS = $(XORG_CFLAGS) $(PCIACCESS_CFLAGS)

dummyqbs_drv_la_LTLIBRARIES = dummyqbs_drv.la
dummyqbs_drv_la_LDFLAGS = -module -avoid-version
dummyqbs_drv_la_LIBADD = $(XORG_LIBS) -L../../xf86-vanir-common -lxf86-vanir-common
dummyqbs_drv_ladir = @moduledir@/drivers

dummyqbs_drv_la_SOURCES = \
         compat-api.h \
         dummy_cursor.c \
         dummy_driver.c \
         dummy.h

if DGA
dummyqbs_drv_la_SOURCES +=	\
         dummy_dga.c
endif
