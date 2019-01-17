#include <common.h>
#include <hw/timer.h>

#define TIMER_BASE	((vu16*)(0x10003000))

#define TIMER_VAL(n)	TIMER_BASE[((n) * 2)]
#define TIMER_CNT(n)	TIMER_BASE[1 + ((n) * 2)]

static void
timer_reset(void)
{
	for (int i = 0; i < 2; i++)
		TIMER_CNT(i) = 0;

	for (int i = 0; i < 2; i++)
		TIMER_VAL(i) = 0;
}

void
timer_start(void)
{
	timer_reset();

	for (int i = 0; i < 4; i++)
		TIMER_CNT(i) = 0;

	TIMER_VAL(1) = 0;
	TIMER_VAL(0) = 0;

	TIMER_CNT(1) = BIT(2) | BIT(7);
	TIMER_CNT(0) = BIT(7);
}

void
timer_stop(void)
{
	for (int i = 0; i < 2; i++)
		TIMER_CNT(i) = 0;
}

u32
timer_read(void)
{
	return (TIMER_VAL(1) << 16UL) | TIMER_VAL(0);
}
