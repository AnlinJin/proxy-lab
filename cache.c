#include "cache.h"


int LRU_counter;
unsigned long hash(char const *input);
sem_t mutex, w, lru_w;
int readcnt;

cache_t *init_cache()
{
    cache_t *cache = malloc(sizeof(cache_t));
    cache->cacheline = malloc(sizeof(cacheline_t *) * NUM_CACHELINE);
    for (int i = 0; i < NUM_CACHELINE; i++)
    {
        cache->cacheline[i] = (cacheline_t *)malloc(sizeof(cacheline_t) * NUM_OF_WAY);
        for (int j = 0; j < NUM_OF_WAY; j++)
        {
            cache->cacheline[i][j].LRU_counter = 0;
            cache->cacheline[i][j].valid_bit = 0;
            cache->cacheline[i][j].tag = NULL;
            cache->cacheline[i][j].data_length = 0;
        }
    }
    LRU_counter = 0;
    cache->size = 0;
    readcnt = 0;
    sem_init(&mutex, 0, 1);
    sem_init(&w, 0, 1);
    sem_init(&lru_w, 0, 1);
    return cache;
}
char *read_cache(cache_t *cache, char *hostname, char *uri)
{
    char *buf = malloc(strlen(hostname) + strlen(uri) + 1);
    char *data = NULL;
    sprintf(buf, "%s%s", hostname, uri);
    int index = (int)(hash(buf) % NUM_CACHELINE);

    P(&mutex);
    readcnt++;
    if (readcnt == 1)
        P(&w);
    V(&mutex);
    
    for (int j = 0; j < NUM_OF_WAY; j++)
    {   //input for strcmp can not be NULL
        if ((cache->cacheline[index][j].valid_bit == 1) && cache->cacheline[index][j].tag != NULL &&
        (!strcmp(cache->cacheline[index][j].tag, buf)))
        {
            data = cache->cacheline[index][j].data;
            cache->cacheline[index][j].LRU_counter = LRU_counter;
            P(&lru_w);
            LRU_counter++;
            V(&lru_w);
            break;
        }
    }
    P(&mutex);
    readcnt--;
    if (readcnt == 0)
        V(&w);
    V(&mutex);

    return data;
}

void write_cache(cache_t *cache, char *hostname, char *uri, char *data, long length)
{
    char *buf = malloc(strlen(hostname) + strlen(uri) + 1);
    sprintf(buf, "%s%s", hostname, uri);
    int index = hash(buf) % NUM_CACHELINE;
    char evic = 1;
    P(&w);
    for (int j = 0; j < NUM_OF_WAY; j++)
    {
        if (cache->cacheline[index][j].valid_bit == 0)
        {
            cache->cacheline[index][j].data = data;
            cache->cacheline[index][j].LRU_counter = LRU_counter;
            cache->cacheline[index][j].tag = buf;
            cache->cacheline[index][j].valid_bit = 1;
            cache->cacheline[index][j].data_length = length;
            evic = 0;
            break;
        }
    }
    if (evic == 1)
    {
        int min = cache->cacheline[index][0].LRU_counter;
        int min_index = 0;

        for (int j = 0; j < NUM_OF_WAY; j++)
        {
            if (cache->cacheline[index][j].LRU_counter < min)
            {
                min = cache->cacheline[index][j].LRU_counter;
                min_index = j;
            }
        }
        //delete the length of the evicted cacheline from the cache
        cache->size = cache->size - cache->cacheline[index][min_index].data_length;
        free(cache->cacheline[index][min_index].tag);
        free(cache->cacheline[index][min_index].data);

        cache->cacheline[index][min_index].data = data;
        cache->cacheline[index][min_index].LRU_counter = LRU_counter;
        cache->cacheline[index][min_index].tag = buf;
        cache->cacheline[index][min_index].valid_bit = 1;
        cache->cacheline[index][min_index].data_length = length;
    }
    cache->size += length;
    V(&w);
}

unsigned long hash(char const *input)
{
    unsigned long hash = 5381;
    int c;

    while ((c = *input++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

long getCacheSize(cache_t *cache)
{
    P(&w);    
    long size = cache->size;
    V(&w);
    return size;
    
}