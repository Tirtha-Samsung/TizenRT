#include <tinyara/config.h>

#ifndef BOARD_QEMU

#define BOARD_QEMU
#ifdef CONFIG_ARCH_LEDS
void board_autoled_on(int led);
#else
# define board_autoled_on(led)
#endif

#endif
