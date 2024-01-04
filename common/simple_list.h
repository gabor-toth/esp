//
// Created by tothg on 2024.01.04..
//

#ifndef HAJO_SIMPLE_LIST_H
#define HAJO_SIMPLE_LIST_H

typedef struct simple_list_node_t {
    struct simple_list_node_t *next;
} simple_list_node_t;

extern void simple_list_add_tail( simple_list_node_t **root, simple_list_node_t *node );

extern void simple_list_free( simple_list_node_t **root );

#endif //HAJO_SIMPLE_LIST_H
