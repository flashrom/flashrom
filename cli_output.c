/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: 2009 Sean Nelson <audiohacked@gmail.com>
 * SPDX-FileCopyrightText: 2011 Carl-Daniel Hailfinger
 */

#include <cli_output.h>

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

enum flashrom_log_level verbose_screen = FLASHROM_MSG_INFO;
enum flashrom_log_level verbose_logfile = FLASHROM_MSG_DEBUG2;

/* Enum to indicate what was the latest printed char prior to a progress indicator. */
enum line_state {
	NEWLINE,
	MIDLINE,
	PROGRESS
};
static enum line_state line_state = NEWLINE;

static FILE *logfile = NULL;

int close_logfile(void)
{
	if (!logfile)
		return 0;
	/* No need to call fflush() explicitly, fclose() already does that. */
	if (fclose(logfile)) {
		/* fclose returned an error. Stop writing to be safe. */
		logfile = NULL;
		msg_gerr("Closing the log file returned error %s\n", strerror(errno));
		return 1;
	}
	logfile = NULL;
	return 0;
}

int open_logfile(const char * const filename)
{
	if (!filename) {
		msg_gerr("No logfile name specified.\n");
		return 1;
	}
	if ((logfile = fopen(filename, "w")) == NULL) {
		msg_gerr("Error: opening log file \"%s\" failed: %s\n", filename, strerror(errno));
		return 1;
	}
	return 0;
}

void start_logging(void)
{
	enum flashrom_log_level oldverbose_screen = verbose_screen;

	/* Shut up the console. */
	verbose_screen = FLASHROM_MSG_ERROR;
	print_version();
	verbose_screen = oldverbose_screen;
}

static const char *flashrom_progress_stage_to_string(enum flashrom_progress_stage stage)
{
	if (stage == FLASHROM_PROGRESS_READ)
		return "READ";
	if (stage == FLASHROM_PROGRESS_WRITE)
		return "WRITE";
	if (stage == FLASHROM_PROGRESS_ERASE)
		return "ERASE";
	return "UNKNOWN";
}

static void print_progress(const struct cli_progress *cli_progress, enum flashrom_progress_stage stage)
{
	if (!(cli_progress->visible_stages & (1 << stage)))
		return;

	msg_ginfo("[%s: %2u%%]", flashrom_progress_stage_to_string(stage), cli_progress->stage_pc[stage]);
}

void flashrom_progress_cb(enum flashrom_progress_stage stage, size_t current, size_t total, void* user_data)
{
	unsigned int pc = 0;
	struct cli_progress *cli_progress = user_data;

	/* The expectation is that initial progress of zero is reported before doing anything. */
	if (current == 0) {
		if (!cli_progress->stage_setup) {
			cli_progress->stage_setup = true;

			/* Initialization of some stage doesn't imply that it will make any progress,
			 * only show stages which have progressed. */
			cli_progress->visible_stages = 0;

			if (line_state != NEWLINE) {
				/* We're going to clear and replace ongoing progress output, so make a new line. */
				msg_ginfo("\n");
			}
		}

		cli_progress->stage_pc[stage] = 0;
	} else {
		cli_progress->stage_setup = false;
		cli_progress->visible_stages |= 1 << stage;
	}

	if (current > 0 && total > 0)
		pc = ((unsigned long long) current * 100llu) /
		     ((unsigned long long) total);
	if (cli_progress->stage_pc[stage] != pc) {
		cli_progress->stage_pc[stage] = pc;

		if (line_state == PROGRESS) {
			/* Erase previous output, because it was previous progress step. */
			int i;
			for (i = 0; i < 16 * FLASHROM_PROGRESS_NR; ++i)
				msg_ginfo("\b \b");
		} else if (line_state == MIDLINE) {
			/* Start with new line, to preserve some other previous message */
			msg_ginfo("\n");
		} // Remaining option is NEWLINE, which means nothing to do: newline has been printed already.

		/* The order is deliberate, the operations typically follow this sequence. */
		print_progress(cli_progress, FLASHROM_PROGRESS_READ);
		print_progress(cli_progress, FLASHROM_PROGRESS_ERASE);
		print_progress(cli_progress, FLASHROM_PROGRESS_WRITE);

		/* There can be output right after the progress, this acts as a separator. */
		msg_ginfo("...");

		/* Reset the flag, because now the latest message is a progress one. */
		line_state = PROGRESS;
	}
}

static void update_line_state(const char *fmt)
{
	size_t len = strlen(fmt);
	if (len > 0)
		line_state = (fmt[len - 1] == '\n' ? NEWLINE : MIDLINE);
}

/* Please note that level is the verbosity, not the importance of the message. */
int flashrom_print_cb(enum flashrom_log_level level, const char *fmt, va_list ap)
{
	int ret = 0;
	FILE *output_type = stdout;

	va_list logfile_args;
	va_copy(logfile_args, ap);

	if (level < FLASHROM_MSG_INFO)
		output_type = stderr;

	if (level <= verbose_screen) {
		ret = vfprintf(output_type, fmt, ap);
		update_line_state(fmt);
		/* msg_*spew often happens inside chip accessors in possibly
		 * time-critical operations. Don't slow them down by flushing. */
		if (level != FLASHROM_MSG_SPEW)
			fflush(output_type);
	}

	if ((level <= verbose_logfile) && logfile) {
		ret = vfprintf(logfile, fmt, logfile_args);
		update_line_state(fmt);
		if (level != FLASHROM_MSG_SPEW)
			fflush(logfile);
	}

	va_end(logfile_args);
	return ret;
}
