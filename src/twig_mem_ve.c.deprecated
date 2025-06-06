#include "twig.h"
#include "twig_priv.h"

struct ve_mem {
    twig_mem_t pub_mem;
    struct ve_mem *next;
};

static struct ve_mem first_chunk = {0};

static twig_mem_t *twig_allocator_ve_mem_alloc(twig_allocator_t *allocator, size_t size) {
    if (!allocator || size <= 0)
        return NULL;

    if (pthread_mutex_lock(&allocator->lock))
        return NULL;

    void *addr = NULL;
    size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    twig_mem_t *ret = NULL;

    struct ve_mem *chunk, *best_chunk = NULL;
    for (chunk = &first_chunk; chunk != NULL; chunk = chunk->next) {
        if (chunk->pub_mem.virt == NULL && chunk->pub_mem.size >= size) {
            if (best_chunk == NULL || chunk->pub_mem.size < best_chunk->pub_mem.size)
                best_chunk = chunk;

            if (chunk->pub_mem.size == size)
                break;
        }
    }

    if (!best_chunk)
        goto finish;

    int size_left = best_chunk->pub_mem.size - size;
    addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, allocator->dev_fd, best_chunk->pub_mem.phys + PAGE_OFFSET);
    if (addr == MAP_FAILED)
        goto finish;

    best_chunk->pub_mem.virt = addr;
    best_chunk->pub_mem.size = size;
    best_chunk->pub_mem.type = TWIG_MEM_VE;
    ret = &best_chunk->pub_mem;
    
    if (size_left > 0) {
        chunk = malloc(sizeof(struct ve_mem));
        if (chunk) {
            chunk->pub_mem.phys = best_chunk->pub_mem.phys + size;
            chunk->pub_mem.size = size_left;
            chunk->pub_mem.virt = NULL;
            chunk->pub_mem.type = TWIG_MEM_VE;
            chunk->next = best_chunk->next;
            best_chunk->next = chunk;
        }
    }

finish:
    pthread_mutex_unlock(&allocator->lock);
    return ret;
}

static void twig_allocator_ve_mem_free(twig_allocator_t *allocator, twig_mem_t *pub_mem) {
    if (!allocator || !pub_mem)
        return;

    pthread_mutex_lock(&allocator->lock);

    struct ve_mem *chunk;
    for (chunk = &first_chunk; chunk != NULL; chunk = chunk->next) {
        if (chunk->pub_mem.virt == pub_mem->virt) {
            munmap(pub_mem->virt, chunk->pub_mem.size);
            chunk->pub_mem.virt = NULL;
            break;
        }
    }

    for (chunk = &first_chunk; chunk != NULL; chunk = chunk->next) {
        if (chunk->pub_mem.virt == NULL) {
            while(chunk->next != NULL && chunk->next->pub_mem.virt == NULL) {
                struct ve_mem *next = chunk->next;
                chunk->pub_mem.size += next->pub_mem.size;
                chunk->next = next->next;
                free(next); 
            }
        }
    }

    pthread_mutex_unlock(&allocator->lock);
}

static void twig_allocator_ve_mem_flush(twig_allocator_t *allocator, twig_mem_t *pub_mem) {
    if (!allocator || !pub_mem)
        return;

    struct cedarv_cache_range range = {
        .start = (int)pub_mem->virt,
        .end = (int)(pub_mem->virt + pub_mem->size)
    };
    ioctl(allocator->dev_fd, IOCTL_FLUSH_CACHE, (void*)(&range));
}

static void twig_allocator_ve_destroy(twig_allocator_t *allocator) {
    if (!allocator)
        return;

    pthread_mutex_destroy(&allocator->lock);
    free(allocator);
}

twig_allocator_t *twig_allocator_ve_create(int ve_fd, const struct cedarv_env_infomation *ve_info) {
	if (ve_info->phymem_total_size <= 0)
		return NULL;

	twig_allocator_t *allocator = calloc(1, sizeof(*allocator));
	if (!allocator)
		return NULL;

	allocator->dev_fd = ve_fd;

	first_chunk.pub_mem.phys = ve_info->phymem_start - PAGE_OFFSET;
	first_chunk.pub_mem.size = ve_info->phymem_total_size;
    first_chunk.pub_mem.virt = NULL;
    first_chunk.pub_mem.type = TWIG_MEM_VE;
    first_chunk.next = NULL;

	pthread_mutex_init(&allocator->lock, NULL);

	allocator->mem_alloc = twig_allocator_ve_mem_alloc;
	allocator->mem_free = twig_allocator_ve_mem_free;
	allocator->mem_flush = twig_allocator_ve_mem_flush;
	allocator->destroy = twig_allocator_ve_destroy;

	return allocator;
}