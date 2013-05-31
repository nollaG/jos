
#include <inc/lib.h>

int a;
void
umain(int argc, char **argv)
{
  envid_t env;
	cprintf("Father:%04x", sys_getenvid());
  a=1;
  if ((env = sfork()) == 0) {
    while (1) {
      cprintf("child:a=%d\n",a);
      sys_yield();
    }
    exit();
  }
  sys_yield();
  sys_yield();
  sys_yield();
  sys_yield();
  sys_yield();
  sys_yield();
  cprintf("Father:a changed\n");
  a=2;
  sys_yield();
  sys_yield();
  sys_yield();
  sys_yield();
  sys_yield();
  sys_yield();
  cprintf("Killing the child\n");
  sys_env_destroy(env);
}

