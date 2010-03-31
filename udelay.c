/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2000 Silicon Integrated System Corporation
 * Copyright (C) 2009,2010 Carl-Daniel Hailfinger
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <sys/time.h>
#include <stdlib.h>
#include <limits.h>
#include "flash.h"

/* loops per microsecond */
unsigned long micro = 1;

__attribute__ ((noinline)) void myusec_delay(int usecs)
{
	unsigned long i;
	for (i = 0; i < usecs * micro; i++) {
		/* Make sure the compiler doesn't optimize the loop away. */
		asm volatile ("" : : "rm" (i) );
	}
}

unsigned long measure_delay(int usecs)
{
	unsigned long timeusec;
	struct timeval start, end;
	
	gettimeofday(&start, 0);
	myusec_delay(usecs);
	gettimeofday(&end, 0);
	timeusec = 1000000 * (end.tv_sec - start.tv_sec) +
		   (end.tv_usec - start.tv_usec);
	/* Protect against time going forward too much. */
	if ((end.tv_sec > start.tv_sec) &&
	    ((end.tv_sec - start.tv_sec) >= LONG_MAX / 1000000 - 1))
		timeusec = LONG_MAX;
	/* Protect against time going backwards during leap seconds. */
	if ((end.tv_sec < start.tv_sec) || (timeusec > LONG_MAX))
		timeusec = 1;

	return timeusec;
}

void myusec_calibrate_delay(void)
{
	unsigned long count = 1000;
	unsigned long timeusec;
	int i, tries = 0;

	printf("Calibrating delay loop... ");

recalibrate:
	while (1) {
		timeusec = measure_delay(count);
		if (timeusec > 1000000 / 4)
			break;
		if (count >= ULONG_MAX / 2) {
			msg_pinfo("timer loop overflow, reduced precision. ");
			break;
		}
		count *= 2;
	}
	tries ++;

	/* Avoid division by zero, but in that case the loop is shot anyway. */
	if (!timeusec)
		timeusec = 1;
	
	/* Compute rounded up number of loops per microsecond. */
	micro = (count * micro) / timeusec + 1;
	msg_pdbg("%luM loops per second, ", micro);

	/* Did we try to recalibrate less than 5 times? */
	if (tries < 5) {
		/* Recheck our timing to make sure we weren't just hitting
		 * a scheduler delay or something similar.
		 */
		for (i = 0; i < 4; i++) {
			if (measure_delay(100) < 90) {
				msg_pdbg("delay more than 10% too short, "
					 "recalculating... ");
				goto recalibrate;
			}
		}
	} else {
		msg_perr("delay loop is unreliable, trying to continue ");
	}

	/* We're interested in the actual precision. */
	timeusec = measure_delay(10);
	msg_pdbg("10 myus = %ld us, ", timeusec);
	timeusec = measure_delay(100);
	msg_pdbg("100 myus = %ld us, ", timeusec);
	timeusec = measure_delay(1000);
	msg_pdbg("1000 myus = %ld us, ", timeusec);
	timeusec = measure_delay(10000);
	msg_pdbg("10000 myus = %ld us, ", timeusec);

	printf("OK.\n");
}

void internal_delay(int usecs)
{
	/* If the delay is >1 s, use usleep because timing does not need to
	 * be so precise.
	 */
	if (usecs > 1000000) {
		usleep(usecs);
	} else {
		myusec_delay(usecs);
	}
}

