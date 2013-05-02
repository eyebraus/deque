///////////////////////////////////////////////////////////////////////////////
// This file has been adapted from the RSTM distribution
// http://www.cs.rochester.edu/research/synchronization/rstm/


#ifndef ATOMIC_OPS_H__
#define ATOMIC_OPS_H__

#if defined(__i386__) && defined(__GNUC__)

/* "compiler fence" for preventing reordering of loads/stores to
   non-volatiles */
#define CFENCE          asm volatile ("":::"memory")
// depending on the CPU, it may not be cheaper to use a dummy xchg call than to
// use an mfence to get WBR ordering.
// #define WBR asm volatile("mfence")
#define WBR { unsigned long a = 0; unsigned long b = 1; swap(&a, b); }

// gcc x86 CAS and TAS

static inline unsigned long
cas(volatile unsigned long* ptr, unsigned long old, unsigned long _new)
{
    unsigned long prev;
    asm volatile("lock;"
                 "cmpxchgl %1, %2;"
                 : "=a"(prev)
                 : "q"(_new), "m"(*ptr), "a"(old)
                 : "memory");
    return prev;
}

static inline unsigned long tas(volatile unsigned long* ptr)
{
    unsigned long result;
    asm volatile("lock;"
                 "xchgl %0, %1;"
                 : "=r"(result), "=m"(*ptr)
                 : "0"(1), "m"(*ptr)
                 : "memory");
    return result;
}

static inline unsigned long
swap(volatile unsigned long* ptr, unsigned long val)
{
    asm volatile("lock;"
                 "xchgl %0, %1"
                 : "=r"(val), "=m"(*ptr)
                 : "0"(val), "m"(*ptr)
                 : "memory");
    return val;
}

static inline void nop()
{
    asm volatile("nop");
}

// Our code will differ based on the status of -fPIC, since using
// -fPIC on x86 will not let us use EBX in inline asm.
//
// Also note that we want to support -fPIC, since it can't be turned
// off on Darwin.
//
// It's important to list a and d as output registers so that gcc knows that
// they're being messed with in the cmpxchg. Can't be inlined otherwise.
static inline bool
casX(volatile unsigned long long* addr,
     unsigned long expected_high, unsigned long expected_low,
     unsigned long new_high, unsigned long new_low)
{
    char success;
#ifndef __PIC__
    asm volatile("lock; cmpxchg8b (%6);"
                 "setz %7; "
                 : "=a" (expected_low), "=d" (expected_high)
                 : "0" (expected_low), "1" (expected_high),
                   "c" (new_high), "b" (new_low),
                   "r" (addr), "m" (success)
                 : "cc", "memory");
#else
    // %ebx is used oddly when compiling position independent code. All we do
    // is manually save it.
    asm volatile("pushl %%ebx;"           // Save %ebx on the stack
                 "movl %5, %%ebx;"        // Move the proper value into %ebx
                 "lock; cmpxchg8b (%6);"  // Perform the exchange
                 "setz %7; "
                 "popl %%ebx;"            // Restore %ebx
                 : "=d" (expected_high), "=a" (expected_low)
                 : "0" (expected_high),  "1" (expected_low),
                   "c" (new_high),  "r" (new_low),
                   "r" (addr), "m" (success)
                 :  "cc", "memory");
#endif
    return success;
}


static inline bool casX(volatile unsigned long long* addr,
                        const unsigned long long *oldVal,
                        const unsigned long long *newVal)
{
    unsigned long old_high = *oldVal >> 32, old_low = *oldVal;
    unsigned long new_high = *newVal >> 32, new_low = *newVal;

    return casX(addr, old_high, old_low, new_high, new_low);
}

// atomic load and store of *src into *dest
static inline void
mvx(const volatile unsigned long long *src, volatile unsigned long long *dest)
{
    // Cast into double, since this will be 64-bits and (hopefully) result in
    // atomic code.

    const volatile double *srcd = (const volatile double*)src;
    volatile double *destd = (volatile double*)dest;

    *destd = *srcd;
}


#elif defined(__x86_64__) && defined(__GNUC__)

/* "compiler fence" for preventing reordering of loads/stores to
   non-volatiles */
#define CFENCE          asm volatile ("":::"memory")
// depending on the CPU, it may not be cheaper to use a dummy xchg call than to
// use an mfence to get WBR ordering.
// #define WBR asm volatile("mfence")
#define WBR { unsigned long a = 0; unsigned long b = 1; swap(&a, b); }

// gcc x86 CAS and TAS

