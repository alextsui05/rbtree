import sys
try:
    from ez_setup import use_setuptools
    use_setuptools()
except ImportError:
    pass


from setuptools import setup, find_packages
from setuptools.extension import Extension
from Cython.Build import cythonize

extensions = [
        Extension("rbtree", ["src/rbtree_impl.c",
            "src/rbtree.pyx"],
            libraries=[],
            include_dirs=['./src', ])
]
setup(
    name="rbtree",
    version="0.9.1",
    packages=find_packages('src', exclude=["*.tests", "ez_setup.py"]),
    package_dir={'': 'src'},
    package_data={'': ['*.txt'], },
    ext_modules=cythonize(extensions),
    test_suite="tests.test_rbtree",
    zip_safe=False,
    include_package_data=True,
    author='Benjamin Saller',
    author_email='bcsaller@gmail.com',
    description="""A red black tree with extended iterator
    support.""",
    download_url="http://bitbucket.org/bcsaller/rbtree/",
    url="http://bitbucket.org/bcsaller/rbtree/",
    license='GPL 3',
    keywords="rbtree red-black tree data-structure",
    )

#setup(
#    name="rbtree",
#    version="0.9.0",
#    packages=find_packages('src', exclude=["*.tests", "ez_setup.py"]),
#    package_dir={'': 'src'},
#    package_data={'': ['*.txt'], },
#    ext_modules=[Extension("rbtree", ["src/rbtree_impl.c",
#                                      "src/rbtree.pyx"],
#                           libraries=[],
#                           include_dirs=['./src', ])
#                  ],
#    test_suite="tests.test_rbtree",
#    zip_safe=False,
#    include_package_data=True,
#    author='Benjamin Saller',
#    author_email='bcsaller@gmail.com',
#    description="""A red black tree with extended iterator
#    support.""",
#    download_url="http://bitbucket.org/bcsaller/rbtree/",
#    url="http://bitbucket.org/bcsaller/rbtree/",
#    license='GPL 3',
#    keywords="rbtree red-black tree data-structure",
#    )

