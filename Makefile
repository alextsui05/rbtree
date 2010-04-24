export PYTHON=python
export PYTHONPATH+=$(shell ${PYTHON} ./scripts/testpath.py)

all: clean build test


build:
	@${PYTHON} setup.py build

test:
	@${PYTHON} setup.py test

install:
	@${PYTHON} setup.py install

dist:
	@${PYTHON} setup.py bdist

clean:
	@find . -name "*.pyc" -exec rm {} \;
	@find . -name "*.so" -exec rm {} \;
	@find . -name "*~" -exec rm {} \;
	@rm -rf src/rbtree.egg-info
	@rm -rf build
	@rm -rf dist

tags:
	@ctags -e -R -o TAGS

debug:
	gdb --command=.gdbdebug python

release:
	@$(PYTHON) setup.py sdist
	@$(PYTHON) setup.py bdist_egg
	@echo Release ready, check dist/
