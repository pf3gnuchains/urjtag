/*
 * $Id: jtag.c 1744 2010-01-29 08:34:30Z vapier $
 *
 * Copyright (C) 2002, 2003 ETC s.r.o.
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
 * Written by Marcel Telka <marcel@telka.sk>, 2002, 2003.
 * Modified by Ajith Kumar P.C <ajithpc@kila.com>, 20/09/2006.
 *
 */

#include <sysdep.h>

#ifndef SVN_REVISION
#define SVN_REVISION "0"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef HAVE_LIBREADLINE
#include <readline/readline.h>
#ifdef HAVE_READLINE_HISTORY_H
#include <readline/history.h>
#endif
#endif
#include <getopt.h>
#ifdef ENABLE_NLS
#include <locale.h>
#endif /* ENABLE_NLS */
#include <errno.h>

#include <urjtag/chain.h>
#include <urjtag/bus.h>
#include <urjtag/cmd.h>
#include <urjtag/flash.h>
#include <urjtag/parse.h>
#include <urjtag/jtag.h>

static int urj_interactive = 0;

#define JTAGDIR         ".jtag"
#define HISTORYFILE     "history"
#define RCFILE          "rc"

static int
jtag_create_jtagdir (void)
{
    char *home = getenv ("HOME");
    char *jdir;
    int r;

    if (!home)
    {
        urj_error_set (URJ_ERROR_UNSUPPORTED, "env var HOME not set");
        return URJ_STATUS_FAIL;
    }

    jdir = malloc (strlen (home) + strlen (JTAGDIR) + 2);       /* "/" and trailing \0 */
    if (!jdir)
    {
        urj_error_set (URJ_ERROR_OUT_OF_MEMORY, "malloc(%zd) fails",
                       strlen (home) + strlen (JTAGDIR) + 2);
        return URJ_STATUS_FAIL;
    }

    strcpy (jdir, home);
    strcat (jdir, "/");
    strcat (jdir, JTAGDIR);

    /* Create the directory if it doesn't exists. */
    r = mkdir (jdir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    if (r == -1)
    {
        if (errno == EEXIST)
        {
            /* @@@@ RFHH check if it is a directory */
            errno = 0;
        } else {
            free (jdir);
            urj_error_IO_set ("cannot mkdir(%s)", jdir);
            return URJ_STATUS_FAIL;
        }
    }

    free (jdir);

    return URJ_STATUS_OK;
}

#ifdef HAVE_LIBREADLINE

#ifdef HAVE_READLINE_COMPLETION
static char **
urj_cmd_completion (const char *text, int start, int end)
{
    char **ret = NULL;

    if (start == 0)
        ret = rl_completion_matches (text, urj_cmd_find_next);

    return ret;
}
#endif /* def HAVE_READLINE_COMPLETION */

#ifdef HAVE_READLINE_HISTORY

static int
jtag_load_history (void)
{
    char *home = getenv ("HOME");
    char *file;

    using_history ();

    if (!home)
    {
        urj_error_set (URJ_ERROR_UNSUPPORTED, "env var HOME not set");
        return URJ_STATUS_FAIL;
    }

    file = malloc (strlen (home) + strlen (JTAGDIR) + strlen (HISTORYFILE) + 3);        /* 2 x "/" and trailing \0 */
    if (!file)
    {
        urj_error_set (URJ_ERROR_OUT_OF_MEMORY, "malloc(%zd) fails",
                   strlen (home) + strlen (JTAGDIR) + strlen (HISTORYFILE) + 3);
        return URJ_STATUS_FAIL;
    }

    strcpy (file, home);
    strcat (file, "/");
    strcat (file, JTAGDIR);
    strcat (file, "/");
    strcat (file, HISTORYFILE);

    read_history (file);

    free (file);

    return URJ_STATUS_OK;
}

static int
jtag_save_history (void)
{
    char *home = getenv ("HOME");
    char *file;

    if (!home)
    {
        urj_error_set (URJ_ERROR_UNSUPPORTED, "env var HOME not set");
        return URJ_STATUS_FAIL;
    }

    file = malloc (strlen (home) + strlen (JTAGDIR) + strlen (HISTORYFILE) + 3);        /* 2 x "/" and trailing \0 */
    if (!file)
    {
        urj_error_set (URJ_ERROR_OUT_OF_MEMORY, "malloc(%zd) fails",
                   strlen (home) + strlen (JTAGDIR) + strlen (HISTORYFILE) + 3);
        return URJ_STATUS_FAIL;
    }

    strcpy (file, home);
    strcat (file, "/");
    strcat (file, JTAGDIR);
    strcat (file, "/");
    strcat (file, HISTORYFILE);

    write_history (file);

    free (file);

    return URJ_STATUS_OK;
}

#endif /* HAVE_READLINE_HISTORY */

#endif /* HAVE_READLINE */

/** @return URJ_STATUS_QUIT on quit command, URJ_STATUS_OK on success,
 * URJ_STATUS_ERROR on error */
static int
jtag_readline_multiple_commands_support (urj_chain_t *chain, char *line)        /* multiple commands should be separated with '::' */
{
    int r;
    char *nextcmd = line;

    if (!line || !(strlen (line) > 0))
        return 1;

    do
    {
        line = nextcmd;

        nextcmd = strstr (nextcmd, "::");       /* :: to not confuse ms-dos users ;-) */

        if (nextcmd)
        {
            *nextcmd++ = 0;
            ++nextcmd;
            while (*line == ':')
                ++line;
        }

        r = urj_parse_line (chain, line);
        if (r == URJ_STATUS_FAIL)
        {
            urj_log (URJ_LOG_LEVEL_NORMAL, "Error: %s\n", urj_error_describe());
            urj_error_reset ();
        }

        urj_tap_chain_flush (chain);

    }
    while (nextcmd && r != URJ_STATUS_MUST_QUIT);

    return r;
}

static void
jtag_readline_loop (urj_chain_t *chain, const char *prompt)
{
#ifdef HAVE_LIBREADLINE
    char *line = NULL;
#ifdef HAVE_READLINE_HISTORY
    HIST_ENTRY *hptr;
#endif

    /* Iterate */
    while (jtag_readline_multiple_commands_support (chain, line)
           != URJ_STATUS_MUST_QUIT)
    {
        free (line);

        /* Read a line from the terminal */
        line = readline (prompt);

        /* We got EOF, bail */
        if (!line)
        {
            /* @@@@ RFHH check strdup result */
            line = strdup ("quit\n");
            puts ("quit");
            if (!line)
                return;
        }

#ifdef HAVE_READLINE_HISTORY
        /* Check if we actually got something , Don't add duplicate lines */
        if (strlen (line))
        {
            if (history_length == 0)
                add_history (line);
            else
            {
                hptr = history_get (history_length);
                /* Apple leopard libreadline emulation screws up indexing, try the other one */
                if (hptr == NULL)
                    hptr = history_get (history_length - 1);
                if (hptr != NULL)
                {
                    if (strcmp (line, hptr->line))
                        add_history (line);
                }
                else
                    add_history (line);
            }
        }
#endif
    }
    free (line);
#else
    char line[1024];
    line[0] = 0;
    do
    {
        if (jtag_readline_multiple_commands_support (chain, line)
            == URJ_STATUS_MUST_QUIT)
            break;
        printf ("%s", prompt);
        fflush (stdout);
    }
    while (fgets (line, 1023, stdin));
#endif
}

static int
jtag_parse_rc (urj_chain_t *chain)
{
    char *home = getenv ("HOME");
    char *file;
    int go;

    if (!home)
    {
        urj_error_set (URJ_ERROR_UNSUPPORTED, "env var HOME not set");
        return URJ_STATUS_FAIL;
    }

    file = malloc (strlen (home) + strlen (JTAGDIR) + strlen (RCFILE) + 3);     /* 2 x "/" and trailing \0 */
    if (!file)
    {
        urj_error_set (URJ_ERROR_OUT_OF_MEMORY, "malloc(%zd) fails",
                       strlen (home) + strlen (JTAGDIR) + strlen (RCFILE) + 3);
        return URJ_STATUS_FAIL;
    }

    strcpy (file, home);
    strcat (file, "/");
    strcat (file, JTAGDIR);
    strcat (file, "/");
    strcat (file, RCFILE);

    go = urj_parse_file (URJ_LOG_LEVEL_DETAIL, chain, file);

    free (file);

    return go;
}

static void
cleanup (urj_chain_t *chain)
{
    urj_flash_cleanup ();
    urj_bus_buses_free ();
    urj_tap_chain_free (chain);
    chain = NULL;
}

int
main (int argc, char *const argv[])
{
    int go = 1;
    int i;
    int c;
    int norc = 0;
    int help = 0;
    int version = 0;
    int quiet = 0;
    urj_chain_t *chain = NULL;

    urj_set_argv0 (argv[0]);

    if (geteuid () == 0 && getuid () != 0)
    {
        printf (_("'%s' must not be run suid root!\n"), "jtag");
        return (-1);
    }

#ifdef ENABLE_NLS
    /* l10n support */
    setlocale (LC_ALL, "");
    bindtextdomain (PACKAGE, LOCALEDIR);
    textdomain (PACKAGE);
#endif /* ENABLE_NLS */

    while (1)
    {
        static struct option long_options[] = {
            {"version", no_argument, 0, 'v'},
            {"norc", no_argument, 0, 'n'},
            {"interactive", no_argument, 0, 'i'},
            {"help", no_argument, 0, 'h'},
            {"quiet", no_argument, 0, 'q'},
            {0, 0, 0, 0}
        };

        /* `getopt_long' stores the option index here. */
        int option_index = 0;

        c = getopt_long (argc, argv, "vnhiq", long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c)
        {
        case 'v':
            version = 1;
            break;

        case 'n':
            norc = 1;
            break;

        case 'i':
            urj_interactive = 1;
            break;

        case 'h':
        default:
            help = 1;
            break;

        case 'q':
            quiet = 1;
            break;
        }
    }

    if (help)
    {
        /* Print help info and exit.  */
        printf (_("%s #%s\n"), PACKAGE_STRING, SVN_REVISION);
        printf ("\n");

        printf (_("Usage: %s [OPTIONS] [FILE [FILE ... ]] \n"), "jtag");
        printf ("\n");

        printf (_("  -h, --help          display this help and exit\n"));
        printf (_("  -v, --version       display version information and exit\n"));
        printf ("\n");
        printf (_("  -n, --norc          disable reading ~/.jtag/rc on startup\n"));
        printf (_("  -i, --interactive   enter interactive mode after reading files\n"));
        printf (_("  -q, --quiet         Do not print help on startup\n"));
        printf ("\n");
        printf (_("  [FILE]              file containing commands to execute\n"));
        printf ("\n");

        printf (_("  Please report bugs at http://www.urjtag.org\n"));

        exit (0);
    }

    if (version)
    {
        printf (_("\n%s #%s\n\n"
                  "Copyright (C) 2002, 2003 ETC s.r.o.\n"
                  "Copyright (C) 2007, 2008, 2009 Kolja Waschk and the respective authors\n"),
                PACKAGE_STRING, SVN_REVISION);

        printf (_("\n"
                  "This program is free software; you can redistribute it and/or modify\n"
                  "it under the terms of the GNU General Public License as published by\n"
                  "the Free Software Foundation; either version 2 of the License, or\n"
                  "(at your option) any later version.\n"
                  "\n"
                  "This program is distributed in the hope that it will be useful,\n"
                  "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
                  "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
                  "GNU General Public License for more details.\n"
                  "\n"
                  "You should have received a copy of the GNU General Public License\n"
                  "along with this program; if not, write to the Free Software\n"
                  "Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA\n"));

        exit (0);
    }

    /* input from files */
    if (argc > optind)
    {
        for (i = optind; i < argc; i++)
        {
            chain = urj_tap_chain_alloc ();
            if (!chain)
            {
                printf (_("Out of memory\n"));
                return -1;
            }

            go = urj_parse_file (URJ_LOG_LEVEL_NORMAL, chain, argv[i]);
            cleanup (chain);
            if (go < 0 && go != URJ_STATUS_MUST_QUIT)
            {
                printf (_("Unable to open file `%s'!\n"), argv[i]);
                break;
            }
        }

        if (!urj_interactive)
            return 0;
    }

    /* input from stdin */
    if (!isatty (0))
    {
        chain = urj_tap_chain_alloc ();
        if (!chain)
        {
            printf (_("Out of memory\n"));
            return -1;
        }
        urj_parse_stream (URJ_LOG_LEVEL_NORMAL, chain, stdin);

        cleanup (chain);

        return 0;
    }

    /* interactive */
    if (!quiet)
        printf (_("\n%s #%s\n"
                  "Copyright (C) 2002, 2003 ETC s.r.o.\n"
                  "Copyright (C) 2007, 2008, 2009 Kolja Waschk and the respective authors\n\n"
                  "%s is free software, covered by the GNU General Public License, and you are\n"
                  "welcome to change it and/or distribute copies of it under certain conditions.\n"
                  "There is absolutely no warranty for %s.\n\n"),
                PACKAGE_STRING, SVN_REVISION, PACKAGE_NAME, PACKAGE_NAME);

    chain = urj_tap_chain_alloc ();
    if (!chain)
    {
        printf (_("Out of memory\n"));
        return -1;
    }

    if (!quiet)
    {
        urj_warning (_("%s may damage your hardware!\n"), PACKAGE_NAME);
        urj_log (URJ_LOG_LEVEL_NORMAL,
                 _("Type \"quit\" to exit, \"help\" for help.\n\n"));
    }

    /* Create ~/.jtag */
    if (jtag_create_jtagdir () != URJ_STATUS_OK)
    {
        urj_warning ("%s\n", urj_error_describe());
        urj_error_reset();
    }

    /* Parse and execute the RC file */
    if (!norc)
    {
        if (jtag_parse_rc (chain) == URJ_STATUS_FAIL)
        {
            if (urj_error_get() != URJ_ERROR_IO)
            {
                urj_log (URJ_LOG_LEVEL_NORMAL, "Error: %s\n",
                         urj_error_describe());
                go = 0;
            }
            urj_error_reset();
        }
    }

#ifdef HAVE_LIBREADLINE
#ifdef HAVE_READLINE_COMPLETION
    rl_completer_quote_characters = "\"";
    rl_filename_completion_desired = 1;
    rl_filename_quote_characters = " ";
    rl_attempted_completion_function = urj_cmd_completion;
#endif
#endif

    if (go)
    {

#ifdef HAVE_READLINE_HISTORY
        /* Load history */
        jtag_load_history ();
#endif

        /* main loop */
        jtag_readline_loop (chain, getenv ("JTAG_PROMPT") ? : "jtag> ");

#ifdef HAVE_READLINE_HISTORY
        /* Save history */
        jtag_save_history ();
#endif
    }

    cleanup (chain);

    return 0;
}
