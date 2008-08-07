# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

# This file is part of libasyncns.
#
# Copyright 2005-2008 Lennart Poettering
#
# libasyncns is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation, either version 2.1 of the
# License, or (at your option) any later version.
#
# libasyncns is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with libasyncns. If not, see
# <http://www.gnu.org/licenses/>.

AC_PREREQ(2.62)
AC_INIT([libasyncns],[0.5],[mznflapaf (at) 0pointer (dot) de])
AC_CONFIG_SRCDIR([libasyncns/asyncns.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign 1.9 -Wall])

AC_SUBST(PACKAGE_URL, [http://0pointer.de/lennart/projects/libasyncns/])

AC_SUBST(LIBASYNCNS_VERSION_INFO, [2:1:2])

if type -p stow > /dev/null && test -d /usr/local/stow ; then
   AC_MSG_NOTICE([*** Found /usr/local/stow: default install prefix set to /usr/local/stow/${PACKAGE_NAME}-${PACKAGE_VERSION} ***])
   ac_default_prefix="/usr/local/stow/${PACKAGE_NAME}-${PACKAGE_VERSION}"
fi

# Checks for programs.
AC_PROG_CC
AC_PROG_CC_C99
AC_PROG_CPP

AC_USE_SYSTEM_EXTENSIONS

AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL

DESIRED_FLAGS="-Wall -W -Wextra -pedantic -pipe -Wformat -Wold-style-definition -Wdeclaration-after-statement -Wfloat-equal -Wmissing-declarations -Wmissing-prototypes -Wstrict-prototypes -Wredundant-decls -Wmissing-noreturn -Wshadow -Wendif-labels -Wpointer-arith -Wbad-function-cast -Wcast-qual -Wcast-align -Wwrite-strings -Winline -Wno-unused-parameter"

for flag in $DESIRED_FLAGS ; do
  CC_CHECK_CFLAGS([$flag], [CFLAGS="$CFLAGS $flag"])
done

# Checks for header files.
AC_HEADER_ASSERT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h stdlib.h string.h sys/socket.h unistd.h sys/prctl.h netinet/in.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT8_T
AC_HEADER_TIME

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
AC_CHECK_FUNCS([memset select strndup setresuid setreuid strerror])

AC_CHECK_LIB(nsl, gethostbyname)
AC_CHECK_LIB(socket, connect)

AC_CHECK_FUNC(res_query, ,
  [AC_CHECK_LIB(resolv, res_query, [],
    [ save_libs="$LIBS"
      LIBS="-lresolv $LIBS"
      AC_MSG_CHECKING([for res_query in -lresolv (alternate version)])
      AC_LINK_IFELSE(
        [AC_LANG_PROGRAM([[#include <resolv.h>]], [[res_query(0,0,0,0,0)]])],
        [ AC_MSG_RESULT(yes) ],
        [ AC_MSG_RESULT(no)
          LIBS="$save_libs"
          AC_CHECK_LIB(bind, res_query,
            [], [ AC_MSG_ERROR(res_query not found) ] ) ] ) ] )
  ]
)

ACX_PTHREAD

ZP_LYNX_DOC

AC_CONFIG_FILES([Makefile libasyncns/Makefile doc/Makefile doc/README.html doxygen/Makefile doxygen/doxygen.conf libasyncns.pc])
AC_OUTPUT
