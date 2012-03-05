"""
rbtree
~~~~~~~~~~~~~~~~~~~~
an rbtree impl for python using pyrex

"""

__author__ = 'Benjamin Saller <bcsaller@gmail.com>'
__copyright__ = 'Benjamin Saller 2010'
__license__  = 'The GNU Public License 3'


cdef extern from "stdio.h":
    void printf(char *fmt, ...)

cdef extern from "Python.h":
    ctypedef struct PyObject

cdef extern from "rbtree.h":
    ctypedef struct rbtree_node_t:
        PyObject *key
        PyObject *value
        rbtree_node_t *l
        rbtree_node_t *r
        rbtree_node_t *p

    ctypedef struct rbtree_t:
        rbtree_t *root
        rbtree_t *nil
        long      ct
        PyObject *compare

    rbtree_t *rbtree_alloc()
    int rbtree_init(rbtree_t *T)

    void rbtree_dealloc(rbtree_t *T)
    void rbtree_free(rbtree_t *T)

    int rbtree_add(rbtree_t *T, object key, object value)
    int rbtree_del(rbtree_t *T, object key)
    void *rbtree_get(rbtree_t *T, object key)
    rbtree_node_t *rbtree_node_get (rbtree_t *T, object key)
    rbtree_node_t *rbtree_node_del(rbtree_t *T, rbtree_node_t *i)

    rbtree_node_t *tree_min(rbtree_t *T, rbtree_node_t *i)
    rbtree_node_t *tree_max(rbtree_t *T, rbtree_node_t *i)
    rbtree_node_t *tree_successor(rbtree_t *T, rbtree_node_t *i)
    rbtree_node_t *tree_predecessor(rbtree_t *T, rbtree_node_t *i)



    void rbtree_do_slice(rbtree_t *T, object start,
                         object end, object step,
                         rbtree_t *new)

    void rbtree_do_del_slice(rbtree_t *T, object start, object end,
                             object step)


    void rbtree_validate(rbtree_t *T, int verbose)

    void rbtree_set_compare(rbtree_t *T, object compare)
    object rbtree_get_compare(rbtree_t *T)

    ctypedef enum iter_direction:
        FORWARD = 1
        BACKWARD = -1

KEYS   = 1
VALUES = 2
ITEMS  = 4
NODES  = 8

cdef class rbbase
cdef class rbtree
cdef class rbset

cdef class rbbase:
    cdef rbtree_t *_tree

    def __dealloc__(self):
        rbtree_dealloc(self._tree)
        rbtree_free(self._tree)

    # Simple pickle support
    def __getnewargs__(self, *args):
        return ()

    def __setstate__(self, state):
        # if there was a compare function we need to try to pull it
        # back in
        if state['compare'] is not None:
            rbtree_set_compare(self._tree, state['compare'])

        self.update(state['data'])


    def __len__(self):
        return self._tree.ct

    def __contains__(self, key):
        cdef void *v
        v = rbtree_get(self._tree,  key)
        return v is not NULL

    def has_key(self, key):
        return key in self


    def __iter__(self):
        return rb_iterator(self, KEYS)

    def __reversed__(self):
        i = rb_iterator(self, KEYS)
        i.direction = BACKWARD
        return i

    def copy(self):
        ## XXX: check ref count handling
        return self.__class__(self, cmp=rbtree_get_compare(self._tree))

    cdef empty_copy(self):
        return self.__class__(cmp=rbtree_get_compare(self._tree))

    def clear(self):
        rbtree_dealloc(self._tree)
        rbtree_free(self._tree)
        self._tree = rbtree_alloc()
        rbtree_init(self._tree)

    def min(self):
        cdef rbtree_node_t *n
        if len(self) == 0:
            return None
        return <object>(tree_min(self._tree, NULL).key)

    def max(self):
        cdef rbtree_node_t *n
        if len(self) == 0:
            return None
        return <object>(tree_max(self._tree, NULL).key)


