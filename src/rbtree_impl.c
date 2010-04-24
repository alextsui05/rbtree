#include "rbtree.h"
#include <Python.h>
#include <string.h>
#include <assert.h>

#define NEW(x,t)       { \
                           x = (t *) PyMem_Malloc(sizeof(t)); \
                           if (x == NULL) return 0; \
                         }

#define DEL(x)           { \
                             if (x != NULL)     \
                           { \
                               PyMem_Free(x); \
                               x = NULL;  \
                           } \
                         }



#define COMPARE(T, X, Y) ((T->compare) == Py_None ? \
                          T->cmp_func(X, Y) : \
                          rbtree_pycmp(T->compare, X, Y))



static
int
rbtree_pycmp(PyObject *compare, PyObject *x, PyObject *y)
{
  // Invoke a Python compare function returning the result as an int.
  PyObject *res;
  PyObject *args;
  int i;
  args = PyTuple_New(2);
  if (args == NULL)
    return -1;
  Py_INCREF(x);
  Py_INCREF(y);
  PyTuple_SET_ITEM(args, 0, x);
  PyTuple_SET_ITEM(args, 1, y);
  res = PyObject_Call(compare, args, NULL);
  Py_DECREF(args);
  if (res == NULL) return -1;
  if (!PyInt_Check(res)) {
    Py_DECREF(res);
    PyErr_SetString(PyExc_TypeError, "comparison function must return int");
    return -1;
  }
  i = PyInt_AsLong(res);
  Py_DECREF(res);
  return i;

}

static
int
rbtree_node_compare(PyObject *x, PyObject *y)
{
  /* a three way compare that should work with whatever objects support */
  int gt = PyObject_RichCompareBool(x,y, Py_GT);
  if (gt == 1) return 1;
  int lt = PyObject_RichCompareBool(x,y, Py_LT);
  if (lt == 1)  return -1;
  return 0;
}

static
int
rbtree_node_compare_string(PyObject *x, PyObject *y)
{
  return strcmp((char*)PyString_AS_STRING(x),
		(char*)PyString_AS_STRING(y));
}

static
void
__rotate_left     (rbtree_t *T, rbtree_node_t *x)
{
   rbtree_node_t  *y = x->r;

   x->r      = y->l;
   if (x->r != T->nil)
   {
      x->r->p = x;
   }
   y->p      = x->p;

   if (y->p == T->nil)
   {
      T->root = y;
      goto gc;
   }
   if (y->p->l == x)
   {
      y->p->l = y;
      goto gc;
   }
   {
      y->p->r = y;
   }

 gc:
   y->l    = x;
   x->p    = y;
}

static
void
__rotate_right    (rbtree_t *T, rbtree_node_t *x)
{
   rbtree_node_t  *y = x->l;

   x->l      = y->r;
   if (x->l != T->nil)
   {
      x->l->p = x;
   }
   y->p      = x->p;

   if (y->p == T->nil)
   {
      T->root = y;
      goto gc;
   }
   if (y->p->l == x)
   {
      y->p->l = y;
      goto gc;
   }
   {
      y->p->r = y;
   }

 gc:
   y->r    = x;
   x->p    = y;
}

static
rbtree_node_t *
__tree_min        (rbtree_t *T, rbtree_node_t *x)
{
   while (x->l != T->nil)
   {
      x = x->l;
   }

   return x;
}

static
rbtree_node_t *
__tree_max        (rbtree_t *T, rbtree_node_t *x)
{
   while (x->r != T->nil)
   {
      x = x->r;
   }

   return x;
}

static
rbtree_node_t *
__tree_successor  (rbtree_t *T, rbtree_node_t *x)
{
   rbtree_node_t *y;

   if (x->r != T->nil)
   {
      return __tree_min(T, x->r);
   }

   for (y = x->p; (y != T->nil) && (x == y->r); )
   {
      x = y;
      y = y->p;
   }

   return y;
}


static
rbtree_node_t *
__tree_predecessor(rbtree_t *T, rbtree_node_t *x)
{
   rbtree_node_t *y;

   if (x->l != T->nil)
   {
      return __tree_max(T, x->l);
   }

   for (y = x->p; (y != T->nil) && (x == y->l); )
   {
      x = y;
      y = y->p;
   }

   return y;
}

