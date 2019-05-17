#ifndef REQUEST_QUEUE
#define REQUEST_QUEUE

#include <stdlib.h>
#include "../utils/types.h"

typedef struct reqQ_node{
    tlv_request_t key;
    struct reqQ_node * next;
} reqQ_node_t;

typedef struct requestQueue{
    reqQ_node_t *front, *back;
} reqQ_t;

reqQ_node_t * newNode(tlv_request_t k){
    reqQ_node_t *temp = (reqQ_node_t *) malloc(sizeof(reqQ_node_t));
    temp->key = k;
    temp->next = NULL;
    return temp;
}

reqQ_t* createQueue(){
    reqQ_t *q = (reqQ_t *) malloc(sizeof(reqQ_t));
    q->front = q->back = NULL;
    return q;
}

void enQueue(reqQ_t * q, tlv_request_t req){

    reqQ_node_t * temp = newNode(req);
    if (q->front == NULL){
        q->front = q->back = temp;
        return;
    }

    q->back->next = temp;
    q->back = temp;
}

reqQ_node_t * deQueue(reqQ_t *q){
    if(q->front == NULL) return NULL;

    reqQ_node_t * temp = q->front;
    q->front = q->front->next;

    if(q->front == NULL) q->back = NULL;

    return temp;
}

#endif
