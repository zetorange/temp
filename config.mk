DESTDIR ?=
PREFIX ?= /usr

CATEYES := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

CATEYES_COMMON_FLAGS := --buildtype minsize --strip
CATEYES_DIET_FLAGS := -Denable_diet=auto
CATEYES_MAPPER_FLAGS := -Denable_mapper=auto

CATEYES_OPTIMIZATION_FLAGS ?= -Os
CATEYES_DEBUG_FLAGS ?= -g3

CATEYES_ASAN ?= no

PYTHON ?= $(shell which python)
PYTHON_VERSION := $(shell $(PYTHON) -c 'import sys; v = sys.version_info; print("{0}.{1}".format(v[0], v[1]))')
PYTHON_NAME ?= python$(PYTHON_VERSION)

PYTHON3 ?= python3

NODE ?= $(shell which node)
NODE_BIN_DIR := $(shell dirname $(NODE) 2>/dev/null)
NPM ?= $(NODE_BIN_DIR)/npm

MESON ?= $(PYTHON3) $(CATEYES)/releng/meson/meson.py
NINJA ?= $(CATEYES)/releng/ninja-$(build_platform_arch)

tests ?=
