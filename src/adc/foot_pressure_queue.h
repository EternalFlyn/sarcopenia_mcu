#ifndef FLYN_FOOT_PRESSURE_QUEUE
#define FLYN_FOOT_PRESSURE_QUEUE

typedef struct foot_pressure_data {
    int16_t value[6];
} foot_pressure_data_t;

typedef struct data_node {
    sys_snode_t snode;
    int16_t ha, lt, m1, m5, arch, hm;
} data_node_t;

void foot_pressure_queue_push(struct k_queue*, foot_pressure_data_t);
uint16_t foot_pressure_queue_pop_amount(struct k_queue*, foot_pressure_data_t*, uint16_t);
void foot_pressure_queue_clean(struct k_queue*);

#endif