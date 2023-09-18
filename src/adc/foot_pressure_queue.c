#include <stdlib.h>
#include <stdint.h>
#include <kernel.h>

#include "foot_pressure_queue.h"

#define QUEUE_BLOCK_SIZE 16
#define QUEUE_BLOCK_COUNT 1024

K_MEM_SLAB_DEFINE(queue_slab, QUEUE_BLOCK_SIZE, QUEUE_BLOCK_COUNT, sizeof(void *));

static uint16_t slab_data_count = 0;

void foot_pressure_queue_push(struct k_queue *queue, foot_pressure_data_t data) {
    data_node_t *node;
    if (slab_data_count >= QUEUE_BLOCK_COUNT) {
        // printk("block the data\n");
        return;
    }
    if (
        // slab_data_count < QUEUE_BLOCK_COUNT ||
        !k_mem_slab_alloc(&queue_slab, (void**) &node, K_NO_WAIT)
    ) {
        node->ha = data.value[0];
        node->lt = data.value[1];
        node->m1 = data.value[2];
        node->m5 = data.value[3];
        node->arch = data.value[4];
        node->hm = data.value[5];
    }
    else {
        printk("Memory allocation fail\n");
        return;
    }
    k_queue_append(queue, (void*) node);
    slab_data_count++;
}

uint16_t foot_pressure_queue_pop_amount(struct k_queue *queue, foot_pressure_data_t *array, uint16_t amount) {
    uint16_t pop_amount = 0;
    data_node_t *node;
    for (int i = 0; i < amount; i++) {
        if (k_queue_is_empty(queue)) break;
        node = k_queue_get(queue, K_NO_WAIT);

        array[i].value[0] = node->ha;
        array[i].value[1] = node->lt;
        array[i].value[2] = node->m1;
        array[i].value[3] = node->m5;
        array[i].value[4] = node->arch;
        array[i].value[5] = node->hm;

        k_mem_slab_free(&queue_slab, (void**) &node);
        pop_amount++;
    }
    slab_data_count -= pop_amount;
    return pop_amount;
}

void foot_pressure_queue_clean(struct k_queue *queue) {
    data_node_t *node;
    while (!k_queue_is_empty(queue)) {
        node = k_queue_get(queue, K_NO_WAIT);
        k_mem_slab_free(&queue_slab, (void**) &node);
    }
    slab_data_count = 0;
}