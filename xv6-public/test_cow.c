#include "types.h"
#include "stat.h"
#include "user.h"


int main()
{

  int pid=fork();
  int a=0;
  if(pid==0)
  {
    a++;
    exit();
  }
  else
  {
    wait();
  }
  exit();
}
