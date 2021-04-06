AC_INIT([grepL], [0.1.0], [simon.scatton@epita.fr])

AC_CONFIG_SRCDIR([src/])

AM_INIT_AUTOMAKE([
  -Wall
  -Werror
  foreign
  subdir-objects
])

AM_SILENT_RULES([yes])


AC_PROG_CC
AC_PROG_INSTALL
AM_PROG_AR

AC_CONFIG_MACRO_DIRS([m4])


AC_CHECK_LIB(rationl, regex_compile, [
                AC_SUBST([CRITERION_LIBS])
], [
  AC_MSG_ERROR([Could not find library rationL - get it at https://github.com/TheNaturLFoundation/rationL])
])



AC_ARG_ENABLE([debug], AS_HELP_STRING([--enable-debug], [enable debug build]), [], [])
AS_IF([test "$enable_debug" = "yes"], [
        AC_DEFINE([DEBUG], [1], [debug build])
])

AH_TEMPLATE([DEBUG],
            [Define to 1 if debug is enabled, 0 otherwise])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
# Output the files
AC_OUTPUT
