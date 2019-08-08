/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/

#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1<<12)
#define MAX(x, y) ((x)>(y)? x:y)
#define GET(p)       (*(size_t *)(p))
#define PUT(p, val)  (*(size_t *)(p)  = (val))
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define HDRP(p)      ((char *)(p) - WSIZE)
#define FTRP(p)      ((char *)(p) + GET_SIZE(HDRP(p)) - DSIZE)
#define NEXT_BLKP(p) ((char *)(p) + GET_SIZE((char *)(p) - WSIZE))
#define PREV_BLKP(p) ((char *)(p) - GET_SIZE((char *)(p) - DSIZE))
#define PACK(size, alloc) ((size)|(alloc))
#define GET_PREV_ALLOC(p) (GET(HDRP(p))& 0x2)
#define SET_ALLOC(size, p)         ((GET_PREV_ALLOC(p))? PACK(size, 0x3):PACK(size, 0x1))   //用于当前块已被申请时，设置当前申请块的块大小， 前块信息
#define SET_UNALLOC(size, p)       ((GET_PREV_ALLOC(p))? PACK(size, 0x2):PACK(size, 0x0))   //用于当前块未被申请时，设置当前申请块的块大小， 前块信息
#define GET_PREV(p)      (GET(p))	   //获得块的前驱地址
#define PUT_PREV(p, val) (PUT(p, (size_t)val))  //写入块的前驱地址
#define GET_SUCC(p)      (*((size_t *)p + 1))        //获得快的后继
#define PUT_SUCC(p, val) (*((size_t *)p + 1) = (size_t)(val)) //写入块的后继
/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8  //双字对其
/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)  //取最小的size
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))    //最靠近size_t的大小 sizeof(size_t) = 4

static size_t * list_root = NULL;
static void * heap_listp = NULL;
static void *extend_heap(size_t size);
static void *coalesce(void * p);
static void *find_fit(size_t newsize);
static void place(void *p, size_t newsize);
void link2root(void* p);
void linkjump(void *p);
static void *realloc_coalesce(void *p, size_t newsize, int *next_free_enough);
static void realloc_place(void *p, size_t newsize);

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if((heap_listp = (char *)mem_sbrk(6*WSIZE)) == (void *)-1)//静态指针向堆申请6*4的内存，分别是填充字，链表头，序言快和结尾块
        return -1;
    PUT(heap_listp, 0);     //填充字置零
    PUT(heap_listp + 3*WSIZE, PACK(DSIZE, 1));   //序言块
    PUT(heap_listp + 4*WSIZE, PACK(DSIZE, 1));
    PUT(heap_listp + 5*WSIZE, PACK(0, 0x3));   //结尾块前块是序言快，所以不是空闲
    heap_listp += 4*WSIZE;     //指向序言块
    list_root = (size_t *)(heap_listp - 3*WSIZE);  //指向链表头
    PUT_PREV(list_root, 0);     //前驱后继置为null
    PUT_SUCC(list_root, 0);     //后继设置为null
    if(extend_heap(CHUNKSIZE) == NULL)     //申请堆
        return -1;
    return 0;
}

static void *extend_heap(size_t size){
    char *p;                             //p是上一次堆的顶部，即结尾块的
    size_t newsize;                      //对齐后的大小
    newsize = ALIGN(size);
    if((size_t)(p = mem_sbrk(newsize))<0)     //p的旧地址已经到顶无法分配
        return NULL;
    PUT(HDRP(p), SET_UNALLOC(newsize, p));   //p上一块就是结尾块，结尾块中有前面是否空闲的信息
    PUT(FTRP(p), PACK(newsize, 0));          //空闲块有尾部，不用储存前块信息，所以置零
    PUT(HDRP(NEXT_BLKP(p)), PACK(0, 0x1));   //结尾块，前块是空闲块
    return coalesce(p);                      //合并该块，并连在链表头后面
}

void link2root(void* p)   //将p连到root后
{
    size_t succ = GET_SUCC(list_root);   //获得list_root的后继
    PUT_PREV(p, list_root);                //将list_root的地址值放入p中
    PUT_SUCC(list_root, p);              //将p的地址值放入listroot中
    PUT_SUCC(p, succ);                  //将后继的地址放在p中
    if(succ) PUT_PREV(succ, p);         //如果后继不是NULL，将p放在后继中
}

void linkjump(void *p)    //将p块从显式链表中删除
{
    size_t succ, prev;
    succ = GET_SUCC(p);                //p的后继
    prev = GET_PREV(p);                //p的前驱
    PUT_SUCC(prev, succ);              //将后继地址放在前驱中
    if(succ) PUT_PREV(succ, prev);      //如果后继不是NULL，将前驱地址放在后继中
}

