#include "types.h"
#include "stat.h"
#include "user.h"

int main()
{
	int pid;
	int i;
	for(i=0;i<10;i++)
	{
		pid=fork();

		if(pid<0)
		{
			printf(1,"fork failed.\n");
			exit();
		}
		else if(pid==0)
		{
			int j,x=0;
			printf(1,"-----Child %d created.-----\n",getpid());
			for(j=0;j<302000000;j++)
				x=x+1*2+1;
			for(j=0;j<302000000;j++)
				x=x+1*2+1;
			printf(1,"-----%d, %d-----\n\n",x,getpid());
			exit();
		}
	}
	for(i=0;i<10;i++)
		wait();
	exit();
}
