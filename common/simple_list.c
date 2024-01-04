#include "simple_list.h"
#include "malloc.h"
#include "stddef.h"

void simple_list_add_tail( simple_list_node_t **root, simple_list_node_t *node ) {
    while ( *root != NULL) {
        root = &( *root )->next;
    }
    *root = node;
    node->next = NULL;
}

void simple_list_free( simple_list_node_t **root ) {
    simple_list_node_t *next = *root;
    while ( next != NULL) {
        simple_list_node_t *_this = next;
        next = next->next;
        free( _this );
    }
    *root = NULL;
}
