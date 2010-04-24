#ifndef __RBTREE_H
#define __RBTREE_H

#include <Python.h>

typedef enum {
  BLACK = 0,
  RED   = 1
} rbtree_color;

typedef enum  {
  BACKWARD = -1,
  FORWARD  = 1
} iter_direction;

#define COLOR(x)       (x->color)
#define DATA(x)        (x->value)

#define MAKE_BLACK(x)  (x->color = BLACK)
#define MAKE_RED(x)    (x->color = RED)


typedef struct rbtree_node rbtree_node_t;

struct rbtree_node {
  PyObject          *key;
  PyObject          *value;
  rbtree_node_t     *l;
  rbtree_node_t     *r;
  rbtree_node_t     *p;
  rbtree_color       color;
};

typedef struct {
  rbtree_node_t     *root;
  rbtree_node_t     *nil;
  long               ct;
  int (*cmp_func)(PyObject *a, PyObject *b);
  PyObject          *compare;
} rbtree_t;


// lifecycle
rbtree_t *rbtree_alloc(void);
int rbtree_init(rbtree_t *rbt);
void rbtree_dealloc(rbtree_t *rbt);
void rbtree_free(rbtree_t *rbt);
void rbtree_set_compare(rbtree_t *T, PyObject *compare);

// manipulation
int rbtree_add(rbtree_t *rbt, PyObject *key, PyObject *value);
int rbtree_del(rbtree_t *rbt, PyObject *key);
void *rbtree_get(rbtree_t *rbt, PyObject *key);

// node manipulation
rbtree_node_t *rbtree_node_get (rbtree_t *T, PyObject *key);
rbtree_node_t *rbtree_node_del(rbtree_t *T, rbtree_node_t *n);

// iteration
rbtree_node_t *tree_min(rbtree_t *T, rbtree_node_t *n);
rbtree_node_t *tree_max(rbtree_t *T, rbtree_node_t *n);
rbtree_node_t *tree_successor(rbtree_t *T, rbtree_node_t *x);
rbtree_node_t *tree_predecessor(rbtree_t *T, rbtree_node_t *x);

// slices
void rbtree_do_slice(rbtree_t *T, PyObject *s, PyObject *e, PyObject *step, rbtree_t *new);
void rbtree_do_del_slice(rbtree_t *T, PyObject *start, PyObject *end, PyObject *stride);

#endif
