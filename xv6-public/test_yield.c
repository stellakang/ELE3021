#include "types.h"
#include "stat.h"
#include "user.h"

int main()
{

	int pid=fork();
//	int num=0;
	if(pid<0)
	{
	    printf(1,"fork failed\n");
	    exit();
	}
	else if(pid==0)
	{
	   
	    while(1)
	    {
//		if(num++==25)
//			break;
		printf(1,"Child\n");
		yield();
	    }
	    exit();
	}
	else
	{
	    while(1)
	    {
//		if(num++==25)
//			break;
		printf(1,"Parent\n");
		yield();
	    }
	    wait();
	}

    return 0;
}

