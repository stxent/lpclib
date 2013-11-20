/*
 * startup.c
 * Copyright (C) 2013 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

extern unsigned long _etext;
extern unsigned long _data;
extern unsigned long _edata;
extern unsigned long _bss;
extern unsigned long _ebss;
/*----------------------------------------------------------------------------*/
#ifdef __REDLIB__
extern int __main(void); /* Redlib */
#else
extern int main(void); /* Newlib */
#endif
/*----------------------------------------------------------------------------*/
void RESET_ISR(void)
{
  register unsigned long *dst __asm__ ("r0");
  register unsigned long *src __asm__ ("r1");

  /* Copy the data segment initializers from flash to RAM */
  for (dst = &_data, src = &_etext; dst < &_edata;)
    *dst++ = *src++;

  /* Zero fill the BSS segment */
  for (dst = &_bss; dst < &_ebss;)
    *dst++ = 0;

#if defined(__cplusplus)
  __libc_init_array(); /* Call C++ library initialization */
#endif

#ifdef __REDLIB__
  __main();
#else
  main();
#endif

#if defined(__cplusplus)
  __libc_fini_array();
#endif

  while (1);
}