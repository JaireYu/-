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
#define GET_PREV_ALLOC(p) (GET_ALLOC(HDRP(p))& 0x2)
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
static void realloc_place(void *p, size_t newsize)

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if((heap_listp = (char *)mem_sbrk(6*WSIZE)) == (void *)-1)
        return -1;
    PUT(heap_listp, 0);
    PUT(heap_listp + 3*WSIZE, PACK(DWISE, 1));
    PUT(heap_listp + 4*WSIZE, PACK(DWISE, 1));
    PUT(heap_listp + 5*WSIZE, PACK(0, 0x3));   //结尾块前块是序言快，所以不是空闲
    heap_listp += 4*WSIZE;
    list_root = (size_t *)(heap_listp - 3*WSIZE);
    PUT_PREV(list_root, 0);     //前驱后继置为null
    PUT_SUCC(list_root, 0);
    if(extend_heap(CHUNKSIZE) == NULL)
        return -1;
    return 0;
}

static void *extend_heap(size_t size){
    char *p;
    size_t newsize;
    newsize = ALIGN(size);
    if((int)(p = mem_sbrk(newsize))<0)
        return NULL;
    PUT(HDRP(p), SET_UNALLOC(newsize, p));
    PUT(FTRP(p), PACK(newsize, 0));
    PUT(HDRP(NEXT_BLKP(p)), PACK(0, 0x1));   //结尾块，前块是空闲块
    return coalesce(p);
}
void link2root(void* p)   //将p连到root后
{
    size_t succ = GET_SUCC(list_root);
    PUT_PREV(p, list_root);
    PUT_SUCC(list_root, p);
    PUT_SUCC(p, succ);
    if(succ) PUT_PREV(succ, p);
}

void linkjump(void *p)    //将p块从显式链表中删除
{
    size_t succ, prev;
    succ = GET_SUCC(p);
    prev = GET_PREV(p);
    PUT_SUCC(prev, succ);
    if(succ) PUT_PREV(succ, prev);
}

static void *coalesce(void * p){
    size_t prev_alloc = GET_PREV_ALLOC(p);
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(p)));
    size_t size = GET_SIZE(HDRP(bp));
    if(pre_alloc && next_alloc){      //前后都被分配
        link2root(p);
        return p;
    }
    else if(prev_alloc && !next_alloc){    //前分后闲
        size += GET_SIZE(HDRP(NEXT_BLKP(p)));
        PUT(HDRP(p), SET_UNALLOC(size, p));
        PUT(FTRP(p), PACK(size, 0));
        linkjump(NEXT_BLKP(p));
        link2root(p);
    }
    else if(!prev_alloc && next_alloc){      //前闲后分
        size += GET_SIZE(HDRP(PREV_BLKP(p)));
        PUT(FTRP(p), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(p)), SET_UNALLOC(size, PREV_BLKP(p)));
        linkjump(PREV_BLKP(p));             //先断开，合并后放到前面
        p = PREV_BLKP(p);
        link2root(p);
    }
    else{                                      //前后都闲
        size += GET_SIZE(HDRP(PREV_BLKP(p))) + GET_SIZE(FTRP(NEXT_BLKP(p)));
        PUT(HDRP(PREV_BLKP(p)), SET_UNALLOC(size, PREV_BLKP(p)));
        PUT(FTRP(NEXT_BLKP(p)), PACK(size, 0));
        linkjump(PREV_BLKP(p));
        linkjump(NEXT_BLKP(p));
        p = PREV_BLKP(p);
        link2root(p);
    }
    return p;
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
    int newsize = ALIGN(size + WSIZE);  //预留头的位置，也保证双字对其
    if((p = find_fit(newsize)) != NULL){
        place(p, newsize);
        return p;
    }
    /*no fit found*/
    extendsize = MAX(newsize, CHUNKSIZE);
    if((p = extend_heap(newsize)) == NULL){
        return NULL;
    }
    place(p, newsize);
    return p;
}

static void *find_fit(size_t newsize){
    size_t t = GET_SUCC(list_root);
    while(t != 0){
        if(newsize <= GET_SIZE(HDRP(p)))
            return (void *)t;
        else
            t = GET_SUCC(t);
    }
    return NULL;
}

