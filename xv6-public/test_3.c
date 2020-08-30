#include "types.h"
#include "user.h"
#include "stat.h"

int main()
{
  printf(1,"%p\n",sbrk(0));

  printf(1,"%p\n",sbrk(1000));

  printf(1,"%p\n",sbrk(-500));

  exit();
}
