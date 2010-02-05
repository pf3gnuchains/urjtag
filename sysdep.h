/*
 * $Id: sysdep.h 1745 2010-01-29 08:39:13Z vapier $
 *
 * Copyright (C) 2003 ETC s.r.o.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * Written by Marcel Telka <marcel@telka.sk>, 2003.
 *
 */

#ifndef SYSDEP_H
#define	SYSDEP_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <urjtag/gettext.h>
#define	_(s)		gettext(s)
#define	N_(s)		gettext_noop(s)
#define	P_(s,p,n)	ngettext(s,p,n)

#ifdef S_SPLINT_S
#undef gettext
#define	gettext(s)	s
#undef gettext_noop
#define	gettext_noop(s)	s
#undef ngettext
#define	ngettext(s,p,n)	s
#endif

#if __CYGWIN__
#include <windows.h>
#endif

#ifdef __MINGW32__
/* Diff versions of mingw used slightly different names */
#define NO_W32_PSEUDO_MODIFIERS
#define _NO_W32_PSEUDO_MODIFIERS
#include <windows.h>
#define geteuid() 0
#define getuid() 0
/* Microsoft uses a different swprintf() than ISO C requires */
#include <stdio.h>
#define swprintf _snwprintf
/* No perms to give to mkdir */
#include <io.h>
#define mkdir(path, mode) mkdir(path)
#endif

#ifndef HAVE_USLEEP
int usleep (long unsigned usec);
#endif

#ifndef HAVE_NANOSLEEP
#include <unistd.h>
struct timespec { unsigned long tv_sec, tv_nsec; };
#define nanosleep(req, rem) usleep((req)->tv_sec * 1000 * 1000 + (req)->tv_nsec / 1000)
#endif

#define ARRAY_SIZE(a) (sizeof (a) / sizeof ((a)[0]))

#endif /* SYSDEP_H */
