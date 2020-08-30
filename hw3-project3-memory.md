1. Design
=======================
 1) lazy memory allocation 
-----------------------------------
*  lazy memory allocation이란 os에 메모리를 요청하면 바로 물리적 메모리를 할당하는 것이 아니라 메모리가 사용될 때 할당하는 것을 말합니다.   
여기서는 heap영역에 대한 동적할당에 lazy memory allocation이 구현되도록 합니다.  

* 먼저, growproc 함수에서 물리적 할당이 일어나는 부분을 지우고 할당받은 유저 영역의 메모리 크기만 증가시킵니다.  

  
* 그리고 실제 메모리를 사용하려고 할 때 exception이 발생하는 경우에 대하여 할당을 해주는 코드를 trap.c에 추가해줍니다.  

  
* 이때 page fault를 의미하는 T_PGFLT에 대해서 이를 처리해줬습니다. 

  
* 실제 할당을 해줄 때에는 유저 프로세스에 대한 물리 페이지를 할당한 후에 이를 프로세스의 페이지 테이블에 mapping시켜줍니다.  

  
   이외 자세한 구현 및 예외 처리는 implementation에 서술하겠습니다.
 
 2) copy on write  
------------------------------------  
* fork할 때 자식 프로세스가 부모 프로세스의 메모리를 그대로 복사하는 것이 아니라 write가 일어나기 전까지 공유하도록 하는 것을 말합니다.  
  
* 물리 페이지에 대해 reference count를 두고 필요할 때 이를 리턴하거나 증가시키거나 감소시키는 함수를 만들었습니다.  

  
* 현재 os의 할당되지 않는 모든 물리페이지의 수를 리턴하는 int get_n_free_pages() 시스템콜을 구현하였습니다.  

  
* copyuvm는 따로 할당을 하는 부분은 지우고 부모 프로세스의 페이지를 공유할 수 있도록 프로세스의 페이지 테이블에 mapping시켜주며 reference count를 1증가시켜야 합니다.    

  
* write을 하려고 하면 page fault가 발생합니다.   
 이를 포함한 문제를 총체적으로 처리하는 부분을 trap.c에 추가하였습니다. (cow_trap 함수)   
   * rcr2()를 호출하여 page fault가 일어나는 address가 아예 virtual address의 범위를 벗어나는지   
   * read-only로 되어있는 경우에는 페이지의 복사본을 만드는 과정을 진행해야 합니다.   
    (이때, reference count가 1인 경우에는 read-only를 제거하면 되고, reference count가 1보다 큰 경우에는 새로 페이지를 할당해주어야 합니다.)  

이 외에 자세한 구현 및 예외 처리는 implementation에 서술하겠습니다.  

3) lazy memory allocation & copy on write  
------------------------------------------------------  
* page fault가 발생했을 때, lazy memory allocation에 의한 것인지, cow에 의한 것인지 알 수 없으므로 이를 구분하여 처리될 수 있도록 해야 합니다.  
  
* if문을 통해 tf->err값이 서로 다른 값을 가진다는 것을 확인하였고 6인 경우에는 lazy memory allocation에 해당하는 처리를,  
  이외의 경우에는 copy on write에 해당하는 처리를 해주었습니다.  

2. Implementation  
=========================
 1) lazy memory allocation  
------------------------------------  

```{.c}
int growproc(int n){
  uint sz;
  struct proc *curproc = myproc();
  sz = curproc->sz;
    if(n > 0){
//    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
//      return -1;
       sz+=n;
     } else if(n < 0){
        if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
        return -1;
    }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}
```

위 함수는 growproc함수입니다.  
growproc 함수에서 allocuvm호출 코드를 지우고 프로세스의 메모리 크기만 증가시켰습니다.  
  
```{.c}
  char* mem;    
  mem=kalloc();
//    cprintf("error: %d\n",tf->err);    
  if(mem==0){
      cprintf("alloc page, out of memory\n");
      kfree((char *)P2V(mem));
      return ;
    }

    memset(mem,0,PGSIZE);
//    cprintf("pagefault\n");
    uint st=PGROUNDDOWN(rcr2());
    mappages(myproc()->pgdir, (char*)st, PGSIZE, V2P(mem), PTE_W|PTE_U);
```
 그리고 pagefault는 유저프로세스의 메모리 크기만 증가시켰기 때문에 그 부분을 사용하려고 실제 접근하려고 할 때 발생한 것입니다.  
 이때에는 kalloc을 통해 유저 프로세스에 대한 물리적 페이지를 할당하고 mappages를 통해  
프로세스의 페이지 테이블에 mapping합니다.  

 2) copy on write  
------------------------  
```{.c}
struct {
  struct spinlock lock;
  int use_lock;
  struct run *freelist;
  int free_pages;
  int ref_count[PHYSTOP>>PGSHIFT];
} kmem;   
```
struct kmem에 아직 할당되지 않은 물리 페이지의 개수를 알 수 있는 free_pages를 추가하고 
ref_coount배열을 통해 물리 페이지에 대한 reference count를 셀 수 있도록 했습니다.  

  
그리고 관련 kinit/ freerange/ kfree/ kalloc에서 이 값을 상황에 맞게 바꿔주는 코드를 추가했습니다. 