static inline unsigned long
cas(volatile unsigned long* ptr, unsigned long old, unsigned long _new)
{
    unsigned long prev;
    asm volatile("lock;"
                 "cmpxchgq %1, %2;"
                 : "=a"(prev)
                 : "r"(_new), "m"(*ptr), "a"(old)
                 : "memory");
    return prev;
}

static inline unsigned long tas(volatile unsigned long* ptr)
{
    unsigned long result;
    asm volatile("lock;"
                 "xchgq %0, %1;"
                 : "=r"(result), "=m"(*ptr)
                 : "0"(1), "m"(*ptr)
                 : "memory");
    return result;
}

static inline unsigned long
swap(volatile unsigned long* ptr, unsigned long val)
{
    asm volatile("lock;"
                 "xchgq %0, %1"
                 : "=r"(val), "=m"(*ptr)
                 : "0"(val), "m"(*ptr)
                 : "memory");
    return val;
}

static inline void nop()
{
    asm volatile("nop");
}


#elif defined(__ia64__) && defined(__GNUC__)
/* "compiler fence" for preventing reordering of loads/stores to
   non-volatiles */
#define CFENCE          asm volatile ("":::"memory")

#define WBR asm volatile ("mf;;")

// unsigned long => 64 bit CAS
static inline unsigned long
cas(volatile unsigned long* ptr, unsigned long old, unsigned long _new)
{
    unsigned long _old;
    asm volatile("mf;;mov ar.ccv=%0;;"::"rO"(old));
    asm volatile("cmpxchg8.acq %0=%1,%2,ar.ccv ;mf;;"   // instruction
                 : "=r"(_old), "=m"(*ptr)               // output
                 : "r"(_new)                            // inputs
                 : "memory");                           // side effects
    return _old;
}

static inline unsigned long tas(volatile unsigned long* ptr)
{
    unsigned long result;
    asm volatile("mf;;xchg8 %0=%1,%2;;mf;;"
                 : "=r"(result), "=m"(*ptr)
                 : "r"(1)
                 : "memory");
    return result;
}

static inline unsigned long
swap(volatile unsigned long* ptr, unsigned long val)
{
    unsigned long result;
    asm volatile("mf;;xchg8 %0=%1,%2;;mf;;"
                 : "=r"(result), "=m"(*ptr)
                 : "r"(val)
                 : "memory");
    return result;
}

static inline void nop()
{
    asm volatile("nop.m 0;;");
}

// NB: casX is not yet implemented

#elif defined(__sparc__) && defined(__GNUC__)

/* "compiler fence" for preventing reordering of loads/stores to
   non-volatiles */
#define CFENCE          asm volatile ("":::"memory")

#define WBR asm volatile ("membar #StoreLoad")

static inline unsigned long
cas(volatile unsigned long* ptr, unsigned long old, unsigned long _new)
{
    asm volatile("cas [%2], %3, %0"                     // instruction
                 : "=&r"(_new)                          // output
                 : "0"(_new), "r"(ptr), "r"(old)        // inputs
                 : "memory");                           // side effects
    return _new;
}

static inline unsigned long tas(volatile unsigned long* ptr)
{
    unsigned long result;
    asm volatile("ldstub [%1], %0"
                 : "=r"(result)
                 : "r"(ptr)
                 : "memory");
    return result;
}

static inline unsigned long
swap(volatile unsigned long* ptr, unsigned long val)
{
    asm volatile("swap [%2], %0"
                 : "=&r"(val)
                 : "0"(val), "r"(ptr)
                 : "memory");
    return val;
}

// NB: When Solaris is in 32-bit mode, it does not save the top 32 bits of a
// 64-bit local (l) register on context switch, so always use an "o" register
// for 64-bit ops in 32-bit mode

// we can't mov 64 bits directly from c++ to a register, so we must ldx
// pointers to get the data into registers
static inline bool casX(volatile unsigned long long* ptr,
                        const unsigned long long* expected_value,
                        const unsigned long long* new_value)
{
    bool success = false;

    asm volatile("ldx   [%1], %%o4;"
                 "ldx   [%2], %%o5;"
                 "casx  [%3], %%o4, %%o5;"
                 "cmp   %%o4, %%o5;"
                 "mov   %%g0, %0;"
                 "move  %%xcc, 1, %0"   // predicated move... should do this
                                        // for bool_cas too
                 : "=r"(success)
                 : "r"(expected_value), "r"(new_value), "r"(ptr)
                 : "o4", "o5", "memory");
    return success;
}

