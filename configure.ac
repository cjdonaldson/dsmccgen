#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(FULL-PACKAGE-NAME, VERSION, BUG-REPORT-ADDRESS)
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE([-Wall -Werror foreign])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
#AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_C_CONST
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STRFTIME
AC_STRUCT_TM
AC_TYPE_SIGNAL
AC_TYPE_PID_T



# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_TIME
AC_CHECK_HEADERS([stdlib.h string.h glib-2.0/glib.h sys/timeb.h])
AC_CHECK_HEADERS([arpa/inet.h netinet/in.h sys/socket.h netdb.h])
AC_CHECK_HEADERS([fcntl.h float.h limits.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_SUBST(CFLAGS)
AC_SUBST(LDFLAGS)

# Checks for library functions.
AC_CHECK_FUNCS([gethostbyname socket select])
AC_CHECK_FUNCS([memset ftime])
AC_CHECK_FUNCS(atexit)
AC_CHECK_FUNCS(gettimeofday)



AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
