# Initialize Autoconf
AC_PREREQ([2.60])
AC_INIT([xf86-video-dummy-vanir],
        [0.3.6],
        [http://wiki.vos.org/trac],
        [xf86-video-dummy-vanir])
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR(.)

# Initialize Automake
AM_INIT_AUTOMAKE([foreign dist-bzip2])
AM_MAINTAINER_MODE

# Require xorg-macros: XORG_DEFAULT_OPTIONS
m4_ifndef([XORG_MACROS_VERSION],
          [m4_fatal([must install xorg-macros 1.3 or later before running autoconf/autogen])])
XORG_MACROS_VERSION(1.3)
XORG_DEFAULT_OPTIONS

# Initialize libtool
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

AH_TOP([#include "xorg-server.h"])

# Define a configure option for an alternate module directory
AC_ARG_ENABLE(dga,           AS_HELP_STRING([--disable-dga], [Build DGA extension (default: yes)]), [DGA=$enableval], [DGA=yes])
AC_ARG_WITH(xorg-module-dir, [  --with-xorg-module-dir=DIR ],
                             [ moduledir="$withval" ],
                             [ moduledir="$libdir/xorg/modules" ])
AC_SUBST(moduledir)


# Store the list of server defined optional extensions in REQUIRED_MODULES
XORG_DRIVER_CHECK_EXT(RANDR, randrproto)
XORG_DRIVER_CHECK_EXT(RENDER, renderproto)
XORG_DRIVER_CHECK_EXT(XV, videoproto)

if test "x$DGA" = xyes; then
	XORG_DRIVER_CHECK_EXT(XFreeXDGA, xf86dgaproto)
	AC_DEFINE(USE_DGA, 1, [Support DGA extension])
fi
AC_SUBST([DGA])
AM_CONDITIONAL([DGA], [test "x$DGA" = xyes])

# Obtain compiler/linker options for the driver dependencies
PKG_CHECK_MODULES(XORG, [xorg-server >= 1.0.99.901] xproto fontsproto $REQUIRED_MODULES)

# Checks for libraries.
AC_CHECK_LIB([xengnttab], [xengntshr_open])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADER([xengnttab.h])


DRIVER_NAME=dummy-vanir
AC_SUBST([DRIVER_NAME])

AC_CONFIG_FILES([
                Makefile
                src/Makefile
])
AC_OUTPUT
