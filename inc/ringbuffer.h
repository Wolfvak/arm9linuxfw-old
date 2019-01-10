#pragma once

#include <common.h>

typedef struct {
	u32 fetch, store, count, size;
	void **ring;
} ringbuffer;

#define DEFINE_RINGBUFFER(name, count) \
static void *name##_buffer[(count)]; \
static ringbuffer name = { .fetch = 0, .store = 0, .size = (count), .ring = name##_buffer}

static inline int
ringbuffer_full(const ringbuffer *ring) {
	return ring->count == ring->size;
}

static inline int
ringbuffer_empty(const ringbuffer *ring) {
	return ring->count == 0;
}

static inline void
ringbuffer_store(ringbuffer *ring, void *item) {
	ring->ring[ring->store] = item;
	ring->store += 1;
	ring->store %= ring->size;
	ring->count += 1;
}

static inline void*
ringbuffer_fetch(ringbuffer *ring) {
	void *ret = ring->ring[ring->fetch];
	ring->fetch += 1;
	ring->fetch %= ring->size;
	ring->count -= 1;
	return ret;
}

static inline void
ringbuffer_clear(ringbuffer *ring) {
	ring->fetch = ring->store = ring->count = 0;
}
