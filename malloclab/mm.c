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
team_t team = {
        /* Team name */
        "ateam",
        /* First member's full name */
        "Harry Bovik",
        /* First member's email address */
        "bovik@cs.cmu.edu",
        /* Second member's full name (leave blank if none) */
        "",
        /* Second member's email address (leave blank if none) */
        ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define WSIZE           4
#define DSIZE           8
#define CHUNKSIZE       1<<12
#define MINBLOCKSIZE    1<<5

#define CLASSNUM    9

#define MAX(x, y)           ((x) > (y) ? (x) : (y))
#define MIN(x, y)           ((x) < (y) ? (x) : (y))
#define GET(p)              (*(size_t *)(p))
#define PUT(p, val)         (*(size_t *)(p) = (val))
#define PACK(size, alloc)   ((size) | (alloc))

#define GET_SIZE(p)     (GET(p) & ~0x7)
#define GET_ALLOC(p)    (GET(p) & 0x1)

#define HDRP(bp)    ((char *)(bp) - DSIZE)
#define FTRP(p)     ((char *)(p) + GET_SIZE(p) - DSIZE)

#define NEXT(p)     (*(char **)((char *)(p) + DSIZE))
#define PREV(p)     (*(char **)((char *)(p) + DSIZE + WSIZE))

#define SET_NEXT(p, np) (NEXT(p) = (np))
#define SET_PREV(p, pp) (PREV(p) = (pp))

#define NEXT_BLKP(p)   ((char *)(p) + GET_SIZE(p))
#define PREV_BLKP(p)   ((char *)(p) - GET_SIZE((char *)(p) - DSIZE))

static void *extend_heap(size_t size);
static void *coalesce(void *p);
static void *find_fit(size_t size);
static void *place(void *p, size_t size, size_t free);
static void delete(void *p);
static void insert(void *p);

static char *heap_start;

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    // create dummy blocks
    if ((heap_start = mem_sbrk(4 * DSIZE * CLASSNUM + 2 * DSIZE)) == (void *) -1) {
        return -1;
    }

    char *p = heap_start;
    for (int i = 0; i < CLASSNUM; ++i) {
        PUT(p, PACK(4 * DSIZE, 1));
        PUT(FTRP(p), PACK(4 * DSIZE, 1));
        SET_NEXT(p, p);
        SET_PREV(p, p);
        p = NEXT_BLKP(p);
    }

    PUT(p, PACK(DSIZE, 1));
    PUT(NEXT_BLKP(p), PACK(DSIZE, 1));

    if (extend_heap(CHUNKSIZE) == NULL) {
        return -1;
    }
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;
    char *p;

    // Ignore spurious requests
    if (size == 0)
        return NULL;

    // Adjust block size to include overhead and alignment requirements
    asize = ALIGN(size + 2 * DSIZE);

    // Search the free list for a fit
    if ((p = find_fit(asize)) != NULL) {
        p = place(p, asize, 1);
        return p + DSIZE;
    }

    // No fit found. Get more memory and place the block
    if ((p = extend_heap(MAX(asize, CHUNKSIZE))) == NULL)
        return NULL;
    p = place(p, asize, 1);
    return p + DSIZE;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(HDRP(bp)), PACK(size, 0));
    coalesce(HDRP(bp));
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *bp, size_t size)
{
    void *p = HDRP(bp);
    void *newptr = bp;
    size_t osize;
    size_t asize;

    // bp is NULL
    if (bp == NULL)
        return mm_malloc(size);

    // size is zero
    if (size == 0) {
        mm_free(bp);
        return NULL;
    }

    osize = GET_SIZE(p);
    asize = ALIGN(size + 2 * DSIZE);
    if (osize >= asize) {
        // try to shrink
        place(p, asize, 0);
    } else {
        // try to extend
        if (!GET_ALLOC(NEXT_BLKP(p)) && GET_SIZE(NEXT_BLKP(p)) >= asize - osize) {
            delete(NEXT_BLKP(p));
            osize += GET_SIZE(NEXT_BLKP(p));
            PUT(p, PACK(osize, 1));
            PUT(FTRP(p), PACK(osize, 1));
            place(p, asize, 0);
        } else {
            newptr = mm_malloc(size);
            if (newptr == NULL)
                return NULL;
            memcpy(newptr, bp, osize - 2 * DSIZE);
            mm_free(bp);
        }
    }

    return newptr;
}

static void delete(void *p)
{
    SET_NEXT(PREV(p), NEXT(p));
    SET_PREV(NEXT(p), PREV(p));
}

