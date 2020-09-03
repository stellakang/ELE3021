# Assignment2: scheduler  

코드와 사진은 해당 코드, 사진의 밑에 있는 설명과 짝을 이룹니다.  

## Design  

### 1) FCFS scheduler  
* 프로세스를 생성한 시점을 저장하는 변수를 만든다. (creation time을 의미하는 ctime을 만들었다. )  
* ctime에는 ticks를 저장하며 가장 작은 값을 갖는 프로세스가 제일 먼저 생성되었음을 의미한다.  
* 매번 스케줄링이 일어날 때마다 RUNNABLE한 프로세스들 중에 가장 작은 ctime을 갖는 프로세스가 실행되도록 한다.  
* 기존 xv6의 경우 1tick마다 스케줄링이 일어나므로 이를 그대로 두고 1tick마다 스케줄링이 일어나도록 했다. (1tick마다 스케줄링 하더라도 RUNNABLE한 모든 프로세스의 ctime을 비교하므로 FCFS scheduler를 만족한다.)  

### 2) PRIORITY scheduler   
* 프로세스의 우선순위를 저장하는 priority(uint) 변수를 만든다. (편의를 위해 MLFQ에서 priority를 저장하는 변수는 qlev로 생성하여 서로 다른 변수에 값을 저장하도록 했다. )  
* 우선순위 스케줄러에 따라 다음에 실행할 프로세스를 저장하는 struct proc *firstP 를 생성했다.  
* 같은 우선순위에 대해서 rr정책을 만족하기 위해서 이중포문이 필요하며(구현 부분에서 자세히 서술), 바깥 포문은 p에, 안쪽 포문은 newP에 프로세스를 대입했다.  
* void setpriority(int pid, int priority) 시스템 콜을 추가했다. (자세한 구현은 2번에서 서술)   

### 3) MLFQ scheduler (gaming 막는 방법 이 부분에 서술)   
* 프로세스가 현재 해당 level에서 실행한 tick을 저장하는 tick_quantum변수를 만든다. (만약 lev1에서 2tick만큼 실행되었다면 값이 2가 대입된다. time interrupt가 발생하거나 yield 시스템콜을 호출하면 0으로 바꿔주며 관련 구현에 대해서는 2번에서 서술했다. )   
* 각 프로세스가 현재 위치한 qlevel을 저장하는(priority) qlev변수를 만든다.   
* MLFQ scheduler는 level0부터 탐색하여 존재하는 프로세스를 실행시킨다. 다음에 실행시킬 프로세스를 탐색하는 struct proc* mlfq_find_process(int *lev0, int *lev1, int *lev2, int *qlevel) 함수를 정의한다.   
* qlevel을 반환하는 int getlev(void) 함수를 정의하며 시스템 콜로 추가한다.   
* 각 프로세스가 수행한 tick을 증가시키는 int inc_tick(void) 함수를 정의한다.   
* 테스트 프로그램을 수행하기 위해 yield() 시스템 콜을 추가한다. (저번 과제로 수행했음.)    
* time quantum을 모두 채웠을 경우 프로세스를 아래 큐 레벨(레벨의 숫자는 커지고 우선순위는 낮아짐)로 이동시켜야 하므로 해당 기능을 수행하는 void dec_qlev(void) 함수를 정의한다.   
*  tick이 100이 지났을 경우, boosting을 실행해야 하므로 해당 기능을 수행하는 void mlfq_boost(void) 함수를 정의한다.   
* 그 외에 fork, allocproc에 대해 프로세스의 qlev, tick_quantum값을 초기화 시켜주며 yield 시스템콜을 호출할 경우에는 tick_quantum값을 0으로 초기화한다. (관련 이유 및 구현은 2번에서 설명)   
* trap.c에 해당 큐의 time quantum을 만족했을 때 qlevel 낮춰주고 yield하거나 boosting에 대한 구현을 했다.     
* gaming은 막지 않는다. -> gaming을 막는 방법 :  time quantum과 관계없이 프로세스가 cpu 사용한 시간을 확인해서 일정 수준을 넘어가면 우선순위를 낮춰준다.    

## Implementation

