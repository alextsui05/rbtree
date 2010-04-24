Fast Red-Black Tree implementation for Python
=============================================
:Author: Benjamin Saller <bcsaller@gmail.com>


An rbtree is a fast, balanced efficient data structure with the
following properties:

get      O(log n)
set      O(log n)
delete   O(log n)
min	     O(log n)
max	     O(log n)
contains O(log n)

Because the worst case timing is minimal across the range of standard
dict and ordered data operations it makes sense to use this when you
have volatile/dynamic sorted data.

In common usage its nearly as fast as the Python dict impl but has a
slightly more expensive usage of the compare function as the keys are
ordered and not hashed.

Installation
=============
Uses distutils (and optionally make)

sudo easy_install rbtree

OR

sudo python setup.py install

OR

make
sudo make install


Usage
=====

rbtree.rbtree can be used as a normal Python dictionary but will keep
values ordered by a key compare function.

>>> import rbtree
>>> r = rbtree.rbtree()

We can assign key/values like a dictionary using the same protocol.
>>> r[2] = 3
>>> r[1] = 2
>>> r[3] = 4

Dictionary methods like keys, items and values work but the keys will
come out ordered with the compare function.

>>> assert r.keys() == [1,2,3]



Protocol Extensions
===================

A protocol is the combination of syntactic style and invocation
semantic associated with the usage of an object. For example dictionary
like getitem access is a protocol as is object.method. Because rbtrees
support fast ordered data manipulation it is convenient to extend the
iteration protocol to support its additional capabilities.

Here we create an rbtree with the key/value pairs from 1 to 9.

>>> r = rbtree.rbtree(dict(zip(range(10), range(10))))

Create an iterator
>>> i = iter(r)

and jump to the key/value pair pointed at by key 5
>>> i.goto(5)

We can now verify that the next keys we see in normal iteration are
as expected
>>> assert i.next() == 6
>>> assert i.next() == 7

Lets finish off the iterator now
>>> assert list(i) == [8, 9]

Other interesting extensions to the iterator protocol are as
follows. Because keys are ordered we can support mutation (and
deletion) of the tree while doing iteration.

>>> r = rbtree.rbtree(zip(range(10), range(10)))

We will now use the iternodes method of the tree to walk the key value
node set. Each node has the following attributes
     .key -> key
     .value -> value
     .item -> (k,v)
and a .delete() method which remove the node. The iterator will be set
to the next node in the tree in the direction indicated by
iter.direction which can be 1 (the default) for forwards iteration and
-1 for backwards.

>>> for i in  r.iternodes():
...    i.delete()


>>> r = rbtree.rbtree(zip(range(10), range(10)))
>>> i = iter(r)
>>> i.goto(5)
>>> i.delete()

>>> assert i.item == (6, 6)

Here we will alter the direction of the iterator to point backwards,
now when we do a delete the cursor will end up on the previous node.
>>> i = iter(r)
>>> i.direction = -1
>>> i.goto(4)
>>> i.delete()
>>> assert i.item == (3,3)

Using the iternodes interface and the .delete method is a rare and
special case if you are treating the rbtree as an ordered
dictionary. However, if you are using custom compare functions that
allows the same key into the tree more than once then its very useful
to take an iterator and walk sets of similar keys which the dictionary
interface will not be able to return. If this use-case doesn't make
sense to you it continue using the rbtree as an ordered dict.


