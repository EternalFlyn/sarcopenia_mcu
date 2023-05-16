#include <stdlib.h>
#include <stdint.h>
#include <kernel.h>

#include "foot_pressure_queue.h"

#define QUEUE_BLOCK_SIZE 8
#define QUEUE_BLOCK_COUNT 1024

K_MEM_SLAB_DEFINE(queue_slab, QUEUE_BLOCK_SIZE, QUEUE_BLOCK_COUNT, sizeof(void *));

void foot_pressure_queue_push(struct k_queue *queue, foot_pressure_data_t data) {
    data_node_t *node;
    if (!k_mem_slab_alloc(&queue_slab, &node, K_NO_WAIT)) {
        node->data.value[0] = data.value[0];
        node->data.value[1] = data.value[1];
        node->data.value[2] = data.value[2];
        node->data.value[3] = data.value[3];
        node->data.value[4] = data.value[4];
        node->data.value[5] = data.value[5];
    }
    else {
        printk("Memory allocation fail\n");
        return;
    }
    k_queue_append(queue, (void *) node);
}

uint16_t foot_pressure_queue_pop_amount(struct k_queue *queue, foot_pressure_data_t *array, uint16_t amount) {
    uint16_t pop_amount = 0;
    data_node_t *node;
    for (int i = 0; i < amount; i++) {
        if (k_queue_is_empty(queue)) break;
        node = k_queue_get(queue, K_NO_WAIT);
        array[i] = node->data;
        k_mem_slab_free(&queue_slab, &node);
        pop_amount++;
    }
    return pop_amount;
}

void foot_pressure_queue_clean(struct k_queue *queue) {
    data_node_t *node;
    while (!k_queue_is_empty(queue)) {
        node = k_queue_get(queue, K_NO_WAIT);
        k_mem_slab_free(&queue_slab, &node);
    }
}