static
rbtree_node_t *
__tree_search     (rbtree_t *T, rbtree_node_t *x, const PyObject *k, int *rc)
{
  rbtree_node_t *y = T->nil;

   for ((*rc) = -1; (x != T->nil); )
     {
       (*rc) = COMPARE(T, (PyObject *)k, x->key);
       y     = x;

       if ((*rc) == 0)
	   return x;
       if ((*rc)  < 0) x = x->l;
       else x = x->r;
     }

   return ((*rc) < 0) ? y : __tree_successor(T, y);
}


static
rbtree_node_t *
__tree_insert     (rbtree_t *T, PyObject *k, PyObject *v)
{
   rbtree_node_t *x = T->root;
   rbtree_node_t *y = T->nil;
   int            rc = 0;

   // Use the optimized function until we can't
   if (!PyString_CheckExact(k)) {
     T->cmp_func = rbtree_node_compare;
   }

   while (x != T->nil)
   {
     rc = COMPARE(T, k, x->key);
     y  = x;

     if (rc == 0)
       {
	 // force a value update
	 // XXX: (later) validate this?
	 Py_XDECREF(x->key);
	 Py_XDECREF(x->value);
	 x->key = k;
	 x->value = v;
	 return 0;
       }
     if (rc  < 0) x = x->l;
     else x = x->r;
   }

   NEW(x, rbtree_node_t);
   x->key = k;
   x->value = v;
   x->p   = y;
   x->l   = T->nil;
   x->r   = T->nil;
   MAKE_BLACK(x);
   T->ct++;


   if (y == T->nil)
   {
      T->root = x;
      return x;
   }

   if (rc < 0)
   {
     y->l = x;
     return x;
   }

   {
     y->r = x;
     return x;
   }
}

static
int
__rb_insert (rbtree_t *T, PyObject *k, PyObject *v)
{
  rbtree_node_t *x = __tree_insert(T, k, v);
  rbtree_node_t *y;

  if (x ==  NULL) {
    return 0;
  }

  Py_INCREF(k);
  Py_INCREF(v);


  if (x)
    {
     MAKE_RED  (x);

      while ((x != T->root) && (COLOR(x->p) == RED))
      {
	 if (x->p == x->p->p->l)
	 {
	    y = x->p->p->r;
	    if (COLOR(y) == RED)
	    {
	       MAKE_BLACK(x->p);
	       MAKE_BLACK(y);
	       MAKE_RED  (x->p->p);
	       x = x->p->p;
	    }
	    else
	    {
	       if (x == x->p->r)
	       {
		  x = x->p;
		  __rotate_left(T, x);
	       }
	       MAKE_BLACK(x->p);
	       MAKE_RED  (x->p->p);
	       __rotate_right(T, x->p->p);
	       break;
	    }
	 }
	 else
	 {
	    y = x->p->p->l;
	    if (COLOR(y) == RED)
	    {
	       MAKE_BLACK(x->p);
	       MAKE_BLACK(y);
	       MAKE_RED  (x->p->p);
	       x = x->p->p;
	    }
	    else
	    {
	       if (x == x->p->l)
	       {
		  x = x->p;
		  __rotate_right(T, x);
	       }
	       MAKE_BLACK(x->p);
	       MAKE_RED  (x->p->p);
	       __rotate_left(T, x->p->p);
	       break;
	    }
	 }
      }

      MAKE_BLACK(T->root);
      return 0;
   }
   return 1;
}

