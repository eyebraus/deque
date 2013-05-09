/*
 * FILE: hrtimer_alpha.h
 * DESCRIPTION: Hearder file for hrtimer_alpha.c - a high-resolution timer on alpha.
 */

#ifndef _HRTIMER_ALPHA_H_
#define _HRTIMER_ALPHA_H_

/* get the elapsed time (in seconds) since startup */
double gethrtime_alpha(void);

/* get the number of CPU cycles since startup */
unsigned long gethrcycle_alpha(void);
#endif	// _HRTIMER_ALPHA_H_
