#pragma once

#define TIMER_FREQ_BASE (67027964)

void timer_start(void);
void timer_stop(void);
u32 timer_read(void);

static inline u32 ticks_to_ms(u32 ticks) {
	u64 t = ticks;
	return (t * 1000) / TIMER_FREQ_BASE;
}
