dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])

AC_INIT([pcsc-tools],[1.7.1])
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

AS_IF([test "x$use_gettext" != xno],
	[AC_CHECK_PROG([found_msgfmt], [msgfmt], [yes])
	 AS_IF([test x$found_msgfmt != xyes],
		[AC_MSG_WARN([msgfmt not found. Please install gettext])
		 use_gettext=no])])
AM_CONDITIONAL(WITH_GETTEXT, test "${use_gettext}" != "no")

# Check for some target-specific stuff
ATR_PARSER="ATR_analysis"
AS_CASE(["$host"],
	[*-*-darwin*],
		[PCSC_LIBS=${PCSC_LIBS:--framework PCSC}
		PCSC_PATH="PCSC/"
		ATR_PARSER="no"],
	[*-*-mingw*|*-*-msys],
		[PCSC_LIBS=-lwinscard
		CFLAGS="-Wl,-Bstatic -pthread"
		ATR_PARSER="no"])

dnl Checks for libraries.
# check pcsc-lite
PKG_CHECK_EXISTS([libpcsclite],
    [PKG_CHECK_MODULES(PCSC, libpcsclite, [],
    [
    AS_IF([test -f /usr/local/lib/pkgconfig/libpcsclite.pc -a "x$PKG_CONFIG" != x],
        [AC_MSG_ERROR([use PKG_CONFIG_PATH=/usr/local/lib/pkgconfig ./configure])],
        [AC_MSG_WARN([install pcsc-lite])])
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
	[ ATRparser="${enableval}" ], [ ATRparser="$ATR_PARSER" ] )
AS_IF([test "$ATRparser" != no],
	[AC_DEFINE_UNQUOTED(ATR_PARSER, "$ATRparser", [ATR parser to use])])

dnl Checks for header files.
AC_CHECK_HEADERS(unistd.h time.h string.h stdio.h stdlib.h sys/time.h sysexits.h)

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
CFLAGS:     ${CFLAGS}
CPPFLAGS:   ${CPPFLAGS}

EOF

AC_CONFIG_FILES([
	Makefile
	ATR_analysis
	ATR_analysis.1p
	po/Makefile
	gscriptor.desktop
	])
AC_OUTPUT