static
void
__rb_del_fix(rbtree_t *T, rbtree_node_t *x)
{
   rbtree_node_t *w;

   while ((x != T->root) && (COLOR(x) == BLACK))
   {
      if (x == x->p->l)
      {
	 w = x->p->r;
	 if (COLOR(w) == RED)
	 {
	    MAKE_BLACK(w);
	    MAKE_RED  (x->p);
	    __rotate_left(T, x->p);
	    w = x->p->r;
	 }
	 if ((COLOR(w->l) == BLACK) && (COLOR(w->r) == BLACK))
	 {
	    MAKE_RED(w);
	    x = x->p;
	 }
	 else
	 {
	    if (COLOR(w->r) == BLACK)
	    {
	       MAKE_BLACK(w->l);
	       MAKE_RED  (w);
	       __rotate_right(T, w);
	       w = x->p->r;
	    }

	    w->color = COLOR(x->p);

	    MAKE_BLACK(x->p);
	    MAKE_BLACK(w->r);
	    __rotate_left(T, x->p);
	    x = T->root;
	    break;
	 }
      }
      else
      {
	 w = x->p->l;
	 if (COLOR(w) == RED)
	 {
	    MAKE_BLACK(w);
	    MAKE_RED  (x->p);
	    __rotate_right(T, x->p);
	    w = x->p->l;
	 }
	 if ((COLOR(w->l) == BLACK) && (COLOR(w->r) == BLACK))
	 {
	    MAKE_RED(w);
	    x = x->p;
	 }
	 else
	 {
	    if (COLOR(w->l) == BLACK)
	    {
	       MAKE_BLACK(w->r);
	       MAKE_RED  (w);
	       __rotate_left(T, w);
	       w = x->p->l;
	    }
	    w->color = COLOR(x->p);
	    MAKE_BLACK(x->p);
	    MAKE_BLACK(w->l);
	    __rotate_right(T, x->p);
	    x = T->root;
	    break;
	 }
      }
   }

   MAKE_BLACK(x);
}

static
rbtree_node_t *
__rb_del_node(rbtree_t *T, rbtree_node_t *z)
{
  // Delete a node and return successor
  rbtree_node_t  *y;
  rbtree_node_t  *x;
  rbtree_node_t  *remainder = NULL;

  if (z == NULL) return NULL;
  remainder = __tree_successor(T, z);
  if ((z->l == T->nil) || (z->r == T->nil)) y = z;
  else y = remainder;

  if (y->l != T->nil) x = y->l;
  else                x = y->r;

  x->p = y->p;
  if (y->p == T->nil) T->root = x;
  else
    {
      // Disconnect y from tree
      if (y == y->p->l) y->p->l = x;
      else              y->p->r = x;
    }
  if (y != z)
    {
      // copy y to z replace z, the delete target with y's data, then
      // remove the old shell. saves a rotate.
      Py_DECREF(z->key);
      Py_DECREF(z->value);
      z->key = y->key;
      z->value = y->value;
      remainder = z;
    }

  if (COLOR(y) == BLACK)
    {
      __rb_del_fix(T, x);
    }

  DEL(y);

  T->ct--;
  return remainder;
}



static
int
__rb_del    (rbtree_t *T, PyObject *k)
{
   rbtree_node_t  *z;
   int             rc;

   z = __tree_search(T, T->root, k, &rc);
   if (rc == 0) {
     __rb_del_node(T, z);
     return 0;
   }
   return 1;
}


rbtree_node_t *
rbtree_node_del(rbtree_t *T, rbtree_node_t *n) {
  if (n == NULL) return 0;
  if (n == T->nil) return 0;
  return __rb_del_node(T, n);
}


rbtree_t *
rbtree_alloc           (void)
{
   rbtree_t       *l_rbt;

   NEW(l_rbt, rbtree_t);
   return l_rbt;
}

int
rbtree_init            (rbtree_t *T)
{
   NEW(T->nil, rbtree_node_t);
   memset(T->nil, 0, sizeof(rbtree_node_t));
   MAKE_BLACK(T->nil);
   T->nil->l = T->nil;
   T->nil->r = T->nil;
   T->nil->p = T->nil;
   T->root   = T->nil;
   T->ct     = 0;
   T->cmp_func = rbtree_node_compare_string;
   T->compare = Py_None;
   Py_INCREF(Py_None);

   return 0;
}


void
rbtree_set_compare(rbtree_t *T, PyObject *compare)
{
  if (T->ct == 0) {
    // We can only set the compare function when the tree is empty
    Py_DECREF(T->compare);
    T->compare = compare;
    Py_INCREF(T->compare);
  }
}


static
void
__destroy              (rbtree_t *T, rbtree_node_t *n)
{
   if (n != T->nil)
   {
      Py_DECREF(n->key);
      Py_DECREF(n->value);
      __destroy(T, n->l);
      __destroy(T, n->r);
      DEL      (n);
   }
}