```{.c}

void increase_refcount(uint pa){
  if(pa<(uint)V2P(end) || pa >=PHYSTOP)
    panic("increase refcount");
  acquire(&kmem.lock);
  ++kmem.ref_count[pa>>PGSHIFT];
  release(&kmem.lock);
}

void decrease_refcount(uint pa){
  if(pa<(uint)V2P(end)||pa>=PHYSTOP)
    panic("decrease refcount");
    acquire(&kmem.lock);
  --kmem.ref_count[pa>>PGSHIFT];
  release(&kmem.lock);
}

uint ref_count(uint pa){
  if(pa<(uint)V2P(end)||pa>=PHYSTOP)
    panic("error: get_refcount");
  acquire(&kmem.lock);
  uint count=kmem.ref_count[pa>>PGSHIFT];
  release(&kmem.lock);
  return count;
}  

```
 그리고, 물리 페이지에 대한 reference count를 두고 (ref_count배열)  
이를 1만큼 증가시키고/ 1만큼 감소시키고/ 값을 리턴하는 함수를 추가했습니다.  
물론 이때, 동기화가 이루어지도록 lock을 두고 값을 변경하였습니다.  

```{.c}
int get_n_free_pages(void){
  acquire(&kmem.lock);
  int free_p=kmem.free_pages;
  release(&kmem.lock);
  return free_p;
}  
```

과제의 명세대로 get_n_free_pages() 시스템콜을 추가했습니다.  

page fault가 발생하는 경우에 대해서는 따로 함수를 만들어서(cow_trap())  
조건에 따라 이를 처리할 수 있도록 했습니다.  

```{.c}
if(refn>1){
      mem=kalloc();
      if(mem==0){
        cprintf("out of memory\n");
        myproc()->killed=1;
        return;
      } 
      memmove(mem,(char*)P2V(pa),PGSIZE); 
     *pte=V2P(mem)|PTE_P|PTE_U|PTE_W;
      decrease_refcount(pa);
}  
```

write하려고 할 때 page fault가 발생하는 경우를 중심으로 코드를 작성하였으며  
reference count가 1보다 크다는 것은 새로운 복사본을 만들어야 한다는 의미로 kalloc을 통해 새로 할당하고 해당 메모리를 복사하여 독립적으로 접근합니다.  

```{.c}
 else if(refn==1){
   *pte|=PTE_W;
 }
 ```

reference count가 1이라는 것은 read-only로 되어있어 page fault가 발생한 것으로 해당 부분만 바꿔주면 됩니다.  




 3) lazy memory allocation & copy on write  
------------------------------------------------------   

```{.c}
if(tf->trapno==T_PGFLT){
  if(tf->err==6){
    //lazy memory allocation에 해당하는 코드  
  }
  else
    //cow에 해당하는 코드;
  
  return;
}
```  
tf->err의 값을 비교하여 출력해보았으며  
lazy memory allocation의 경우 page fault가 났을 때 이 값이 6이었으므로 이 값을 기준으로  
수행하는 코드의 위치를 재배치 했습니다.  


3. Result
=======================  

1) lazy memory allocation  
![p3_1](/uploads/3126dedfb9b9de31638f6b06cc7c8939/p3_1.JPG)  


![p3_2](/uploads/b0ccd3ac1b2b9bcdcb3b2fa5f03d74d2/p3_2.JPG)  

기본적인 명령어가 제대로 실행되는 것을 볼 수 있습니다.  

![p3_3](/uploads/4c81143bef8c0501337c21430fa2a245/p3_3.JPG)  

sbrk()를 호출하였을 때 인자가 양수/음수 일 때 제대로 실행되는 것을 볼 수 있습니다.  

2) copy on write  


![p3_5](/uploads/eb6262209c702ae2ed4d8a83a8db3584/p3_5.JPG)  
cprintf를 통해 메모리 할당이 제대로 일어나고 있는지 fork는 제대로 이루어지는지 확인하는 과정을 거쳤습니다.  
test_4를 작성하여 프로그램을 실행한 결과, write을 시도하려고 할 경우에는 new allocation이 일어나서 새로 페이지를 할당했음을 알 수 있습니다.  

![p3_6](/uploads/fa0bb01dd455acac3799b3712cee890c/p3_6.JPG)  
fork를 여러 번 했을 때 출력 값이다.  역시 알맞게 할당이 되며 fork한 자식 프로세스가 종료가 되면 할당되었던 물리 페이지들이 다시 늘어나는 것을 알 수 있다.  




4. Trouble shooting
=======================   

### 1) lazy memory allocation을 먼저 구현 완료한 상태에서 copy on write를 구현하는 과정에서 제대로 구현이 되지 않아 lazy memory allocation도 잘 실행이 되지 않는 문제가 발생했습니다. 처음 프로세스가 fork를 호출하는 과정에서 오류가 발생하여 lazy memory allocation을 테스트하는 데에 문제가 발생한다는 것을 알게 되었습니다.   
  
### 2) free pages와 reference count의 개수를 제대로 표시하는 과정에서 빠뜨리는 부분이 발생하여 trap에서 page fault를 처리하는 과정에서 오류가 발생했습니다.  
  
### 3) page fault를 처리하는 과정에 있어서 조건을 나누고 에러를 처리하는 과정이 어려웠습니다.  구글링과 이론 수업/ 실습 수업을 다시 살펴보며 함수의 의미를 파악하면서 해결할 수 있었습니다.  

     