static void *coalesce(void * p){
    size_t prev_alloc = GET_PREV_ALLOC(p);            //前面块是否被分配 1分配0空闲
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(p)));   //后面块是否被分配 1分配0空闲
    size_t size = GET_SIZE(HDRP(p));                  //p块的大小
    if(prev_alloc && next_alloc){      //前后都被分配
        link2root(p);                  //将p块连在头后面
        return p;
    }
    else if(prev_alloc && !next_alloc){    //前分后闲
        size += GET_SIZE(HDRP(NEXT_BLKP(p))); //获取下一块的大小，加在一起作为合并块的大小
	linkjump(NEXT_BLKP(p));                //将后块分离
        PUT(HDRP(p), SET_UNALLOC(size, p));   //本块空闲，大小size
        PUT(FTRP(p), PACK(size, 0));   
        link2root(p);                          //合并后放在链表头后
    }
    else if(!prev_alloc && next_alloc){      //前闲后分
        size += GET_SIZE(HDRP(PREV_BLKP(p)));     //获取前块的大小，加在一起
	linkjump(PREV_BLKP(p));             //先断开，合并后放到前面
        PUT(FTRP(p), PACK(size, 0));               //本块空闲，大小size
        PUT(HDRP(PREV_BLKP(p)), SET_UNALLOC(size, PREV_BLKP(p)));  //前面块的信息更新，大小size未分配        
        p = PREV_BLKP(p);                   //合并
        link2root(p);                       //放入链表
    }
    else{                                      //前后都闲
        size += GET_SIZE(HDRP(PREV_BLKP(p))) + GET_SIZE(FTRP(NEXT_BLKP(p)));  //算总的大小
	linkjump(PREV_BLKP(p));                                                //前后析离
        linkjump(NEXT_BLKP(p));
        PUT(HDRP(PREV_BLKP(p)), SET_UNALLOC(size, PREV_BLKP(p)));              //更新前面后面的信息
        PUT(FTRP(NEXT_BLKP(p)), PACK(size, 0));       
        p = PREV_BLKP(p);                                                       //合并
        link2root(p);                 //放入链表， 由于后面本来就是空闲块，所以后面的状态信息没变
    }
    return p;                          //返回合并块指针
}
/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    char *p;
    size_t extendsize;
    if(size <= 0)
        return NULL;
    size_t newsize = ALIGN(size + WSIZE);  //预留头的位置，也保证双字对齐
	//printf("the newsize is %d\n", newsize);
    if((p = find_fit(newsize)) != NULL){   //找到了足够的空闲块
	//printf("find the pos is %zu\n", (size_t)(HDRP(p)));
        place(p, newsize);                 //把p后面的newsize置为分配状态
        return p;                          //返回指针p
    }
    
    //printf("no found fit, I will extend the heap\n");
    /*no fit found*/
    extendsize = MAX(newsize, CHUNKSIZE);   //扩展堆
    if((p = extend_heap(extendsize)) == NULL){   //无法分配时，返回NULL
        return NULL;
    }
    place(p, newsize);                       //把p后面的newsize大小置为分配
    //printf("now the succ of list_root is %zu\n", *((size_t *)list_root + 1));
    return p;
}

static void *find_fit(size_t newsize){        //找大小合适的空闲块
   /* size_t t = GET_SUCC(list_root);           //t是listroot的后继地址值
    while(t != 0){                            //如果t不是NULL，则尝试该地址
        if(newsize <= GET_SIZE(HDRP(t)))      //注意newsize已经是加上头部和双字对齐后的，所以只和size比较
            return (void *)t;                 //返回void型的指针
        else
            t = GET_SUCC(t);                  //t变成t块的后继
    }
    return NULL;                               //找不到返回NULL*/
	size_t t = GET_SUCC(list_root); 
	int count = 150;
	size_t minsize = 0xffffffff;
	void * minp = NULL;
	while(t != 0 && count != 0){
		count --;
		if(newsize <= GET_SIZE(HDRP(t))){
			if(GET_SIZE(HDRP(t))<minsize){
				minsize = GET_SIZE(HDRP(t));
				minp = (void *)t;
			}
		}
		t = GET_SUCC(t);
	}
	return minp;
	/*if(count != 0){
		return minp;
	}
	else{
		while(t != 0){                            //如果t不是NULL，则尝试该地址
        	if(newsize <= GET_SIZE(HDRP(t)))      //注意newsize已经是加上头部和双字对齐后的，所以只和size比较
            	return (void *)t;                 //返回void型的指针
        	else
            	t = GET_SUCC(t);                  //t变成t块的后继
    	}
	}
	return NULL;*/
}

