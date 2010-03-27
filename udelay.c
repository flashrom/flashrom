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

// count to a billion. Time it. If it's < 1 sec, count to 10B, etc.
unsigned long micro = 1;

void myusec_delay(int usecs)
{
	volatile unsigned long i;
	for (i = 0; i < usecs * micro; i++) ;
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

	return timeusec;
}

void myusec_calibrate_delay(void)
{
	int count = 1000;
	unsigned long timeusec;
	int ok = 0;

	printf("Calibrating delay loop... ");

	while (!ok) {
		timeusec = measure_delay(count);
		count *= 2;
		if (timeusec < 1000000 / 4)
			continue;
		ok = 1;
	}

	// compute one microsecond. That will be count / time
	micro = count / timeusec;
	msg_pdbg("%ldM loops per second, ", micro);

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

