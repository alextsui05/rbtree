import unittest
import doctest
from rbtree import rbtree, rbset
from rbtree import (KEYS, VALUES, ITEMS, NODES)


class Test(unittest.TestCase):
    def supply_words(self, ds):
        for word in open('/usr/share/dict/words', 'r'):
            word = word.strip()
            ds[word] = True

    def sample_data(self):
        r = rbtree()
        self.supply_words(r)
        return r

    def test_create(self):
        r = rbtree()
        assert len(r) == 0

    def test_create_mapping(self):
        r = rbtree({'a': 'b'})
        assert len(r) == 1
        assert r['a'] == 'b'

    def test_dict(self):
        r = rbtree({'a': 'b'})
        d = dict(r)
        assert d['a'] == 'b'

    def test_create_sequence(self):
        r = rbtree((('a', 'b'), ('c', 'd')))
        assert len(r) == 2
        assert r['a'] == 'b'
        assert r['c'] == 'd'

    def test_missing(self):
        r = rbtree({'a': 'b'})
        assert r['a']
        try:
            print r['missing']
        except KeyError:
            pass
        else:
            raise KeyError("Able to find missing key")

    def test_iteration(self):
        r = self.sample_data()
        i1 = iter(r)
        i2 = iter(r)
        for word in i1:
            assert i2.next() == word

    def test_axes(self):
        r = rbtree(dict(zip(range(10), range(1, 11))))
        assert r.keys() == range(10)
        assert r.values() == range(1, 11)
        assert r.items() == zip(range(10), range(1, 11))

    def test_slicing(self):
        r = self.sample_data()
        # This should return all matches between b and c in the wordlist
        slice = r['b':'c']
        assert len(slice)
        assert slice.keys()
        del slice
        # Empty slices return empty trees
        r = rbtree()
        assert len(r[0:100]) == 0

    def test_slicing_stepping(self):
        r = rbtree(dict(zip(range(10), range(10))))
        assert r[0:8:2].keys() == [0, 2, 4, 6]
        assert r[1:8:3].keys() == [1, 4, 7]
        # neg slice stride makes no sense with cmp ordered keys
        assert r[0:8:-2].keys() == [0, 2, 4, 6]

    def test_slice_partial(self):
        r = rbtree(dict(zip(range(10), range(10))))
        assert r[8:]

    def test_iter_refcount(self):
        r = rbtree(dict(zip(range(10), range(1, 11))))
        i = iter(r)
        del r
        assert list(i) == range(10)

    def test_pop(self):
        r = rbtree(dict(zip(range(10), range(1, 11))))
        assert len(r) == 10
        # Pops the values, not the keys
        assert r.pop() == 1
        assert r.pop() == 2
        assert len(r) == 8

    def test_random_insert(self):
        import random
        r = rbtree()
        nums = range(1000)
        random.shuffle(nums)
        for i in nums:
            r[i] = True

        assert r.keys() == range(1000)
        assert r[800:810].keys() == range(800, 810)

    def test_contains(self):
        r = rbtree(dict(zip(range(10), range(1, 11))))
        assert 5 in r
        assert 42 not in r

    def test_copy(self):
        r = rbtree(dict(zip(range(10), range(1, 11))))
        c = r.copy()
        assert 5 in r
        assert 5 in c
        del r[5]
        assert 5 not in r
        assert 5 in c

    def test_clear(self):
        r = rbtree(dict(zip(range(10), range(1, 11))))
        assert 5 in r
        r.clear()
        assert len(r) == 0
        assert 5 not in r
        r = r.update(dict(zip(range(10), range(1, 11))))

    def test_setdefault(self):
        r = rbtree()
        r.setdefault('f', 'foo')
        assert r['f'] == 'foo'

    def test_cmp(self):
        reverse = lambda x, y: cmp(x, y) * -1
        r = rbtree(dict(zip(range(10), range(10))),
                          cmp=reverse)
        assert r.keys() == list(reversed(range(10)))

        badcmp = lambda x, y: 'z'
        try:
            r = rbtree(dict(zip(range(10), range(10))),
                              cmp=badcmp)
        except TypeError: pass
        else:
            raise TypeError("Allowed Cmp that returns string?")

    def test_minmax(self):
        r = rbtree(dict(zip(range(10), range(10))))
        assert r.min() == 0
        assert r.max() == 9

    def test_reversed(self):
        # invokes the __reversed__ method
        r = rbtree(dict(zip(range(10), range(10))))
        assert list(reversed(r)) == range(10)[::-1]

    def test_offset(self):
        r = rbtree(dict(zip(range(10), range(10))))
        # get keys by offset
        assert r.byOffset(2) == 2
        assert r.byOffset(1) == 1
        assert r.byOffset(0) == 0
        assert r.byOffset(-1) == 9
        assert r.byOffset(-2) == 8

    def test_stopiteration(self):
        r = rbtree({'a': 'b', 'c': 'd'})
        i = iter(r)
        # iterate the whole set
        list(i)
        # now each call should raise StopIteration to complie with the
        # protocol
        try:
            i.next()
        except StopIteration:
            pass
        else:
            raise NotImplementedError()

        try:
            i.next()
        except StopIteration:
            pass
        else:
            raise NotImplementedError()

    def test_iterdirection(self):
        r = rbtree(dict(zip(range(10), range(10))))
        i = iter(r)
        assert i.next() == 0
        assert i.next() == 1
        assert i.next() == 2
        i.direction = -1
        assert i.next() == 1
        assert i.next() == 0

        i = iter(r)
        i.direction = -1
        assert i.next() == 9

    def test_itergoto(self):
        r = rbtree(dict(zip(range(10), range(10))))
        i = iter(r)
        i.goto(5)
        assert i.next() == 6
        assert i.next() == 7

        # Just to key 5 and walk backwards getting the items tuple
        i2 = r.iteritems()
        i2.goto(5)
        i2.direction = -1
        assert i2.next() == (4, 4)

        # Now goto a missing key
        try:
            i2.goto('missing')
        except KeyError:
            pass
        else:
            raise KeyError("Found Missing key in iter")

    def test_replace(self):
        r = rbtree()
        r[1] = 2
        assert r[1] == 2
        r[1] = 3
        assert r[1] == 3
        assert len(r) == 1

    def test_multikey_compare(self):
        # use a cmp function that allows for key matches
        def multicmp(x, y):
            x = cmp(x, y)
            if x == 0: return 1
            return x
        r = rbtree(cmp=multicmp)
        r[1] = 2
        r[1] = 3
        assert len(r) == 2
        assert 2 in r.values()
        assert 3 in r.values()

        i = iter(r)
        i.next()
        i.delete()
        assert len(r) == 1

    def test_iterprev(self):
        r = rbtree([(chr(i), True) for i in range(ord('a'),
                                                  ord('z') + 1)])
        i = r.iternodes()
        i.goto('c')
        assert i.key == 'c'
        i.prev()
        assert i.key == 'b'
        i.next()
        assert i.key == 'c'

        # Now we flip the base direction
        i.direction = -1
        i.next()
        assert i.key == 'b'
        i.prev()
        assert i.key == 'c'

    def test_pickle(self):
        import pickle
        r = rbtree([(chr(i), True) for i in range(ord('a'),
                                                  ord('z') + 1)])
        s = pickle.dumps(r, protocol=-1)
        t = pickle.loads(s)
        assert t.keys() == [chr(i) for i in
                            range(ord('a'), ord('z') + 1)]

        # and with a custom cmp function, this has to be module global
        # to work because of pickle
        global ncmp

        def ncmp(a, b):
            return cmp(a.lower(), b.lower())

        r = rbtree({'a': 1, 'A': 2}, ncmp)
        s = pickle.dumps(r, protocol=-1)
        t = pickle.loads(s)
        assert t == r

    def test_slice_endcase(self):
        r = rbtree((('x', 1), ('y', 1), ('z', 1)))
        assert r['y':].keys() == ['y', 'z']

    def test_delslice(self):
        r = rbtree([(chr(i), True) for i in range(ord('a'),
                                                  ord('z') + 1)])
        del r['a':'x']
        assert r.keys() == ['x', 'y', 'z']
        del r['y':]
        assert r.keys() == ['x']

    def test_itermode(self):
        r = rbtree([(chr(i), True) for i in range(ord('a'),
                                                  ord('z') + 1)])
        it = r.iternodes()
        for n in it:
            n.get(KEYS)
            n.get(VALUES)
            n.get(ITEMS)
            n.get(NODES)

    def test_invalid_sequence(self):
        self.assertRaises(ValueError, rbtree, [['a', 'b', 'c'], ['c', 'd']])

    def test_rbset(self):
        a = rbset(range(10))
        b = rbset(('a', 'b', 'c', 3, 4, 5))
        assert set((3,4,5)) == set(a & b)
        assert set(range(10) + ['a', 'b', 'c']) == set(a | b)
        assert set([0, 1, 2, 6, 7, 8, 9, 'a', 'b', 'c']) == set(a^b)

        # inplace operators
        a |= b
        a -= b
        assert set(a) == set([0, 1, 2, 6, 7, 8, 9])


if __name__ == "__main__":
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(Test))
    ##suite.addTest(doctest.DocFileSuite('README.txt'))
    runner = unittest.TextTestRunner(verbosity=1)
    runner.run(suite)

