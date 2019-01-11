#pragma once

#include <common.h>

#define RB_FETCH(r) r[0]
#define RB_STORE(r) r[1]
#define RB_SIZE(r)	r[2]

#define RB_OFFSET	3

#define DECLARE_RINGBUFFER(name, size) u32 name[(size) + RB_OFFSET]

static inline u32
rb_mod(u32 n, u32 s) {
	return (n >= s) ? (n - s) : n;
}

static inline int
ringbuffer_full(const u32 *r) {
	return rb_mod(RB_FETCH(r) + 1, RB_SIZE(r)) == RB_STORE(r);
}

static inline int
ringbuffer_empty(const u32 *r) {
	return RB_FETCH(r) == RB_STORE(r);
}

static inline void
ringbuffer_store(u32 *r, void *item) {
	r[RB_STORE(r) + RB_OFFSET] = (u32)item;
	RB_STORE(r) = rb_mod(RB_STORE(r) + 1, RB_SIZE(r));
}

static inline void*
ringbuffer_fetch(u32 *r) {
	void *ret = (void*)r[RB_FETCH(r) + RB_OFFSET];
	RB_FETCH(r) = rb_mod(RB_FETCH(r) + 1, RB_SIZE(r));
	return ret;
}

static inline void
ringbuffer_init(u32 *r, u32 sz) {
	RB_FETCH(r) = RB_STORE(r) = 0;
	RB_SIZE(r) = sz;
}