cdef class rb_iterator:
    cdef rbbase _T
    cdef rbtree_node_t *_iter
    cdef int _type
    cdef int _done
    cdef iter_direction _direction

    def __cinit__(self, rbbase tree, int itype):
        self._T = tree
        self._iter = NULL
        self._type = itype
        self._done = False
        self.direction = FORWARD

    property direction:
        def __get__(self):
            return self._direction
        def __set__(self, int value):
            self._direction = <iter_direction>value

    cdef _position(self, iter_direction direction):
        if self._done:
            raise StopIteration

        if direction == FORWARD:
            self._iter = tree_min(self._T._tree, NULL)
        else:
            self._iter = tree_max(self._T._tree, NULL)

    cdef rbtree_node_t* walk(self, iter_direction direction):
        cdef rbtree_node_t *n
        if direction == FORWARD:
            n = tree_successor(self._T._tree, self._iter)
        else:
            n = tree_predecessor(self._T._tree, self._iter)
        return n

    # iter protocol
    def __iter__(self):
        return self

    cdef step(self, iter_direction direction):
        if self._iter is NULL: self._position(direction)
        else: self._iter = self.walk(direction)

        if self._iter is NULL:
            self._done = True
            raise StopIteration

        if self._type == KEYS: r = self.key
        elif self._type == VALUES: r = self.value
        elif self._type == NODES: r = self
        else: r = self.item
        return r

    def __next__(self): return self.step(self.direction)
    def prev(self): return self.step(self.direction * -1)

    # Extensions
    def goto(self, key):
        cdef rbtree_node_t *n
        n = rbtree_node_get(self._T._tree, key)
        if n is NULL:
            self._done = True
            self._iter = NULL
            raise KeyError, key
        else:
            self._iter = n

    property key:
        def __get__(self):
            if self._iter is not NULL:
                result = <object> (self._iter.key)
                return result
            raise KeyError("Iterator has no current value")

    property value:
        def __get__(self):
            if self._iter is not NULL:
                result = <object> (self._iter.value)
                return result
            raise KeyError, "Iterator has no current value"

    property item:
        def __get__(self):
            if self._iter is not NULL:
                result = (<object> (self._iter.key),
                          <object> (self._iter.value))
                return result
            raise KeyError , "Iterator has no current value"

    def get(self, mode=VALUES):
        # return the property of this node indicated by key
        if mode == KEYS: return self.key
        elif mode == VALUES: return self.value
        elif mode == ITEMS: return (self.key, self.value)
        else: return self

    def delete(self):
        # equivlent to (call next, remove prev node)
        # remove the current node and call next on the iter
        cdef rbtree_node_t *n
        # delete the original node
        n = rbtree_node_del(self._T._tree, self._iter)
        # push the iter to the next location
        if self.direction is BACKWARD:
            n = tree_predecessor(self._T._tree, n)
        self._iter = n
        if self._iter is NULL:
            self._done = True
            raise StopIteration


    def __len__(self):
        return len(self._T)

    def __nonzero__(self):
        return not self._done


cdef class rbtree(rbbase):
    def __cinit__(self, mapping=None, cmp=None):
        self._tree = rbtree_alloc()
        rbtree_init(self._tree)

        if cmp is not None:
            rbtree_set_compare(self._tree, cmp)

        if mapping:
            self.update(mapping)

    # Compare support
    def __richcmp__(self, other, mode):
        if not isinstance(other, rbtree):
            raise ValueError, "Can only test equality of two %s" % self.__class__.__name__
        # We only support the eq check
        if mode != 2: return False
        if len(other) != len(self): return False
        s = self.iternodes()
        o = other.iternodes()
        for i in self:
            if s.next().item != o.next().item: return False
        return True

    # pickle support
    def __getstate__(self):
        d = dict(self)
        return {'data' : d,
                'compare' : <object> self._tree.compare,
                }


    def __setitem__(self, key, value):
        # calling hash on the key verifies that its not
        # mutilble, as far as a dict would anyway...
        if isinstance(key, slice):
            raise ValueError, "setslice is unsupported"

        rbtree_add(self._tree,  key,  value)

    def __getitem__(self, key):
        cdef void * v

        if isinstance(key, slice):
            return self.__doslice__(key)

        v = rbtree_get(self._tree, key)
        if v is NULL: raise KeyError, key
        return <object> v

    def __delitem__(self, key):
        cdef int rc

        if isinstance(key, slice):
            self.__dodeleteslice__(key)
            return

        rc = rbtree_del(self._tree,  key)
        if rc != 0: raise KeyError, key


    def __doslice__(self, sliceobj):
        # This is our hacked up version that getattr will invoke with
        # a slice object. We support key ordering so we could have
        # integer offsets into the results, but really we want to
        # honor the key space and be able to say tree[a:z] to pull all
        # values between a->z. A new rbtree is returned
        instance = self.__class__()
        rbtree_do_slice(self._tree,
                        sliceobj.start,
                        sliceobj.stop,
                        sliceobj.step,
                        (<rbtree>instance)._tree)
        return instance

    def __dodeleteslice__(self, sliceobj):
        rbtree_do_del_slice(self._tree, sliceobj.start, sliceobj.stop, sliceobj.step)

    def get(self, key, default=None):
        try:
            return self[key]
        except KeyError:
            return default


    def iterkeys(self): return rb_iterator(self, KEYS)
    def itervalues(self): return rb_iterator(self, VALUES)
    def iteritems(self): return rb_iterator(self, ITEMS)
    def iternodes(self): return rb_iterator(self, NODES)

    def keys(self): return list(self.__iter__())
    def values(self): return list(self.itervalues())
    def items(self): return list(self.iteritems())

    def update(self, mapping):
        if isinstance(mapping, (dict, rbtree)):
            for k,v in mapping.iteritems():
                self[k] = v
        elif isinstance(mapping, (list, tuple)):
            # ((k,v), ... )
            try:
                for k, v in mapping:
                    self[k] = v
            except ValueError:
                raise ValueError("lists and tuples must be of the form [(k, v), ...]")
        else:
            raise TypeError, "Unsupported type %s for update(need dict/sequence)" % type(mapping)

    def pop(self):
        key = self.__iter__().next()
        v = self[key]
        del self[key]
        return v

    def setdefault(self, key, default):
        if key not in self:
            self[key] = default
            return default
        return self[key]

    def byOffset(self, offset):
        cdef rbtree_node_t *n

        if offset >= 0:
            n = tree_min(self._tree, NULL)
            forward = True
        else:
            n = tree_max(self._tree, NULL)
            forward = False
            offset = abs(offset) -1

        for i in xrange(offset):
            if forward:
                n = tree_successor(self._tree, n)
            else:
                n = tree_predecessor(self._tree, n)

        return <object>(n.key)

    def __repr__(self):
        # represent the rbtree as a list of key/value tuples
        res = []
        for k,v in self.iteritems():
            res.append("(%r,  %r)" % (k,v))
        if len(res):
            res = ' ' + ', '.join(res)
        else:
            res = ''
        return "[%s]" % (res)



