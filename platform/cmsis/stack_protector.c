#include <stdint.h>
#include <string.h>
#include <hal_trace.h>

uint32_t __stack_chk_guard = 0xdeadbeef;

void
__attribute__((__constructor__))
__stack_chk_init (void)
{
  if (__stack_chk_guard != 0)
    return;
}

void
__attribute__((__noreturn__))
__stack_chk_fail (void)
{
  const char *msg = "*** stack smashing detected ***: terminated\n";
  ASSERT(0, "%s", msg);
}