static void place(void *p, size_t newsize){
    size_t prev;
    size_t succ;
    size_t total_size;
    total_size = GET_SIZE(HDRP(p));
    if(total_size-newsize < 4*WSIZE){                      //<=16， 保证双字对齐
        PUT(HDRP(p), PACK(GET(HDRP(p)), 0x1));
        PUT(HDRP(NEXT_BLKP(p)), PACK(GET(HDRP(NEXT_BLKP(p))), 0x2)); //告诉下一块，上一块已经不空闲了
        linkjump(p);
    }
    else{
        PUT(HDRP(p), SET_ALLOC(newsize, p));          //将信息写入当前块头部，指明其大小，是否被申请，保留上一块信息
        void *pnew = NEXT_BLKP(p);                    //分割得到的空闲块的块指针
        link2root(pnew);
        PUT(HDRP(pnew), PACK((total_size-newsize), 0x2));   //将信息写入得到的空闲块头部，尾部，头部指明前块已分配
        PUT(FTRP(pnew), PACK((total_size-newsize), 0x0));
    }
}
/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *p)
{
    size_t size = GET_SIZE(HDRP(p));
    PUT(HDRP(p), SET_UNALLOC(size, p));    //将该块的是否空闲的信息更新
    PUT(FTRP(p), PACK(size, 0));
    coalesce(p);
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
        size += GET_SIZE(HDRP(NEXT_BLKP(p)));
        if(size >= newsize)
        {
            PUT(HDRP(p), SET_UNALLOC(size, p));
            PUT(FTRP(p), PACK(size, 0));
            linkjump(NEXT_BLKP(p));
            *next_free_enough = 1;
        }
    }

    else if (!prev_alloc && next_alloc) {      /* Case 3 */
        size += GET_SIZE(HDRP(PREV_BLKP(p)));
        if(size >= newsize)
        {
            PUT(FTRP(p), PACK(size, 0));
            PUT(HDRP(PREV_BLKP(p)), SET_UNALLOC(size, PREV_BLKP(p)));
            linkjump(PREV_BLKP(p));
            p = PREV_BLKP(p);
        }
    }

    else {                                     /* Case 4 */
        size += GET_SIZE(HDRP(PREV_BLKP(p))) + GET_SIZE(FTRP(NEXT_BLKP(p)));
        if(size >= newsize)
        {
            PUT(HDRP(PREV_BLKP(p)), SET_UNALLOC(size, PREV_BLKP(p)));
            PUT(FTRP(NEXT_BLKP(p)), PACK(size, 0));
            linkjump(PREV_BLKP(p));
            linkjump(NEXT_BLKP(p));
            p = PREV_BLKP(p);
        }
    }
    return p;
}

static void realloc_place(void *p, size_t newsize){
    size_t total_size;
    total_size = GET_SIZE(HDRP(p));
    if(total_size- newsize < 4*WSIZE){
        PUT(HDRP(p), PACK(GET(HDRP(p)), 0x1));
        PUT(HDRP(NEXT_BLKP(p)), PACK(GET(HDRP(NEXT_BLKP(p))), 0x2));
    }
    else{
        PUT(HDRP(p), SET_ALLOC(newsize, p));
        void *pnew = NEXT_BLKP(p);
        PUT(HDRP(pnew), PACK((total_size-newsize), 0x2));   //将信息写入得到的空闲块头部，尾部，头部指明前块已分配
        PUT(FTRP(pnew), PACK((total_size-newsize), 0x0));
        coalesce(pnew);                                      //因为realloc的两个函数与链表无关，因此不需要维护，但要尽量合并
    }
}

void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t newsize = ALIGN(size + WSIZE);
    size_t oldsize = GET_SIZE(ptr);
    int free_enough;

    
    /*newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;*/
    if(size == 0){
        mm_free(oldptr);
        return NULL;
    }
    if(ptr == NULL){
        return mm_malloc(size);
    }
    if(newsize == oldsize){
        return ptr;
    }
    else if(newsize < oldsize){
        realloc_place(ptr, newsize);
        return ptr;
    }
    else{
        newptr = realloc_coalesce(ptr, newsize, free_enough);
        size_t copySize = GET_SIZE(HDRP(ptr));
        if(free_enough == 1){   //case 2
            realloc_place(ptr, newsize);
            return ptr;
        }
        else if(newptr != ptr){   //case 3 4
            memcpy(newptr, oldptr, copySize);
            realloc_place(newptr, newsize);
            return newptr;
        }
        else{
            newptr = mm_malloc(newsize);
            memcpy(newptr, oldptr, copySize);
            mm_free(oldptr);
            return newptr;
        }
    }
}














