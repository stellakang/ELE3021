# Assignment1 : shell  
## 문제 정의와 이해  

1. 2가지 모드가 존재한다. - Interactive mode & Batch mode  
*Interactive mode* :  prompt를 나타내어 사용자가 명령어를 입력하는 방식으로 명령어를 입력하면 해당 결과값을 출력한다.    
*Batch mode* : 실행할 때 입력한 batch file에 포함된 명령어를 실행하는 방식으로 한 줄씩 출력하고 해당 결과값을 나타낸다.  

2. 한 줄에는 여러 개의 명령어가 들어올 수 있으며 각 명령어는 ;로 구분한다.  
- 결과는 섞여서 출력되며 모든 명령어가 실행될 때까지 기다린다.   

3. quit이나 파일의 끝이거나 Ctrl+D가 입력될 경우에는 쉘은 더 이상 명령어를 입력받지 않고 종료한다.  

4. Makefile을 통해 컴파일이 이뤄진다.        
       
   
## 함수 및 코드에 대한 설명    
        
          
### 쉘 구현 방식(전반적인)    

 - 파일이 입력되었으면 batch_mode() 함수를 호출하고 입력되지 않으면 interactive_mode() 함수를 호출한다.      
 - 입력된 한 줄, 또는 파일에 주어진 한 줄을 ;단위로 parsing한다.   
 - parsing할 때마다 fork()를 통해 자식 프로세스를 생성한 후에 해당 명령어를   ' ' 단위로 parsing한 후 execvp( , , )를 호출하여 명령어를 실행한다.   
 - 부모 프로세스는 자식 프로세스들이 명령어 실행을 모두 완료할 때까지 wait()을 통해 대기하며,   
종료 신호를 주기 전까지 이 과정을 반복한다. ( 이 외에 quit에 대한 처리 등 구체적인 것은 아래에서 함수 단위로 설명)<br />  

### interactive_mode()    
```c
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
```    

 - interactive mode는 prompt를 나타내어 사용자의 입력을 command배열에 받는다.   
 - fgets는 마지막의 '\n'까지 받아오므로 이를 Null을 의미하는 '\0'으로 바꾼다.   
 - ;를 단위로 parsing을 하기 위해 strtok에 command 인자를 넣어준다.   
 - 예외 처리    
   - 사용자가 Ctrl+D를 입력할 경우, 더 이상 명령어 입력을 받지 않고 종료해야 하므로 fgets의 리턴값이 NULL일 경우 break를 통해 종료한다.   
   - ptr이 Null값인 경우에는 strcmp를 사용할 때 에러가 나므로 Null값이 아닌 경우에 문자열 "quit"과 비교하여 일치할 경우, break를 통해 종료한다.  
 - 이후 parsing을 계속 진행하기 위해 parsing(ptr)을 호출한다.   

### batch_mode()  
```c
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
```   

 - batch mode는 파일을 열어 한 줄씩 읽어와서 parsing을 진행한다.  
 - 한 줄의 문자열은 contents에 담기며 역시 마지막 '\n'를 '\0'으로 바꿔준다.  
 - ;를 단위로 parsing을 하기 위해 strtok에 command 인자를 넣어준다.    
 - 예외 처리 : ptr이 Null값인 경우에는 strcmp를 사용할 때 에러가 나므로 Null값이 아닌 경우에 문자열 "quit"과 비교하여 일치할 경우, break를 통해 종료한다.   
 - 이후 parsing을 계속 진행하기 위해 parsing(ptr)을 호출한다.   

### parsing(char* ptr)   
 ```c
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
``` 
 
 - ;로 parsing한 각 명령어를 실행하기 위해서 fork()함수로 child process를 만든다.   
 - 리턴 값을 이용하여 pid<0(fork 실패), pid==0(자식 프로세스), pid>0(부모 프로세스) 으로 나누어 코드를 작성한다.   
 - 자식 프로세스의 경우, 명령어를 실행해야 하기 때문에 스페이스 단위로 parsing하기 위해 word_parsing함수를 호출한다.   
 - 부모 프로세스의 경우, 해당 명령어가 마지막 번째인 경우에, wait 함수를 호출하여 여러 명령어가 동시에 처리되어 intermixed된 결과값을 출력하도록 한다.    
 

### word_parsing(char* ptr)  
```c
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
```   

 - ;로 parsing한 하나의 명령어를 execvp를 통해 실행하기 위해서 단어별로 parsing하는 함수이다.  
 - strtok를 통해 단어 별로 분리하여 argument배열에 하나씩 저장한다. 이때, num에 해당 인덱스 값을 저장한다.  
 - execvp함수를 호출하여 해당 명령어를 실행한다.  
  
## Trouble Shooting    

- fgets의 경우 '\n'까지 입력받기 때문에 ;단위로 parsing하기 위해서는 맨 처음에 '\n'을 '\0'으로 바꿔주는 과정이 필요하다.   
- 여러 명령어를 입력했을 때, 결과가 서로 섞여서 출력되기 위해서는 더 이상 처리할 명령어가 없는 경우에만 wait명령어를 실행해야 한다.  


                                            