```c++
struct proc {
  uint sz;                     // Size of process memory (bytes)
  pde_t* pgdir;                // Page table
  char *kstack;                // Bottom of kernel stack for this process
  enum procstate state;        // Process state
  int pid;                     // Process ID
  struct proc *parent;         // Parent process
  struct trapframe *tf;        // Trap frame for current syscall
  struct context *context;     // swtch() here to run process
  void *chan;                  // If non-zero, sleeping on chan
  int killed;                  // If non-zero, have been killed
  struct file *ofile[NOFILE];  // Open files
  struct inode *cwd;           // Current directory
  char name[16];               // Process name (debugging)
  uint ctime;		       // Process creation time
  int priority;		       // Process priority
  int qlev;		       // Process qlevel
  int tick_quantum;	       // tick count for checking quantum
};

```
- 위 사진은 세가지 스케줄러를 구현한 후의 프로세스 구조체의 모습입니다.  

### FCFS scheduler  
* proc.h의 proc에 ctime변수를 추가한다.  
```c++
#elif FCFS_SCHED
    struct proc* minP=0;
    for(p=ptable.proc;p<&ptable.proc[NPROC];p++){
      if(p->state!=RUNNABLE)
	continue;
      if(minP!=0){
	if(p->ctime<minP->ctime)
		minP=p;
      }
      else
	minP=p;
    }
    if(minP){
      p=minP;
      c->proc=p;
      switchuvm(p);
      p->state=RUNNING;
      //cprintf("%d %p\n",p->pid,p->ctime);
      swtch(&(c->scheduler), p->context);
      switchkvm();

      c->proc=0;
    }
```
* RUNNABLE인 프로세스에 대해서 ctime이 가장 작은 프로세스를 minP에 저장한 후 해당 프로세스로 전환한다.  minP의 초기값은 NULL값이므로 minP가 NULL이 아닐 때에는 minP의 ctime과 p의 ctime을 비교하고 NULL 일 때에는 minP에 p를 대입한다.  
  
### PRIORITY scheduler   
* proc.h의 proc에 priority변수를 추가한다.  
```c++
#elif PRIORITY_SCHED
    
    struct proc *firstP=0;
    struct proc *newP=0;

    for(p=ptable.proc;p<&ptable.proc[NPROC];p++){
      if(p->state!=RUNNABLE)
	continue;

      firstP=p;

      for(newP=ptable.proc;newP<&ptable.proc[NPROC];newP++){
	if(newP->state==RUNNABLE&&newP->priority>firstP->priority)
	  firstP=newP;
      }
    
      newP=firstP;
      c->proc=newP;
      switchuvm(newP);
      newP->state=RUNNING;
      swtch(&(c->scheduler),newP->context);
      switchkvm();

      c->proc=0;

    }
```
* 이중 포문을 구성한다. 첫번째 포문에서는 p에 프로세스를 대입하며 RUNNABLE인 프로세스에 대해서 firstP에 p를 일단 대입한다.  
* newP에 프로세스를 순차적으로 대입하여 RUNNABLE인 프로세스에 대해서 p보다 우선순위가 높은 newP가 존재하면 firstP를 newP로 바꿔준다.  
* 이 방식을 사용하면 같은 우선순위를 가진 프로세스에 대해서 round robin 정책을 쓸 수 있다.  
```c++
void 
setpriority(int pid, int priority)
{
  struct proc *p;
  acquire(&ptable.lock);
  for(p=ptable.proc;p<&ptable.proc[NPROC];p++){
    if(p->pid==pid) {
        p->priority=priority;
	break;
    }
  }
  release(&ptable.lock);
}
```  
```c++
int 
sys_setpriority(void)
{
  int pid,priority;
  if(argint(0,&pid)<0)
    return -1;
  if(argint(1,&priority)<0)
    return -1;
  setpriority(pid,priority);
  return 0;
}
```  
* setpriority 함수를 정의하며 시스템콜로 추가하기 위한 여러 과정을 수행한다. (과정에 대한 코드 일부 생략)     


### MLFQ scheduler  

* 1번의 design에서 서술한 tick_quantum, qlev 변수를 만든다.   

