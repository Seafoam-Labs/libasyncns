# $Id$
#
# This file is part of libasyncns.
#
# libasyncns is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# libasyncns is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with libasyncns; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
# USA.

AM_CFLAGS=-D_GNU_SOURCE -D__EXTENSIONS__ $(PTHREAD_CFLAGS)
AM_LIBADD=$(PTHREAD_LIBS)
AM_LDADD=$(PTHREAD_LIBS)

lib_LTLIBRARIES=libasyncns.la
libasyncns_la_LIBADD=$(AM_LIBADD) 
libasyncns_la_CC=$(PTHREAD_CC)
libasyncns_la_SOURCES=asyncns.c asyncns.h

noinst_PROGRAMS=asyncns-test
asyncns_test_SOURCES=asyncns-test.c
asyncns_test_LDADD=$(AM_LDADD) libasyncns.la

include_HEADERS=asyncns.h
