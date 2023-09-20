dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])

AC_INIT([pcsc-tools],[1.6.2])
AC_CONFIG_SRCDIR([pcsc_scan.c])
AM_INIT_AUTOMAKE([1.8 dist-bzip2 no-dist-gzip foreign])

dnl create a config.h file (Automake will add -DHAVE_CONFIG_H)
AC_CONFIG_HEADERS([config.h])

AC_SUBST(VERSION)

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_CANONICAL_HOST

dnl Checks for programs.
AC_PROG_INSTALL
AC_PROG_CC

# --disable-gettext
AC_ARG_ENABLE(gettext,
	AS_HELP_STRING([--disable-gettext],[do not use gettext]),
	[ use_gettext="${enableval}" ], [ use_gettext=yes ] )

if test "x$use_gettext" != xno ; then
	AC_CHECK_PROG([found_msgfmt], [msgfmt], [yes])
	if test x$found_msgfmt != xyes; then
	  AC_MSG_ERROR([msgfmt not found. Please install gettext or use --disable-gettext])
	fi
fi
AM_CONDITIONAL(WITH_GETTEXT, test "${use_gettext}" != "no")

# Check for some target-specific stuff
ATR_PARSER="ATR_analysis"
case "$host" in
*-*-darwin*)
	PCSC_LIBS=${PCSC_LIBS:--framework PCSC}
	PCSC_PATH="PCSC/"
	ATR_PARSER="no"
    ;;
*-*-msys)
	PCSC_LIBS=-lwinscard
	ATR_PARSER="no"
	;;
esac

dnl Checks for libraries.
# check pcsc-lite
PKG_CHECK_EXISTS([libpcsclite],
    [PKG_CHECK_MODULES(PCSC, libpcsclite, [],
    [
    if test -f /usr/local/lib/pkgconfig/libpcsclite.pc -a "x$PKG_CONFIG" != x ; then
        AC_MSG_ERROR([use PKG_CONFIG_PATH=/usr/local/lib/pkgconfig ./configure])
      else
        AC_MSG_WARN([install pcsc-lite])
      fi
    ])],
    [AC_MSG_WARN([libpcsclite not found by pkg-config])]
)

saved_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $PCSC_CFLAGS"
PCSC_ERROR_MSG="install pcsc-lite, or use ./configure PCSC_CFLAGS=..."
AC_CHECK_HEADER("$PCSC_PATH"winscard.h,, [AC_MSG_ERROR([$PCSC_ERROR_MSG])])

saved_LIBS="$LIBS"
LIBS="$LIBS $PCSC_LIBS"
AC_MSG_CHECKING([for SCardEstablishContext])
AC_TRY_LINK_FUNC(SCardEstablishContext,
				 [ AC_MSG_RESULT([yes]) ],
				 [ AC_MSG_ERROR([SCardEstablishContext() not found, install pcsc-lite, or use PCSC_LIBS=...  ./configure]) ])
LIBS="$saved_LIBS"
CPPFLAGS="$saved_CPPFLAGS"

# --enable-parser=foobar
AC_ARG_ENABLE(ATRparser,
	AS_HELP_STRING([--enable-ATRparser=foobar], [pathname of the ATR parser to use]),
	[ ATRparser="${enableval}" ], [ ATRparser="no" ] )
if test "$ATRparser" != no
then
	AC_DEFINE_UNQUOTED(ATR_PARSER, "$ATRparser", [ATR parser to use])
fi

dnl Checks for header files.
AC_CHECK_HEADERS(unistd.h time.h string.h stdio.h stdlib.h sys/time.h)

# Substitute
AX_RECURSIVE_EVAL($datarootdir, datarootdir_exp)
pcsc_dir=${datarootdir_exp}/pcsc
AC_SUBST(pcsc_dir)

AC_HEADER_MAJOR
dnl AC_CHECK_FUNCS(mkfifo)

dnl Checks for typedefs, structures, and compiler characteristics.

dnl Checks for library functions.

cat << EOF

pcsc-tools has been configured with following options:

pcsc_dir:   ${pcsc_dir}
ATR parser: ${ATRparser}

EOF

AC_CONFIG_FILES([
	Makefile
	ATR_analysis
	ATR_analysis.1p
	po/Makefile
	gscriptor.desktop
	])
AC_OUTPUT
