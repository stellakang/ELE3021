#include "types.h"
#include "user.h"
#include "stat.h"

int test=10;

int main()
{
  printf(1,"1. before fork() : %d free pages\n",get_n_free_pages());

  int pid=fork();

  if(pid==0)
  {
    printf(1,"fork 1st : %d free pages\n",get_n_free_pages());

    exit();
  }
  else
  {
    printf(1,"2. before fork() : %d free pages\n",get_n_free_pages());

   int pidd=fork();

   if(pidd==0)
   {
     printf(1,"fork 2nd : %d free pages\n",get_n_free_pages());
     for(int i=1;i<1000;i++)
       test++;
     sbrk(10000);
     printf(1,"hello world\n");
     printf(1,"fork 2nd after change value : %d free pages\n",get_n_free_pages());
     exit();
   }

   wait();
   printf(1,"%d free pages after wait child1\n",get_n_free_pages());
   
  }
  wait();
  printf(1,"%d free pages after wait child2\n",get_n_free_pages());

  exit();
}
