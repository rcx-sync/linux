#ifndef _LINUX_MMAP_LOCK_H
#define _LINUX_MMAP_LOCK_H

#include <linux/lockdep.h>
#include <linux/mm_types.h>
#include <linux/rcx.h>
#include <linux/rwsem.h>

#ifdef CONFIG_MMAP_RWSEM

static inline int mmap_is_locked(struct mm_struct *mm)
{
	return rwsem_is_locked(&mm->mmap_sem);
}

#define mmap_might_lock_read(mm) \
	might_lock_read(&mm->mmap_sem)

#define mmap_lockdep_assert_held_exclusive(mm)	do {	\
		lockdep_assert_held(&mm->mmap_sem);		\
	} while (0)

static inline void mmap_read_lock(struct mm_struct *mm)
{
	down_read(&mm->mmap_sem);
}

static inline int mmap_read_trylock(struct mm_struct *mm)
{
	return down_read_trylock(&mm->mmap_sem);
}

static inline void mmap_read_unlock(struct mm_struct *mm)
{
	up_read(&mm->mmap_sem);
}

static inline void mmap_write_lock(struct mm_struct *mm)
{
	down_write(&mm->mmap_sem);
}

static inline int mmap_write_trylock(struct mm_struct *mm)
{
	return down_write_trylock(&mm->mmap_sem);
}

static inline int mmap_write_lock_killable(struct mm_struct *mm)
{
	return down_write_killable(&mm->mmap_sem);
}

static inline void mmap_write_lock_nested(struct mm_struct *mm, int subclass)
{
	down_write_nested(&mm->mmap_sem, subclass);
}

#define mmap_write_nest_lock(sem, mm)	\
	down_write_nest_lock(sem, &mm->mmap_sem)

static inline void mmap_write_downgrade(struct mm_struct *mm)
{
	downgrade_write(&mm->mmap_sem);
}

static inline void mmap_write_unlock(struct mm_struct *mm)
{
	up_write(&mm->mmap_sem);
}

#elif defined(CONFIG_MMAP_SPINLOCK)

static inline int mmap_is_locked(struct mm_struct *mm)
{
	return spin_is_locked(&mm->mm_spinlock);
}

#define mmap_might_lock_read(mm) do { } while (0)

#define mmap_lockdep_assert_held_exclusive(mm)	do { } while (0)

static inline void mmap_read_lock(struct mm_struct *mm)
{
	spin_lock(&mm->mm_spinlock);
}

static inline int mmap_read_trylock(struct mm_struct *mm)
{
	return spin_trylock(&mm->mm_spinlock);
}

static inline void mmap_read_unlock(struct mm_struct *mm)
{
	spin_unlock(&mm->mm_spinlock);
}

static inline void mmap_write_lock(struct mm_struct *mm)
{
	spin_lock(&mm->mm_spinlock);
}

static inline int mmap_write_trylock(struct mm_struct *mm)
{
	return spin_trylock(&mm->mm_spinlock);
}

static inline int mmap_write_lock_killable(struct mm_struct *mm)
{
	spin_lock(&mm->mm_spinlock);
	return 0;
}

static inline void mmap_write_lock_nested(struct mm_struct *mm, int subclass)
{
	spin_lock(&mm->mm_spinlock);
}

#define mmap_write_nest_lock(sem, mm) spin_lock(&mm->mm_spinlock)

static inline void mmap_write_downgrade(struct mm_struct *mm)
{
	spin_unlock(&mm->mm_spinlock);
	spin_lock(&mm->mm_spinlock);
}

static inline void mmap_write_unlock(struct mm_struct *mm)
{
	spin_unlock(&mm->mm_spinlock);
}

#else

static inline int mmap_is_locked(struct mm_struct *mm)
{
	return rcx_locked(&mm->mm_rcx);
}

#define mmap_might_lock_read(mm) do { } while (0)

#define mmap_lockdep_assert_held_exclusive(mm)	do { } while (0)

static inline void mmap_read_lock(struct mm_struct *mm)
{
	rcx_lock(&mm->mm_rcx);
}

static inline int mmap_read_trylock(struct mm_struct *mm)
{
	return rcx_trylock(&mm->mm_rcx);
}

static inline void mmap_read_unlock(struct mm_struct *mm)
{
	rcx_unlock(&mm->mm_rcx);
}

static inline void mmap_write_lock(struct mm_struct *mm)
{
	rcx_lock(&mm->mm_rcx);
}

static inline int mmap_write_trylock(struct mm_struct *mm)
{
	return rcx_trylock(&mm->mm_rcx);
}

static inline int mmap_write_lock_killable(struct mm_struct *mm)
{
	rcx_lock(&mm->mm_rcx);
	return 0;
}

static inline void mmap_write_lock_nested(struct mm_struct *mm, int subclass)
{
	rcx_lock(&mm->mm_rcx);
}

#define mmap_write_nest_lock(sem, mm) rcx_lock(&mm->mm_rcx)

static inline void mmap_write_downgrade(struct mm_struct *mm)
{
	rcx_unlock(&mm->mm_rcx);
	rcx_lock(&mm->mm_rcx);
}

static inline void mmap_write_unlock(struct mm_struct *mm)
{
	rcx_unlock(&mm->mm_rcx);
}


#endif

#endif	/* _LINUX_MMAP_LOCK_H */
