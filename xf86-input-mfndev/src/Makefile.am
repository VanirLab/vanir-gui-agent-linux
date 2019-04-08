# this is obnoxious:
# -module lets us name the module exactly how we want
# -avoid-version prevents gratuitous .0.0.0 version numbers on the end
# _ladir passes a dummy rpath to libtool so the thing will actually link
# TODO: -nostdlib/-Bstatic/-lgcc platform magic, not installing the .a, etc.
AM_CFLAGS = $(XORG_CFLAGS)

@DRIVER_NAME@_drv_la_LTLIBRARIES = @DRIVER_NAME@_drv.la
@DRIVER_NAME@_drv_la_LDFLAGS = -module -avoid-version
@DRIVER_NAME@_drv_la_LIBADD = -L../../xf86-vanir-common -lxf86-vanir-common
@DRIVER_NAME@_drv_ladir = @inputdir@

INCLUDES=-I$(top_srcdir)/../include/

@DRIVER_NAME@_drv_la_SOURCES = @DRIVER_NAME@.c \
                               @DRIVER_NAME@.h
