#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

void interactive_mode();
void batch_mode();
void parsing(char* ptr);
void word_parsing(char* ptr);

int main(int argc,char* argv[])
{
	int mode=0;//store 1 if interactive mode, 2 if batch mode
	
	if(argv[1]=='\0')
		mode=1;
	else
		mode=2;
	
	if(mode==1)//interactive mode
	{
		interactive_mode();
	}
	else if(mode==2)//batch mode
	{
		batch_mode(argv[1]);
	}
	else
	{
		printf("Error\n");
		exit(0);
	}
	
	return 0;
}

void interactive_mode()
{
	//mode1

	char command[260];

	while(1)
	{	
		printf("prompt> ");
		if(fgets(command,260,stdin)==NULL) //to control ctrl+d 
			break;
		
		command[strlen(command)-1]='\0';//change '\n' to '\0'
		char *ptr=strtok(command,";"); //start parsing with ';'		
		
		if(ptr!=NULL&&!strcmp(ptr,"quit"))//if the command is quit
			break;

		parsing(ptr); //call parsing function 
				
	}
}

void batch_mode(char* file)
{
	// mode2

	FILE *fp;
	char contents[260];
	
	fp=fopen(file,"r");// file open
		
	while((fgets(contents,260,fp))!=NULL)
	{
		contents[strlen(contents)-1]='\0'; // change '\n' to '\0'
		printf("%s\n",contents); // print each command line

		char *ptr=strtok(contents,";"); // start parsing with ';'
		
		if(ptr!=NULL&&!strcmp(ptr,"quit"))
			break;

		parsing(ptr); //call parsing function
			
	}
}

void parsing(char* ptr)
{
	int count=0; //store the number of commands by line

	while(ptr!=NULL)
	{
		pid_t pid;
		
		pid=fork(); //call fork() to create child process
		count++; //count the number of commands by line
				
		if(pid<0)
		{
			printf("fork failed.\n");
			exit(0);
		}
		else if(pid==0)
		{
			//child
			word_parsing(ptr); //call word_parsing function
		}
		else
		{
			//parent

			int state;
			int i;

			ptr=strtok(NULL,";"); //keep parsing with ';'

			if(ptr==NULL)
			{
				for(i=0;i<count;i++)
					wait(&state); // wait if it has no command to execute
			}
		}
	}
}

void word_parsing(char* ptr)
{
	char *word=strtok(ptr," ");
	char *argument[260];
	int num=0;

	while(word!=NULL) // parsing with ' ' using strtok function
	{
		argument[num++]=word; // store each word to array argument
		word=strtok(NULL," ");
	} 

	argument[num]=NULL;
	execvp(argument[0],argument); // execute the command
}


