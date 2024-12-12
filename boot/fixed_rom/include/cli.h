/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (CompanyNameMagicTag) Copyright 2014 Google, Inc
 * Simon Glass <sjg@chromium.org>
 */

#ifndef __CLI_H
#define __CLI_H

/**
 * Go into the command loop
 *
 * This will return if we get a timeout waiting for a command. See
 * CONFIG_BOOT_RETRY_TIME.
 */
void cli_simple_loop(void);

/**
 * cli_simple_run_command() - Execute a command with the simple CLI
 *
 * @cmd:	String containing the command to execute
 * @flag	Flag value - see CMD_FLAG_...
 * @return 1  - command executed, repeatable
 *	0  - command executed but not repeatable, interrupted commands are
 *	     always considered not repeatable
 *	-1 - not executed (unrecognized, bootd recursion or too many args)
 *           (If cmd is NULL or "" or longer than CONFIG_SYS_CBSIZE-1 it is
 *           considered unrecognized)
 */
int cli_simple_run_command(const char *cmd, int flag);

/**
 * cli_readline() - read a line into the console_buffer
 *
 * This is a convenience function which calls cli_readline_into_buffer().
 *
 * @prompt: Prompt to display
 * @return command line length excluding terminator, or -ve on error
 */
int cli_readline(const char *const prompt);

/**
 * readline_into_buffer() - read a line into a buffer
 *
 * Display the prompt, then read a command line into @buffer. The
 * maximum line length is CONFIG_SYS_CBSIZE including a \0 terminator, which
 * will always be added.
 *
 * The command is echoed as it is typed. Command editing is supported if
 * CONFIG_CMDLINE_EDITING is defined. Tab auto-complete is supported if
 * CONFIG_AUTO_COMPLETE is defined. If CONFIG_BOOT_RETRY_TIME is defined,
 * then a timeout will be applied.
 *
 * If CONFIG_BOOT_RETRY_TIME is defined and retry_time >= 0,
 * time out when time goes past endtime (timebase time in ticks).
 *
 * @prompt:	Prompt to display
 * @buffer:	Place to put the line that is entered
 * @timeout:	Timeout in milliseconds, 0 if none
 * @return command line length excluding terminator, or -ve on error: of the
 * timeout is exceeded (either CONFIG_BOOT_RETRY_TIME or the timeout
 * parameter), then -2 is returned. If a break is detected (Ctrl-C) then
 * -1 is returned.
 */
int cli_readline_into_buffer(const char *const prompt, char *buffer,
                             int timeout);

/**
 * parse_line() - split a command line down into separate arguments
 *
 * The argv[] array is filled with pointers into @line, and each argument
 * is terminated by \0 (i.e. @line is changed in the process unless there
 * is only one argument).
 *
 * #argv is terminated by a NULL after the last argument pointer.
 *
 * At most CONFIG_SYS_MAXARGS arguments are permited - if there are more
 * than that then an error is printed, and this function returns
 * CONFIG_SYS_MAXARGS, with argv[] set up to that point.
 *
 * @line:	Command line to parse
 * @args:	Array to hold arguments
 * @return number of arguments
 */
int cli_simple_parse_line(char *line, char *argv[]);

/**
 * Go into the command loop
 *
 * This will return if we get a timeout waiting for a command, but only for
 * the simple parser (not hush). See CONFIG_BOOT_RETRY_TIME.
 */
void cli_loop(void);

#endif
