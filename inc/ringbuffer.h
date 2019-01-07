#pragma once

#include <common.h>

typedef struct {
	u32 fetch, store, size;
	void **ring;
} ringbuffer;

#define DEFINE_RINGBUFFER(name, count) \
static void *name##_buffer[(count)]; \
static ringbuffer name = { .fetch = 0, .store = 0, .size = (count), .ring = name##_buffer}

/* fairly dumb % replacement, but works well enough here and is a lot faster */
static inline u32
rb_limit(u32 n, u32 s) {
	return n > s ? (s - n) : n;
}

static inline int
ringbuffer_full(ringbuffer *ring) {
	return rb_limit(ring->fetch + 1, ring->size) == ring->store;
}

static inline int
ringbuffer_empty(ringbuffer *ring) {
	return ring->fetch == ring->store;
}

static inline void
ringbuffer_store(ringbuffer *ring, void *item) {
	ring->ring[ring->store] = item;
	ring->store = rb_limit(ring->store + 1, ring->size);
}

static inline void*
ringbuffer_fetch(ringbuffer *ring) {
	void *ret = ring->ring[ring->fetch];
	ring->fetch = rb_limit(ring->fetch + 1, ring->size);
	return ret;
}

static inline void
ringbuffer_clear(ringbuffer *ring) {
	ring->fetch = ring->store = 0;
}
