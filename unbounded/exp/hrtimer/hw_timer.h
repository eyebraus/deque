#ifndef HW_TIMER_H
#define HW_TIMER_H

#include <sys/time.h>

#ifdef _SOLARIS_

#define TICKS_PER_NANO 1
#define NANOS_PER_TICK 1
/* The libary support is "gethrtime()". See man page.
/* gethrtime() gets about 225ns */

#define ticks2nano(_ticks) ((double)_ticks)

static inline unsigned int second2tick(unsigned int seconds) {
  return (unsigned int)(TICKS_PER_NANO * seconds * 1000 * 1000 * 1000);
}

#endif /*SOLARIS*/

/*--------------------------------------------------------------------*/
#ifdef _PPC_
/* 

   The hardware support is the assemble instruction "mftb". 
   Ticks is increased by one every 8 cycles.
   See the architecture manual for reference.

   The libary support is "time_base_to_time" and "read_real_time".
   http://publibn.boulder.ibm.com/doc_link/en_US/a_doc_lib/libs/basetrf2/read_real_time.htm

   Here gethrtime() takes about 55 ns(8.5 cycles). the rollover happens 
   every 26.4306 seconds. The return value is in ticks.

*/

#define TICKS_PER_NANO (0.1625)  /* 1.3G/8 */
#define NANOS_PER_TICK (6.15378) /* 8/1.3G */

static inline unsigned int get_ticks() {
#ifdef __GNUC__
  unsigned int  ticks;
  asm volatile ( "mftb %0"      /* lower time base register */
                 :"=r" (ticks));
  return ticks;
#else
  atime_t tm;
  read_real_time(&tm, TIMEBASE_SZ);
  return tm.tb_low;
#endif
}

#define gethrtime() get_ticks()

static inline double  ticks2nano(unsigned int ticks) {
  return ticks * NANOS_PER_TICK;
}
static inline unsigned int second2tick(unsigned int seconds) {
  return (unsigned int)(TICKS_PER_NANO * seconds * 1000 * 1000 * 1000);
}

/* this fucntion can get the nano time in 20 ns in a non-regular way... */
static inline void gettick20ns(unsigned int& least){
   asm("mftb  6");
   asm("stw 6, 0(3)");
}

#endif /*PPC*/
#endif /* HW_TIMER_H */
