##
## @file Makefile
## Makefile for VCS TCP/IP bulletin board exercise.
##
## In the course "Verteilte Computer Systeme" the students shall
## implement a bulletin board. It shall consist of a spawning TCP/IP
## server which executes the business logic provided by the lector,
## and a suitable TCP/IP client.
##
## This is the Makefile for the reference implementation.
##
## @author Thomas M. Galla <galla@technikum-wien.at>
## @date 2014/08/22
##

##
## ------------------------------------------------------------- variables --
##

INSTALL_BASE_DIR := /usr/local
INSTALL_BIN_DIR := $(INSTALL_BASE_DIR)/bin
INSTALL_MAN_DIR := $(INSTALL_BASE_DIR)/man/man1
INSTALL_ETC_DIR := $(INSTALL_BASE_DIR)/etc
INSTALL_LIB_DIR := $(INSTALL_BASE_DIR)/lib
INSTALL_SRC_DIR := $(INSTALL_BASE_DIR)/src
INSTALL_HDR_DIR := $(INSTALL_BASE_DIR)/include
INSTALL_DOC_DIR := /var/www/html

EXESUFFIX :=
CC := gcc
CP := cp
CD := cd
MV := mv
GREP := grep
DOXYGEN := doxygen
AR := ar
INSTALL := install
ZIP := zip
TAR := tar
PUSHD := pushd
POPD := popd
LN := ln -s

# -O3 will enable further warnings like warnings w.r.t. use of
# uninitialized variables
CFLAGSBASE := -DDEBUG -Wall -Wextra -Wstrict-prototypes -fno-common -g -O3
CFLAGS90 := $(CFLAGSBASE) -pedantic
CFLAGS11 := -pedantic -std=gnu11 $(CFLAGSBASE)
LFLAGS := -L .

EXCLUDE_PATTERN := footrulewidth

##
## ----------------------------------------------------------------- rules --
##

%.o : %.c
	$(CC) $(CFLAGS) -o $@ -c $<

##
## =================================================================== eof ==
##