static void insert(void *p)
{
    size_t size = GET_SIZE(p);
    int idx = 0;
    while (size > (MINBLOCKSIZE << idx)) {
        idx++;
    }
    idx = MIN(idx, CLASSNUM - 1);

    void *head = heap_start + 4 * DSIZE * idx;
    SET_PREV(NEXT(head), p);
    SET_NEXT(p, NEXT(head));
    SET_PREV(p, head);
    SET_NEXT(head, p);
}

// 扩展堆的大小
// 输入 - 需要增加的字节数
// 输出 - 扩展得到的空闲内存块的地址
static void *extend_heap(size_t size)
{
    void *p;
    if ((p = mem_sbrk(ALIGN(size))) == (void *) -1) {
        return NULL;
    }

    p = HDRP(p);
    PUT(p, PACK(size, 0));
    PUT(FTRP(p), PACK(size, 0));
    PUT(NEXT_BLKP(p), PACK(DSIZE, 1));

    // 可能需要与前一个块合并
    return coalesce(p);
}

// 如果可能，合并空闲块
// 输入 - 空闲块的地址，发起调用前该空闲块必须已经从空闲链表中移除
// 输出 - 合并后的空闲块的地址，且此空闲块已经插入到对应的空闲链表中
static void *coalesce(void *p)
{
    size_t prev_alloc = GET_ALLOC(PREV_BLKP(p));
    size_t next_alloc = GET_ALLOC(NEXT_BLKP(p));
    size_t size = GET_SIZE(p);

    if (!prev_alloc)
        delete(PREV_BLKP(p));
    if (!next_alloc)
        delete(NEXT_BLKP(p));

    if (prev_alloc && next_alloc) {
        // nop
    } else if (prev_alloc && !next_alloc) {
        size += GET_SIZE(NEXT_BLKP(p));
        PUT(p, PACK(size, 0));
        PUT(FTRP(p), PACK(size, 0));
    } else if (!prev_alloc && next_alloc) {
        size += GET_SIZE(PREV_BLKP(p));
        PUT(PREV_BLKP(p), PACK(size, 0));
        PUT(FTRP(p), PACK(size, 0));
        p = PREV_BLKP(p);
    } else {
        size += GET_SIZE(PREV_BLKP(p)) +
                GET_SIZE(FTRP(NEXT_BLKP(p)));
        PUT(PREV_BLKP(p), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(p)), PACK(size, 0));
        p = PREV_BLKP(p);
    }

    insert(p);
    return p;
}

// 寻找满足所需内存大小的空闲块，不会把空闲块从链表中移除
// 输入 - 所需内存大小
// 输出 - 空闲内存块的地址，如果没有找到则返回 NULL
static void *find_fit(size_t size)
{
    int idx = 0;
    while (size > (MINBLOCKSIZE << idx)) {
        idx++;
    }
    idx = MIN(idx, CLASSNUM - 1);

    char *head;
    char *p;
    while (idx < CLASSNUM) {
        head = heap_start + 4 * DSIZE * idx;
        p = NEXT(head);
        while (p != head) {
            if (!GET_ALLOC(p) && GET_SIZE(p) >= size) {
                return p;
            }
            p = NEXT(p);
        }
        idx++;
    }

    return NULL;
}

// 在空闲块中放置分配内存，如果剩余的空闲内存数目大于最小空闲块，则将其分离并插入到合适的空闲链表中
// 输入 - 空闲块地址
//       需要分配的内存大小
static void *place(void *p, size_t size, size_t free)
{
    size_t osize = GET_SIZE(p);

    if (free) {
        delete(p);
    }

    if (osize <= size + 4 * DSIZE) {
        // 剩余空间小于最小空闲块
        PUT(p, PACK(osize, 1));
        PUT(FTRP(p), PACK(osize, 1));
    } else {
        // 分割该空闲块，将较大的块放在空闲块的后部
        if (size > 128 && free) {
            PUT(p, PACK(osize - size, 0));
            PUT(FTRP(p), PACK(osize - size, 0));
            PUT(NEXT_BLKP(p), PACK(size, 1));
            PUT(FTRP(NEXT_BLKP(p)), PACK(size, 1));
            insert(p);
            p = NEXT_BLKP(p);
        } else {
            PUT(p, PACK(size, 1));
            PUT(FTRP(p), PACK(size, 1));
            PUT(NEXT_BLKP(p), PACK(osize - size, 0));
            PUT(FTRP(NEXT_BLKP(p)), PACK(osize - size, 0));
            insert(NEXT_BLKP(p));
        }
    }

    return p;
}