```c++
struct proc*
mlfq_find_process(int *lev0,int *lev1, int *lev2, int* qlevel)
{
  struct proc* proc;
  
  for(;;){
    int i;
    for(i = 0; i < NPROC; i++){
      if((*qlevel)==0){
        proc=&ptable.proc[(*lev0+i)%NPROC];
	if(proc->state==RUNNABLE && proc->qlev==0){
          *lev0=(*lev0+1+i)%NPROC;
          return proc;
        }  
      }else if(*qlevel==1){
        proc=&ptable.proc[(*lev1+i)%NPROC];
        if(proc->state==RUNNABLE && proc->qlev==1){
          *lev1=(*lev1+1+i)%NPROC;
          return proc;
        }
      }else if(*qlevel==2){
        proc=&ptable.proc[(*lev2+i)%NPROC];
        if(proc->state==RUNNABLE && proc->qlev==2){
          *lev2=(*lev2+1+i)%NPROC;
          return proc;
        }
      }
    }
    if(*qlevel==2)
      return 0;
    else
      (*qlevel)+=1;
  }
  return 0;
}
```
* 위 사진은 mlfq_find_process 함수이다. 각각의 큐 레벨에서는 rr정책이 지켜져야 하므로 각 큐에서 가리키는 프로세스의 위치를 저장하기 위해 int level0, int level1, int level2 을 만들었다.    
* 이 함수에 변수들의 주소를 넘겨주면 각 값을 변경하고 함수를 종료해도 남아있게 된다.  
* 각 레벨에서는 그 레벨에서 존재하는 RUNNABLE한 프로세스를 찾아 실행한다.  
* qlevel은 0을 초기값으로 하고 각 레벨에서 프로세스를 찾지 못했을 경우에는 qlevel을 1증가시킨다.(qlevel이 이미 2일 경우에는 증가시키지 않는다.)   
```c++
int qlevel=0;

    p=mlfq_find_process(&lev0,&lev1,&lev2,&qlevel);
    if(p){
      c->proc=p;
      switchuvm(p);
      p->state=RUNNING;
      swtch(&(c->scheduler), p->context);
      switchkvm();
      
      c->proc=0;    
    }      
```
* mlfq_find_process 함수를 호출해서 리턴된 p를 다음 프로세스로 전환한다.  
```c++
int 
getlev(void)
{
  return myproc()->qlev;
}
```
* getlev 함수는 각 프로세스의 qlevel을 리턴하고 시스템콜로 추가하기 위한 여러 과정을 실행했다.  
```c++
void
dec_qlev(void)
{
  if(myproc()->qlev==2)
    myproc()->qlev=2;
  else
  {
    myproc()->qlev++;
  }
}
```
* qlevel을 낮추는 과정이다. 2인 경우에는 증가시키지 않고 0이나 1인 경우만 증가시킨다.   
```c++
int
inc_tick()
{
  return myproc()->tick_quantum++;
}
```
* 각 프로세스의 tick_quantum을 증가시키는 과정이다.  
```c++
void
mlfq_boost()
{
  struct proc* p;
  acquire(&ptable.lock);
  for(p=ptable.proc;p<&ptable.proc[NPROC];p++){
    if(p){
      p->qlev=0;
      p->tick_quantum=0;
    }
  }
  release(&ptable.lock);
}
```
* mlfq_boost()함수를 호출하여 tick이 100이 될 때마다(정확히는 0보다 큰 100의 배수가 될 때마다) 모든 프로세스의 qlev과 tick_quantum을 0으로 초기화 한다.  
```c++
#ifdef MLFQ_SCHED
    if(myproc() && myproc()->state==RUNNING &&
       tf->trapno == T_IRQ0+IRQ_TIMER){
      inc_tick();
      if(myproc()->qlev==0&&myproc()->tick_quantum==2){
        dec_qlev();
        myproc()->tick_quantum=0;
      }else if(myproc()->qlev==1&&myproc()->tick_quantum==4){
        dec_qlev();
        myproc()->tick_quantum=0;
      }else if(myproc()->qlev==2&&myproc()->tick_quantum==8){
        dec_qlev();
        myproc()->tick_quantum=0;
      }
      yield(); 
    }
    if(ticks!=0&&ticks%100==0)
      mlfq_boost();
  #else
    if(myproc() && myproc()->state == RUNNING &&
       tf->trapno == T_IRQ0+IRQ_TIMER)
      yield();
```
* trap.c파일에 위 사진의 코드를 추가했다. 일단 tick을 1만큼 증가시킨 후에(inc_tick() 호출)  
* 그 프로세스가 어느 레벨에 있느냐에 따라 if문으로 조건을 추가하여 해당 quantum을 충족할 때에 qlevel을 낮추고(우선순위 낮추고) 프로세스의 tick_quantum을 다시 0으로 바꿔준다.  
* 이 과정을 모두 마치면 yield()를 호출한다.  
* boosting은 ticks가 0이 아니고 100으로 나누었을 때 나머지가 0이면 mlfq_boost()를 호출하여 실행한다.  
```c++
void 
sys_yield(void)
{
  myproc()->tick_quantum=0;
  yield();
}
```
* 시스템콜을 호출한 경우에는 그 프로세스의 tick_quantum을 다시 0으로 초기화 해주어야 한다.  
```c++
np->qlev=0;
  np->tick_quantum=0;
```
* fork에서는 위와 같이 초기화를 해주어야 한다.   
```{.cpp}
p->tick_quantum=0;
  p->qlev=0;
```
* allocproc에서 위와 같이 초기화를 해주어야 한다.   

