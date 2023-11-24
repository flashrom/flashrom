/*
 * This file is part of the flashrom project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef CLI_CLASSIC_H
#define CLI_CLASSIC_H

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
#endif /* CLI_CLASSIC_H */
