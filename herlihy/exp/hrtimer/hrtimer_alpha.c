/*
 * FILE: hrtimer_alpha.cc
 * DESCRIPTION: A high-resolution timer on alpha architecture.
 * Window length is 7.17023 seconds on 599MHz processor.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "hrtimer_alpha.h"
double SEC_PER_HZ = 1.0/599000000;

/* get the number of CPU cycles since startup */
unsigned long gethrcycle_alpha(void)
{
  unsigned long _ticks;
  asm volatile ("rpcc %0" : "=r"(_ticks));
  return _ticks & 0xffffffff;
}

/* get the elapsed time (in seconds) since startup */
double gethrtime_alpha(void)
{
  return (gethrcycle_alpha()*SEC_PER_HZ);
}
