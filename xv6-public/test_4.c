#include "types.h"
#include "stat.h"
#include "user.h"

int test=1;

int main()
{
  printf(1,"test\n");

  printf(1,"%d free pages\n",get_n_free_pages());

  int pid=fork();
  if(pid==0)
  {
    printf(1,"Childprocess --> test: %d\n",get_n_free_pages());
    test++;
    printf(1,"hello world\n");
    
    printf(1, "change value : %d free pages\n",get_n_free_pages());
    exit();
   }
   
   printf(1,"Parentprocess --> test: %d\n",test);

   wait();
     
   printf(1, "parent wait : %d free pages\n",get_n_free_pages());
  


  exit();
}