## Screen shot of the Result  
> 사진 누락  
  
### FCFS scheduler   
- p2_fcfs_test 실행프로그램을 실행한 결과이다.   
- FCFS scheduler는 프로세스가 생성된 순서대로 실행시킨다.   
- 가장 먼저 생성된 9번 프로세스가 주기적으로 sleep하기 때문에 9번 프로세스가 깨어있을 때에는 출력을 하고, sleep인 경우에는 10, 11, 12 13 프로세스 순서대로 실행한다.   
##### 2) PRIORITY scheduler  
- p2_prior_test 실행프로그램을 실행한 결과이다.  
- 우선순위가 높은 프로세스부터 실행되는 모습을 볼 수 있다.
- 높은 우선순위의 프로세스부터 실행하되, 같은 우선순위의 프로세스가 존재할 경우에는 Round Robin정책에 따라 실행되는 모습을 볼 수 있다.  
- 제일 우선순위가 높은 프로세스가 시간차를 두고 생성되기 때문에 그 전까지는 우선순위가 3인 프로세스가 실행되고 그 이후에는 우선순위가 높은 프로세스부터 실행된다.  
- 주기적으로 sleep하는 프로세스의 우선순위가 제일 높으므로 그 프로세스가 깨어있을 때에는 실행이 되고 sleep의 상태일 경우에는 나머지 프로세스들끼리 우선순위에 따라 실행된다.    
### MLFQ scheduler  
- yield 프로세스의 경우에는 yield가 빈번하게 일어나기 때문에 lev0에서 계속 머무를 확률이 높고 일찍 수행되고 종료된다.  
- 따라서 위와 같은 결과가 나타나고, compute 프로세스의 경우에는 lev0->lev1->lev2로 time quantum이 다 찰 때마다 내려가고  
  yield프로세스가 yield할 때마다 실행되며 tick이 100일 때마다 boosting에 따라 lev0으로 올라가기 때문에 lev2>lev0>lev1 순으로 값이 많은 것을 볼 수 있다.  
- 짧은 compute 프로세스가 빨리 종료되며 lev0에서 2 tick을 시행하고 lev1에서 4tick을 시행하고 lev2에서 8tick을 시행하고 boosting이 일어난다.  
- 따라서 lev0과 lev1의 값은 boosting이 일어나지 않을 떄보다 값이 커지며 lev0에서 2tick, lev1에서 4tick을 시행하니까 lev1의 값이 lev0의 값의 2배로 나타난다.  
## Trouble Shooting  

### #ifdef/elif/endif 등으로 조건부 컴파일 오류(사소한 문제)  
- make clean을 한 후에 make SCHED_POLICY=에 인자를 넣어 make를 수행하면 된다.  

### unexpected trap 14 from cpu 0 eip~ 오류 발생   
- 프로세스가 존재하는지 제대로 된 확인 과정을 거치지 않고 값에 접근하려고 한 경우 이 오류가 발생했던 것 같다.  
- 예를 들어 p에 프로세스를 대입한 경우라면 프로세스가 존재한다는 확인 과정을 거친 후에 (p!=0) 값을 접근해야 한다.  

### priority scheduler에서 round robin 구현   
- 같은 우선순위를 가진 프로세스에 대해서 round robin이 실행되도록 처리하는 것이 구현하기 어려웠다.  
- 이 경우에는 이중 for문으로 구성해서 각 프로세스를 먼저 변수에 넣고 RUNNABLE한 모든 프로세스에 대해서 우선순위가 더 큰 경우에만 프로세스를 바꾸도록 구현하는 해결책을 찾았다.  
### MLFQ scheduler에서 boosting 구현 오류  
- boosting이라는 새로운 변수를 생성하여 ++해주고 100이 될 때마다 boosting을 해주었는데 boosting이 제대로 이뤄지지 않았다. 
- 이 문제를 해결하고 좀 더 코드를 간소화하기 위해서 ticks를 이용해서 ticks의 값이 0이 아니고 100으로 나누었을 때 나머지가 0일 경우에만 boosting을 실행했다.  


