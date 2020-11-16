#define  _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <math.h>
#include <string.h>

#include "cachelab.h"

#define false 0
#define true 1

typedef struct CacheLine {
    struct CacheLine *prev;
    struct CacheLine *next;
    unsigned long tag;
    unsigned valid;
} CacheLine;

typedef struct CacheSet {
    CacheLine *head;
    CacheLine *tail;
} CacheSet;

CacheSet *create_cache(int S, int E);
CacheLine *find_cacheline(CacheSet *cache, unsigned long tag);
void update_cacheline(CacheSet *cache, CacheLine *cl);
void handle_trace(CacheSet *cache, int verbose, int s, int b, int S, int E, char *fname, int *hit, int *miss, int *evictions);

int main(int argc, char *argv[])
{
    extern char *optarg;

    int help = false;
    int verbose = false;
    int s = -1;
    int E = -1;
    int b = -1;
    char *t;

    char arg;

    while ((arg = getopt(argc, argv, "vhs:E:b:t:")) != -1) {
        switch (arg) {
        case 'v':
            verbose = true;
            break;
        case 'h':
            help = true;
            break;
        case 's':
            s = atoi(optarg);
            break;
        case 'E':
            E = atoi(optarg);
            break;
        case 'b':
            b = atoi(optarg);
            break;
        case 't':
            t = optarg;
            break;
        default:
            break;
        }
    }

    // print help message
    if (help) {
        printf("Usage: ./csim [-hv] -s <num> -E <num> -b <num> -t <file>\n"
            "Options:\n"
            "  -h         Print this help message.\n"
            "  -v         Optional verbose flag.\n"
            "  -s <num>   Number of set index bits.\n"
            "  -E <num>   Number of lines per set.\n"
            "  -b <num>   Number of block offset bits.\n"
            "  -t <file>  Trace file.\n\n"
            "Examples:\n"
            "  linux>  ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace\n"
            "  linux>  ./csim-ref -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
        exit(EXIT_SUCCESS);
    }

    // error checking
    if (s <= 0 || E <= 0 || b <= 0 || access(t, R_OK) == -1) {
        fprintf(stderr, "Invalid arguments.\n");
        exit(EXIT_FAILURE);
    }

    // init cache set
    int S = pow(2, s);
    CacheSet *cache = create_cache(S, E);

    // handle trace file
    int hit = 0;
    int miss = 0;
    int eviction = 0;
    handle_trace(cache, verbose, s, b, S, E, t, &hit, &miss, &eviction);

    printSummary(hit, miss, eviction);
    return 0;
}

CacheSet *create_cache(int S, int E)
{
    CacheSet *cache = (CacheSet *)calloc(S, sizeof(CacheSet));

    for (int i = 0; i < S; i++) {
        CacheLine *cachelines = (CacheLine *)calloc(E, sizeof(CacheLine));
        CacheLine *prev = NULL;
        for (int j = 0; j < E - 1; j++) {
            cachelines[j].prev = prev;
            cachelines[j].next = &cachelines[j + 1];
            prev = &cachelines[j];
        }
        cachelines[E - 1].prev = prev;
        cache[i].head = cachelines;
        cache[i].tail = &cachelines[E - 1];
    }

    return cache;
}

void handle_trace(CacheSet *cache, int verbose, int s, int b, int S, int E, char *fname, int *hit, int *miss, int *eviction)
{
    char *line = NULL;
    size_t len = 0;
    FILE *fp = fopen(fname, "r");
    if (fp == NULL) {
        fprintf(stderr, "Open given trace file %s failed.\n", fname);
        exit(EXIT_FAILURE);
    }

    char op;
    char *end;
    unsigned long addr;
    unsigned long tag;
    unsigned long set;
    const unsigned long mask = (1 << s) - 1;
    int mflag, eflag, hflag;
    CacheLine *cl;

    while (getline(&line, &len, fp) != -1) {
        if (strlen(line) < 5 || line[0] != ' ') {
            continue;
        }

        // parse trace
        op = line[1];
        end = &line[3];
        addr = strtol(strsep(&end, ","), NULL, 16);
        tag = addr >> (s + b);
        set = (addr >> b) & mask;

        mflag = 0;
        eflag = 0;
        hflag = 0;

        switch (op) {
        case 'M':
            hflag++;
            // fall through
        case 'L':
            // fall through
        case 'S':
            cl = find_cacheline(&cache[set], tag);
            if (cl == NULL) {
                mflag++;
                cl = cache[set].tail;
                if (cl->valid) {
                    eflag++;
                }
                cl->valid = true;
                cl->tag = tag;
            } else {
                hflag++;
            }
            update_cacheline(&cache[set], cl);
            break;
        default:
            continue;
        }

        *miss += mflag;
        *eviction += eflag;
        *hit += hflag;

        if (verbose) {
            printf("%s%s%s%s\n", &line[1], mflag ? " miss" : "", eflag ? " eviction" : "", hflag ? " hit" : "");
        }
    }

    fclose(fp);
}

CacheLine *find_cacheline(CacheSet *cache, unsigned long tag)
{
    CacheLine *p = cache->head;
    while (p != NULL && p->valid == true) {
        if (p->tag == tag) {
            return p;
        } else {
            p = p->next;
        }
    }
    return NULL;
}

void update_cacheline(CacheSet *cache, CacheLine *cl)
{
    if (cache->head == cl) {
        return;
    }

    if (cache->tail == cl) {
        cache->tail = cl->prev;
    } else {
        cl->next->prev = cl->prev;
    }
    cl->prev->next = cl->next;
    cl->prev = NULL;
    cl->next = cache->head;
    cache->head->prev = cl;
    cache->head = cl;
}