void
rbtree_dealloc(rbtree_t *T)
{
  __destroy(T, T->root);
  Py_XDECREF(T->compare);
  DEL(T->nil);
}


void
rbtree_free            (rbtree_t *T)
{
   DEL(T);
}


int
rbtree_add             (rbtree_t *T, PyObject *a_ptr, PyObject *value)
{
  return __rb_insert(T, a_ptr, value);
}

int
rbtree_del             (rbtree_t *T, PyObject *a_ptr)
{
  return __rb_del   (T, a_ptr);
}

void *
rbtree_get             (rbtree_t *T, PyObject *key)
{
   rbtree_node_t  *x;
   int             rc;

   x = __tree_search(T, T->root, key, &rc);

   return (rc != 0) ? NULL : (x->value);
}

rbtree_node_t *
rbtree_node_get (rbtree_t *T, PyObject *key)
{
   rbtree_node_t  *x;
   int             rc;

   x = __tree_search(T, T->root, key, &rc);

   return (rc != 0) ? NULL : x;
}

rbtree_node_t *
tree_min(rbtree_t *T, rbtree_node_t *x) {
  if (x == NULL) x = T->root;
  rbtree_node_t *n = __tree_min(T, x);
  if (n == T->nil) n = NULL;
  return n;
}

rbtree_node_t *
tree_max(rbtree_t *T, rbtree_node_t *x) {
  if (x == NULL) x = T->root;
  rbtree_node_t *n = __tree_max(T, x);
  if (n == T->nil) n = NULL;
  return n;
}

rbtree_node_t *
tree_successor(rbtree_t *T, rbtree_node_t *x) {
  rbtree_node_t *n = __tree_successor(T, x);
  if (n == T->nil) n = NULL;
  return n;
}

rbtree_node_t *
tree_predecessor(rbtree_t *T, rbtree_node_t *x) {
  rbtree_node_t *n = __tree_predecessor(T, x);
  if (n == T->nil) n = NULL;
  return n;
}


#define ABS( a )     ( (a) < 0 ? -(a) : (a) )

void rbtree_do_slice(rbtree_t *T, PyObject *start, PyObject *end, PyObject *stride, rbtree_t *new) {
  // return a new rbtree object built from the key slice. Because keys
  // are ordered we slice on the actual keys and not their
  // index/offsets
  rbtree_node_t *sn, *en, *cursor;
  long step = 0;
  long offset = 0;
  int rc = 0;

  // Look for the nearest inclusive match
  if (start == Py_None)
    sn = tree_min(T, NULL);
  else
    sn = __tree_search(T, T->root, start, &rc);

  if (end == Py_None)
    en = T->nil;
  else
    en = __tree_search(T, T->root, end, &rc);

  if (stride != Py_None) {
    step = PyInt_AsLong(stride);
    // negative step makes no sense when keys are added via a cmp function
    if (step < 0) step = ABS(step);
  }

  for(cursor = sn; cursor!= en; offset++) {
    if (step == 0 || (offset % step == 0))
      rbtree_add(new, cursor->key, cursor->value);
    cursor = __tree_successor(T, cursor);
  }
}

void rbtree_do_del_slice(rbtree_t *T, PyObject *start, PyObject *end,
                             PyObject *stride)
{
  rbtree_node_t *sn, *en, *cursor;
  long step = 0;
  long offset = 0;
  int rc = 0;

  // Look for the nearest inclusive match
  if (start == Py_None)
    sn = tree_min(T, NULL);
  else
    sn = __tree_search(T, T->root, start, &rc);

  if (end == Py_None)
    en = T->nil;
  else
    en = __tree_search(T, T->root, end, &rc);

  if (stride != Py_None) {
    step = PyInt_AsLong(stride);
    // negative step makes no sense when keys are added via a cmp function
    if (step < 0) step = ABS(step);
  }

  for(cursor = sn; cursor!= en; offset++) {
    if (step == 0 || (offset % step == 0)) {
      cursor = rbtree_node_del(T, cursor);
    } else {
      cursor = __tree_successor(T, cursor);
    }
    if (cursor == T->nil) break;
  }

}




