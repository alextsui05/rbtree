import ez_setup
ez_setup.use_setuptools()

from setuptools import setup, find_packages
from setuptools.extension import Extension

setup(
    name="rbtree",
    version="0.8.6",
    packages=find_packages('src', exclude=["*.tests"]),
    package_dir={'': 'src'},
    package_data={'': ['*.txt'], },
    ext_modules=[Extension("rbtree", ["src/rbtree_impl.c",
                                      "src/rbtree.pyx"],
                            libraries=[],
                            include_dirs=['./src', ])
                  ],
    install_requires={'pyrex': '>= 0.9.8.5'},
    zip_safe=False,
    author='Benjamin Saller',
    author_email='bcsaller@gmail.com',
    description="""A red black tree with extended iterator support.""",
    license='GPL 3',
    keywords="rbtree red-black tree data-structure",
    )

