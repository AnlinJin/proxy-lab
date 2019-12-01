#include "csapp.h"

#define NUM_CACHELINE 32
#define NUM_OF_WAY    4
typedef struct {
    char *  tag;
    char * data;
    double data_length;
    int LRU_counter;
    char valid_bit;
}cacheline_t;

// 1-way cache
typedef struct {
    cacheline_t ** cacheline;
    long size;
}cache_t;

cache_t * init_cache();

//return the buf that contains the data if it is in the cache
//return null if the uri and server is not in the cache
char * read_cache(cache_t *cache,char *hostname, char *uri);

void write_cache(cache_t *cache,char *hostname, char *uri, char *data,long length);

long getCacheSize(cache_t *cache);