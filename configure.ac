dnl Autoconf initialization and config macros
dnl ***************************************************************************

AC_PREREQ([2.67])
AC_INIT([dubp],[0.1.0],[jeffrey.wildman@gmail.com],[],[])

dnl Come up with a nice explanation for this when it is actually used
dnl AC_CONFIG_HEADERS([config.h])

dnl Source directory sanity check using an arbitrary source file
AC_CONFIG_SRCDIR([src/dubp.c])

dnl install-sh will be searched (and found) here
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([m4])


dnl Automake initialization and config macros
dnl ***************************************************************************

AM_INIT_AUTOMAKE([color-tests subdir-objects silent-rules no-exeext])

AM_SILENT_RULES([yes])


dnl Doxygen initialization and config macros
dnl ***************************************************************************

DX_DOXYGEN_FEATURE([ON])
DX_PDF_FEATURE([OFF])
DX_PS_FEATURE([OFF])
DX_INIT_DOXYGEN([$PACKAGE_NAME],[doc/doxygen.cfg],[doc])
AS_IF([test "${DX_FLAG_doc}" == 1],[enable_documentation=yes],[enable_documentation=no])


dnl Other macros
dnl ***************************************************************************

# Turn off shared libraries during development to cut down on compilation time.
LT_INIT([disable-shared])

AC_PROG_CC
AC_PROG_CC_C_O
AC_PROG_INSTALL

AC_ARG_ENABLE([debug],AS_HELP_STRING([--enable-debug],[add '-g' flag to compiler call [default=yes]]),[enable_debug=$enableval],[enable_debug=yes])
AC_ARG_ENABLE([optimization],AS_HELP_STRING([--enable-optimization],[add '-O2' flag to compiler call [default=no]]),[enable_optimization=$enableval],[enable_optimization=no])

CFLAGS=""

if test "$enable_debug" == "yes"; then
    CFLAGS="-ggdb3 ${CFLAGS}"
fi

if test "$enable_optimization" == "yes"; then
    CFLAGS="-O2 ${CFLAGS}"
fi

dnl Check the libraries that use pkg-config
PKG_PROG_PKG_CONFIG([])
PKG_CHECK_MODULES([LIBNETFILTER_QUEUE], [libnetfilter_queue])
PKG_CHECK_MODULES([LIBNL_3_0], [libnl-3.0])

dnl Add these flags to gcc and g++ 
CPPFLAGS="${CPPFLAGS}"
CFLAGS="${CFLAGS} -pipe -Wall -Wextra"
LDFLAGS="${LDFLAGS}"
LIBS="${LIBS}"


AC_CONFIG_FILES([Makefile \
                 src/Makefile \
                 lib/Makefile \
                 lib/common/Makefile \
                 lib/packetbb/Makefile])
AC_OUTPUT

echo \
    "-------------------------------------------------------------------------------

${PACKAGE_NAME} Version ${PACKAGE_VERSION}

Prefix: '${prefix}'
C Compiler: '${CC} ${CPPFLAGS} ${CFLAGS}'

Package Features:
Debugging Symbols:          ${enable_debug}
Optimization Flags:         ${enable_optimization}
Doxygen Documentation:      ${enable_documentation}

Now type 'make @<:@<target>@:>@' where the optional <target> is:
all                 - build all binaries
install             - install everything

-------------------------------------------------------------------------------"