cdef class rbset(rbbase):
    """set implemented to support ordering and basic set operations
    using a rbtree for the underlying implementation.

    With control over the compare function you can treat this either
    as a set (unique membership) or a bag where you might add multiple
    items of the same value.
    """

    # XXX: default cmp function is insertion order
    def __cinit__(self, mapping=None, cmp=None):
        self._tree = rbtree_alloc()
        rbtree_init(self._tree)

        if cmp is not None:
            rbtree_set_compare(self._tree, cmp)

        if mapping:
            self.update(mapping)

    # Compare support
    def __richcmp__(self, other, mode):
        if not isinstance(other, rbset):
            raise ValueError, "Can only test equality of two rbset"
        # We only support the eq check
        if mode != 2: return False
        if len(other) != len(self): return False
        if self.symmetric_difference(other):
            return False
        return True

    # pickle support
    def __getstate__(self):
        d = list(self)
        return {'data' : d,
                'compare' : <object> self._tree.compare,
                }


    # set interface
    def intersection(self, other):
        r = self.empty_copy()
        for item in self:
            if item in other:
                r.add(item)
        return r

    def __and__(self, other):
        return self.intersection(other)

    def difference(self, other):
        r = self.empty_copy()
        for item in self:
            if item not in other:
                r.add(item)
        return r

    def __sub__(self, other):
        return self.difference(other)

    def symmetric_difference(self, other):
        """Return a new set with elements in either the set or other
        but not both."""
        r = self.empty_copy()
        for item in self:
            if item not in other:
                r.add(item)

        for item in other:
            if item not in self:
                r.add(item)
        return r

    def __xor__(self, other):
        return self.symmetric_difference(other)


    def isdisjoint(self, other):
        """Return True if the set has no elements in common with
        other. Sets are disjoint if and only if their intersection is
        the empty set."""
        for item in self:
            if item in other:
                return False
        return True

    def issubset(self, other):
        """Test whether every element in the set is in other."""
        if len(self) > len(other):
            return False
        for item in self:
            if item not in other:
                return False
        return True

    def issuperset(self, other):
        """Test whether every element in other is in the set."""
        if len(other) > len(self):
            return False
        for item in other:
            if item not in self:
                return False
        return True

    def union(self, *iterables):
        r = self.empty_copy()
        iterables = [self] + list(iterables)
        for iterable in iterables:
            r.update(iterable)
        return r

    def __or__(self, iterable):
        return self.union(iterable)

    def __ior__(self, iterable):
        self.update(iterable)
        return self


    def difference_update(self, *iterables):
        for iterable in iterables:
            for el in iterable:
                self.discard(el)
        return self

    def __isub__(self, iterable):
        return self.difference_update(iterable)

    def intersection_update(self, *iterables):
        """Update the set, keeping only elements found in it and all
        others."""
        for el in self:
            for iterable in iterables:
                if el not in iterable:
                    self.discard(el)
                    break
        return self

    def __iand__(self, iterable):
        return self.intersection_update(iterable)

    ## def symmetric_difference_update(self, *iterables):
    ##     """Update the set, keeping only elements found in either set, but not in both."""
    ##     for el in self:
    ##         for iterable in iterables:
    ##             if el in iterable: break


    def add(self, item):
        rbtree_add(self._tree, item, None)

    def remove(self, item):
        if item not in self:
            raise KeyError(item)
        rbtree_del(self._tree, item)

    def discard(self, item):
        if item in self:
            rbtree_del(self._tree, item)


    def update(self, sequence):
        for el in sequence:
            self.add(el)

    def pop(self):
        key = self.__iter__().next()
        self.discard(key)
        return key

    def __repr__(self):
        # represent the rbtree as a list of key/value tuples
        if len(self):
            res = []
            for i in self:
                res.append(repr(i))
            res = ', '.join(res)
        else:
            res = ''
        return "[%s]" % (res)
