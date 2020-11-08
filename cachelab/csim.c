#include <stdlib.h>
#include <getopt.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include "cachelab.h"

#define false 0
#define true 1

typedef struct {
    CacheLine *head;
    CacheLine *tail;
} CacheSet;

typedef struct {
    CacheLine *prev;
    CacheLine *next;
    unsigned tag;
    unsigned valid;
} CacheLine;

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
    CacheSet *cache = CreateCache(S, E);

    // handle trace file
    int hit, miss, evictions;
    handle_trace(cache, s, b, S, E, t, &hit, &miss, &evictions);

    printSummary(hit, miss, evictions);
    return 0;
}

CacheSet *CreateCache(int S, int E)
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
    }

    return cache;
}

void handle_trace(CacheSet *cache, int s, int b, int S, int E, char *fname, int *hit, int *miss, int *evictions)
{
    char *line = NULL;
    size_t len = 0;
    FILE *fp = fopen(fname, "r");
    if (fp == NULL) {
        fprintf(stderr, "Open given trace file %s failed.\n", fname);
        exit(EXIT_FAILURE);
    }

    char op;
    unsigned long addr;
    unsigned long tag;
    unsigned long set;
    const unsigned long mask = (1 << s) - 1;

    while (getline(line, &len, fp) != -1) {
        if (strlen(line) < 5 || line[0] != ' ') {
            continue;
        }

        // parse trace
        op = line[1];
        addr = strtol(strsep(&line[3], ","), NULL, 16);
        tag = addr >> (s + b);
        set = (addr >> b) & mask;

        printf("%c %x, %x", op, tag, set);

        switch (op) {
        case 'M':
            break;
        case 'L':
            break;
        case 'S':
            break;
        default:
            continue;
        }
    }

    close(fp);
}