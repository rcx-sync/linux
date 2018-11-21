/* SPDX-License-Identifier: GPL-2.0 */
/*
 * RCX
 *
 * Note: The code is for x86-64, quad numa node systems only for now
 *
 * Copyright SeongJae Park, 2018
 *
 * Author: SeongJae Park <sj38.park@gmail.com>
 */

#ifndef _LINUX_RCX_H
#define _LINUX_RCX_H

#include <linux/rtm.h>
#include <linux/topology.h>

/* Intel prefetches next cache line */
#define RCX_PADDING	128
#define RCX_NR_NODES	4

struct rcxlock {
	/* For quad-node only */
	char __attribute__((aligned(RCX_PADDING))) pndlocks[
		RCX_PADDING * RCX_NR_NODES];
#ifdef CONFIG_RCX_SLEEPABLE
	char __attribute__((aligned(RCX_PADDING))) locker_node;
#endif
	spinlock_t __attribute__((aligned(RCX_PADDING))) glblock;
};

#define __RCXLOCK_UNLOCKED(x) \
	{ .pndlocks = {0,}, .glblock = __SPIN_LOCK_UNLOCKED(x.glblock) }

#define DEFINE_RCXLOCK(x) struct rcxlock x = __RCXLOCK_UNLOCKED(x)

/* Get per-node lock for current node */
#define pndlockof(lock) \
	((lock)->pndlocks[RCX_PADDING * numa_node_id()])

/* Get per-node lock for specific node */
#define pndlockofnd(lock, nodeid) \
	((lock)->pndlocks[RCX_PADDING * nodeid])

#define rcx_pnd_locked(rcxlock) \
	(pndlockof(rcxlock) == 1)

/* Lock per-node lock */
#define rcx_pnd_lock(rcxlock) \
	WRITE_ONCE(pndlockof(rcxlock), 1);

#ifdef CONFIG_RCX_SLEEPABLE
/* Unlock per-node lock */
#define rcx_pnd_unlock(rcxlock) \
	WRITE_ONCE(pndlockofnd(rcxlock, rcxlock->locker_node), 0);

/* Lock global lock */
#define rcx_glb_lock(rcxlock) \
	spin_lock(&rcxlock->glblock); \
	rcxlock->locker_node = numa_node_id();

#else

#define rcx_pnd_unlock(rcxlock) \
	WRITE_ONCE(pndlockof(rcxlock), 0);

#define rcx_glb_lock(rcxlock) \
	spin_lock(&rcxlock->glblock);
#endif

/* Unlock global lock */
#define rcx_glb_unlock(rcxlock) \
	spin_unlock(&rcxlock->glblock);

#define rcx_xbegin \
	if (_xbegin() == _XBEGIN_STARTED)

#define rcx_aborted \
	else

#define RCX_EXPLICIT_ABORT 1

#define rcx_abort() \
	_xabort(RCX_EXPLICIT_ABORT);

#define rcx_commit() \
	_xend();

static inline void rcx_lock_init(struct rcxlock *lock)
{
	int nd;

	for (nd = 0; nd < RCX_NR_NODES; nd++)
		WRITE_ONCE(pndlockofnd(lock, nd), 0);
	lock->locker_node = 0;
	spin_lock_init(&lock->glblock);
}

/* Returns 1 if lock is locked, 0 otherwise. */
static inline int rcx_locked(struct rcxlock *lock)
{
	int nd;

	for (nd = 0; nd < RCX_NR_NODES; nd++)
		if (pndlockofnd(lock, nd) != 0)
			return 1;
	return spin_is_locked(&lock->glblock);
}

/* Returns 1 if successful, 0 if contention */
static inline int rcx_trylock(struct rcxlock *lock)
{
	if (rcx_pnd_locked(lock))
		return 0;

	rcx_xbegin {
		if (rcx_pnd_locked(lock))
			rcx_abort();

		rcx_pnd_lock(lock);
		rcx_commit();
	} rcx_aborted {
		return 0;
	}

	rcx_glb_lock(lock);
	return 1;
}

static inline void rcx_lock(struct rcxlock *lock)
{
retry:
	while (rcx_pnd_locked(lock))
		;

	rcx_xbegin {
		if (rcx_pnd_locked(lock))
			rcx_abort();

		rcx_pnd_lock(lock);
		rcx_commit();
	} rcx_aborted {
		goto retry;
	}

	rcx_glb_lock(lock);
}

static inline void rcx_unlock(struct rcxlock *lock)
{
	rcx_pnd_unlock(lock);
	rcx_glb_unlock(lock);
}


#endif	/* _LINUX_RCX_H */
