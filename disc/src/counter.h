/*
 * $Id: counter.h,v 1.2 2003/04/18 17:38:19 bogdan Exp $
 *
 * 2003-02-19 added by bogdan
 *
 */


#ifndef _AAA_DIAMETER_COUNTER_H
#define _AAA_DIAMETER_COUNTER_H


#ifdef CONFIG_SMP
#define LOCK "lock ; "
#else
#define LOCK ""
#endif

typedef struct { volatile int value; } atomic_cnt;

/**
 * atomic_read - read atomic variable
 * @v: pointer of type atomic_cnt
 * 
 * Atomically reads the value of @c.  Note that the guaranteed
 * useful range of an atomic_t is only 24 bits.
 */ 
#define atomic_read(c)		((c)->value)

/**
 * atomic_set - set atomic variable
 * @v: pointer of type atomic_t
 * @i: required value
 *
 * Atomically sets the value of @v to @i.  Note that the guaranteed
 * useful range of an atomic_t is only 24 bits.
 */
#define atomic_set(c,i)     (((c)->value) = (i))

/**
 * atomic_inc - increment atomic variable
 * @c: pointer of type atomic_cnt
 * 
 * Atomically increments @c by 1.  Note that the guaranteed
 * useful range of an atomic_t is only 24 bits.
 */ 
static __inline__ void atomic_inc(atomic_cnt *c)
{
	__asm__ __volatile__(
		LOCK "incl %0"
		:"=m" (c->value)
		:"m" (c->value));
}

/**
 * atomic_dec - decrement atomic variable
 * @c: pointer of type atomic_cnt
 * 
 * Atomically decrements @c by 1.  Note that the guaranteed
 * useful range of an atomic_t is only 24 bits.
 */ 
static __inline__ void atomic_dec(atomic_cnt *c)
{
	__asm__ __volatile__(
		LOCK "decl %0"
		:"=m" (c->value)
		:"m" (c->value));
}

/**
 * atomic_dec_and_test - decrement and test
 * @c: pointer of type atomic_cnt
 * 
 * Atomically decrements @c by 1 and
 * returns true if the result is 0, or false for all other
 * cases.  Note that the guaranteed
 * useful range of an atomic_t is only 24 bits.
 */ 
static __inline__ int atomic_dec_and_test(atomic_cnt *c)
{
	unsigned char v;

	__asm__ __volatile__(
		LOCK "decl %0; sete %1"
		:"=m" (c->value), "=qm" (v)
		:"m" (c->value) : "memory");
	return v != 0;
}

#endif
