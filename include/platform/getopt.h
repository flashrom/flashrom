/*
 * This file is part of the flashrom project.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef __CLI_GETOPT_H__
#define __CLI_GETOPT_H__

/**
 * This header is responsible for either including a standard getop
 * implementation header or to provide a compatible one.
 */

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else

#define	no_argument		0
#define required_argument	1
#define optional_argument	2

extern char *optarg;
extern int optind, opterr, optopt;

struct option {
  const char *name;
  int has_arg;
  int *flag;
  int val;
};

int getopt (int argc, char *const *argv, const char *shortopts);
int getopt_long (int argc, char *const *argv, const char *shortopts,
			const struct option *longopts, int *longind);
int getopt_long_only (int argc, char *const *argv, const char *shortopts,
			const struct option *longopts, int *longind);

#endif /* HAVE_GETOPT_H */
#endif /* __CLI_GETOPT_H__ */
