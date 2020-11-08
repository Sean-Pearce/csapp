#include <stdlib.h>
#include <getopt.h>
#include <stdio.h>

#include "cachelab.h"

#define false 0
#define true 1

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
    if (s < 0 || E < 0 || b < 0) {
        fprintf(stderr, "Invalid arguments.\n");
        exit(EXIT_FAILURE);
    }

    if (verbose) {
        printf("s = %d\nE = %d\nb = %d\nt = %s\n", s, E, b, t);
    }


    printSummary(0, 0, 0);
    return 0;
}
