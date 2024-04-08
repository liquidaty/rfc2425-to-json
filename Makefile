#   ----------------------------------------------------------------------
#   This file is part of rfc2425-to-json, a program and library for
#   parsing text input conforming to rfc2425 and related specification(s)
#
#   Copyright (C) 2024 Liquidaty (info@liquidaty.com)
#
#   This program is released under the MIT license
#   ----------------------------------------------------------------------

# Makefile for use with GNU make

THIS_MAKEFILE_DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
THIS_DIR:=$(shell basename "${THIS_MAKEFILE_DIR}")
THIS_MAKEFILE:=$(lastword $(MAKEFILE_LIST))

CONFIGFILE ?= config.mk
$(info Using config file ${CONFIGFILE})
include ${CONFIGFILE}

CC ?= cc
AWK ?= awk
AR ?= ar
RANLIB ?= ranlib
SED ?= sed

CCBN=$(shell basename ${CC})

DEBUG=
ifeq ($(DEBUG),1)
  CFLAGS+=-g -O0
  DBG_SUBDIR+=dbg
else
  CFLAGS+=-O3
  DBG_SUBDIR+=rel
endif

WIN=
ifeq ($(WIN),)
  WIN=0
  ifneq ($(findstring w64,$(CC)),) # e.g. mingw64
    WIN=1
  endif
endif

ifeq ($(WIN),0)
  BUILD_SUBDIR=$(shell uname)/${DBG_SUBDIR}
else
  BUILD_SUBDIR=win/${DBG_SUBDIR}
endif
BUILD_DIR=build/${BUILD_SUBDIR}/${CCBN}

BUILD_LOCAL_TARGETS=rfc2425parser.lex.c rfc2425parser.lex.h rfc2425parser.tab.c rfc2425parser.tab.h rfc2425-to-json
BUILD_TARGETS=$(addprefix ${BUILD_DIR}/,${BUILD_LOCAL_TARGETS})

help:
	@echo "make [DEBUG=1] build|install"

.PHONY: help build install clean clean-all

build: ${BUILD_DIR} ${BUILD_TARGETS}

install: build ${PREFIX}/bin/rfc2425-to-json

${PREFIX}/bin/rfc2425-to-json: ${BUILD_DIR}/rfc2425-to-json
	cp -p $< $@

${BUILD_DIR}:
	mkdir -p ${BUILD_DIR}

clean-all: clean
	rm -f config.mk

clean:
	@rm -rf ${BUILD_DIR}

${BUILD_DIR}/rfc2425parser.lex.c ${BUILD_DIR}/rfc2425parser.lex.h: src/rfc2425parser.l
	cd ${BUILD_DIR} && flex -DYYLMAX=200 -o $$(basename $@) ${THIS_MAKEFILE_DIR}/$<

${BUILD_DIR}/rfc2425parser.tab.c ${BUILD_DIR}/rfc2425parser.tab.h: src/rfc2425parser.y
	bison27 -vd -o $@ $<

${BUILD_DIR}/rfc2425parser.lex.o: ${BUILD_DIR}/rfc2425parser.lex.c
	(cd ${BUILD_DIR} && ${CC} ${CFLAGS} -I${INCLUDEDIR} -I. -I${THIS_MAKEFILE_DIR}/src -o $$(basename $@) -c $$(basename $^))

${BUILD_DIR}/rfc2425-to-json: src/rfc2425-to-json.c ${BUILD_DIR}/rfc2425parser.lex.o ${BUILD_DIR}/rfc2425parser.tab.c
	${CC} ${CFLAGS} -DRFC2425_BUILD_MAIN -I${INCLUDEDIR} -L${LIBDIR} -I${THIS_MAKEFILE_DIR}/src -I${BUILD_DIR} -ljsonwriter -o $@ $^
	@echo Made $@