static void place(void *p, size_t newsize){
    size_t total_size;
    total_size = GET_SIZE(HDRP(p));                        //总块大小
    linkjump(p);                                            //将P从链表中剔除
    if(total_size-newsize < 4*WSIZE){                      //<=16， 保证双字对齐，分不出来了
	//printf("no need to cut, only %d left\n", total_size-newsize);
        PUT(HDRP(p), PACK(GET(HDRP(p)), 0x1));              //更新本块信息，本块已经不空闲了
        PUT(HDRP(NEXT_BLKP(p)), PACK(GET(HDRP(NEXT_BLKP(p))), 0x2)); //告诉下一块，上一块已经不空闲了
    }
    else{                                             //可以分割
        PUT(HDRP(p), SET_ALLOC(newsize, p));          //将信息写入当前块头部，指明其大小，是否被申请，保留上一块信息
        void *pnew = NEXT_BLKP(p);                    //分割得到的空闲块的块指针
        PUT(HDRP(pnew), PACK((total_size-newsize), 0x2));   //将信息写入得到的空闲块头部，尾部，头部指明前块已分配，本快未分配
        PUT(FTRP(pnew), PACK((total_size-newsize), 0x0));
	//printf("cut, %d left  before coalese\n", total_size-newsize);
        coalesce(pnew);
    }
}
/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));         //获取本快大小
    PUT(HDRP(ptr), SET_UNALLOC(size, ptr));    //将该块的是否空闲的信息更新
    PUT(FTRP(ptr), PACK(size, 0));             //由于该块空闲所以更新尾部信息
    coalesce(ptr);                             //合并空闲块两边的块，并维护链表
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */


static void *realloc_coalesce(void *p, size_t newsize, int *next_free_enough)   //p又可能是分配快
{
    size_t prev_alloc = GET_PREV_ALLOC(p);
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(p)));
    size_t size = GET_SIZE(HDRP(p));
    *next_free_enough = 0;

    if (prev_alloc && !next_alloc) {      /* Case 2 */  //上分下闲
        size += GET_SIZE(HDRP(NEXT_BLKP(p)));           //和下面的大小加在一起获取总的大小
        if(size >= newsize)                             //如果newsize小就合并
        {
		linkjump(NEXT_BLKP(p));                     //将该块从链表中分离出来
            PUT(HDRP(p), SET_UNALLOC(size, p));         //设置头部信息
            PUT(FTRP(p), PACK(size, 0));                //设置尾部信息
            *next_free_enough = 1;                      //标志位，块指针不用动
        }
    }

    else if (!prev_alloc && next_alloc) {      /* Case 3 */
        size += GET_SIZE(HDRP(PREV_BLKP(p)));
        if(size >= newsize)
        {
	 linkjump(PREV_BLKP(p));
            PUT(FTRP(p), PACK(size, 0));
            PUT(HDRP(PREV_BLKP(p)), SET_UNALLOC(size, PREV_BLKP(p)));
            p = PREV_BLKP(p);
        }
    }

    else {                                     /* Case 4 */
        size += GET_SIZE(HDRP(PREV_BLKP(p))) + GET_SIZE(FTRP(NEXT_BLKP(p)));
        if(size >= newsize)
        {
	linkjump(PREV_BLKP(p));
            linkjump(NEXT_BLKP(p));
            PUT(HDRP(PREV_BLKP(p)), SET_UNALLOC(size, PREV_BLKP(p)));
            PUT(FTRP(NEXT_BLKP(p)), PACK(size, 0));
            p = PREV_BLKP(p);
        }
    }
    return p;
}

static void realloc_place(void *p, size_t newsize){
    size_t total_size;
    total_size = GET_SIZE(HDRP(p));
    if(total_size- newsize < 4*WSIZE){
        PUT(HDRP(p), PACK(GET(HDRP(p)), 0x1));                           //不分割，本快已分配
        PUT(HDRP(NEXT_BLKP(p)), PACK(GET(HDRP(NEXT_BLKP(p))), 0x2));     //下一块的前块信息更新为已分配
    }
    else{
        PUT(HDRP(p), SET_ALLOC(newsize, p));
        void *pnew = NEXT_BLKP(p);
        PUT(HDRP(pnew), PACK((total_size-newsize), 0x2));   //将信息写入得到的空闲块头部，尾部，头部指明前块已分配
        PUT(FTRP(pnew), PACK((total_size-newsize), 0x0));
        coalesce(pnew);                                      //因为realloc_coalesce已将p块剔除，这里将分割出来的块放到链表头
    }
}