// When casX is dealing with packed structs, it is convenient to pass each word
// directly
static inline bool volatile casX(volatile unsigned long long* ptr,
                                 unsigned long expected_high,
                                 unsigned long expected_low,
                                 unsigned long new_high,
                                 unsigned long new_low)
{
    bool success = false;
    asm volatile("sllx %1, 32, %%o4;"
                 "or   %%o4, %2, %%o4;"
                 "sllx %3, 32, %%o5;"
                 "or   %%o5, %4, %%o5;"
                 "casx [%5], %%o4, %%o5;"
                 "cmp  %%o4, %%o5;"
                 "be,pt %%xcc,1f;"
                 "mov  1, %0;"
                 "mov  %%g0, %0;"
                 "1:"
                 : "=r"(success)
                 : "r"(expected_high), "r"(expected_low), "r"(new_high),
                   "r"(new_low), "r"(ptr)
                 : "o4", "o5", "memory");
    return success;
}

static inline void
mvx(const volatile unsigned long long* from, volatile unsigned long long* to)
{
    asm volatile("ldx  [%0], %%o4;"
                 "stx  %%o4, [%1];"
                 :
                 : "r"(from), "r"(to)
                 : "o4", "memory");
}

static inline void nop()
{
    asm volatile("nop");
}

#else
#error Your CPU/compiler combination is not supported
#endif

static inline bool
bool_cas(volatile unsigned long* ptr, unsigned long old, unsigned long _new)
{
    return cas(ptr, old, _new) == old;
}

static inline unsigned long fai(volatile unsigned long* ptr)
{
    unsigned long found = *ptr;
    unsigned long expected;
    do {
        expected = found;
    } while ((found = cas(ptr, expected, expected + 1)) != expected);
    return found;
}

static inline unsigned long faa(volatile unsigned long* ptr, int amnt)
{
  unsigned long found = *ptr;
  unsigned long expected;
  do {
    expected = found;
  } while ((found = cas(ptr, expected, expected + amnt)) != expected);
  return found;
}


// exponential backoff
static inline void backoff(int *b)
{
    for (int i = *b; i; i--)
        nop();

    if (*b < 4096)
        *b <<= 1;
}

// issue 64 nops to provide a little busy waiting
static inline void spin64()
{
    for (int i = 0; i < 64; i++)
        nop();
}

// issue 128 nops to provide a little busy waiting
static inline void spin128()
{
    for (int i = 0; i < 128; i++)
        nop();
}

////////////////////////////////////////
// tas lock

typedef volatile unsigned long tas_lock_t;

static inline void tas_acquire(tas_lock_t *L) {
    while(tas(L)) {
        nop();
    }
}

static inline void tas_release(tas_lock_t *L) {
    *L = 0;
}

////////////////////////////////////////
// tatas lock

static inline void tatas_acquire(tas_lock_t* L)
{
    do {
        while(*L)
            nop();
    } while(tas(L));
}

static inline void tatas_release(tas_lock_t* L)
{
    *L = 0;
}

////////////////////////////////////////
// tatas lock with backoff

static inline void tatas_backoff_acquire(tas_lock_t* L)
{
    int b = 64;
    do {
        while(*L) {
            backoff(&b);
        }
    } while(tas(L));
}

static inline void tatas_backoff_release(tas_lock_t* L)
{
    *L = 0;
}

////////////////////////////////////////
// ticket lock

extern "C"
{
    typedef struct
    {
        volatile unsigned long next_ticket;
        volatile unsigned long now_serving;
    } ticket_lock_t;
}

static inline void ticket_acquire(ticket_lock_t* L)
{
    unsigned long my_ticket = fai(&L->next_ticket);
    while (L->now_serving != my_ticket);
}

static inline void ticket_release(ticket_lock_t* L)
{
    L->now_serving += 1;
}

////////////////////////////////////////
// MCS lock

extern "C"
{
    typedef volatile struct _mcs_qnode_t
    {
        bool flag;
        volatile struct _mcs_qnode_t* next;
    } mcs_qnode_t;
}

static inline void mcs_acquire(mcs_qnode_t** L, mcs_qnode_t* I)
{
    I->next = 0;
    mcs_qnode_t* pred =
        (mcs_qnode_t*)swap((volatile unsigned long*)L, (unsigned long)I);

    if (pred != 0) {
        I->flag = true;
        pred->next = I;
        while (I->flag) { } // spin
    }
}

static inline void mcs_release(mcs_qnode_t** L, mcs_qnode_t* I)
{
    if (I->next == 0) {
        if (bool_cas((volatile unsigned long*)L, (unsigned long)I, 0))
            return;
        while (I->next == 0) { } // spin
    }
    I->next->flag = false;
}

#endif // ATOMIC_OPS_H__
