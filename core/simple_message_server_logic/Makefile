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

-include global.mak
-include ../global.mak

##
## ------------------------------------------------------------- variables --
##

PACKAGE := simple_message_server_logic

ARCHIVE_SOURCES := \
	bin2c.c \
	doxygen.dcf \
	simple_message_server_logic.1 \
	simple_message_server_logic.c \
	ok.png \
	error.png \
	vcs_tcpip_bulletin_board.php \
	content_entry_with_img.thtml \
	vcs_tcpip_bulletin_board_response_error.thtml \
	content_entry_without_img.thtml \
	vcs_tcpip_bulletin_board_response_ok.thtml \
	Makefile \
	global.mak \
	README.txt

GEN_FILES_TEXT := \
	vcs_tcpip_bulletin_board.php.h \
	vcs_tcpip_bulletin_board_response_error.thtml.h \
	vcs_tcpip_bulletin_board_response_ok.thtml.h \
	content_entry_with_img.thtml.h \
	content_entry_without_img.thtml.h

GEN_FILES_BIN := \
	ok.png.h \
	error.png.h

OBJECTS_SERVER_LOGIC := \
	simple_message_server_logic.o

OBJECTS_BIN2C := \
	bin2c.o

OBJECTS := \
	$(OBJECTS_BIN2C) \
	$(OBJECTS_SERVER_LOGIC)

SYMLINKS := \
	global.mak

GLOBAL_MAK := \
	$(wildcard ../global.mak)

ARCHIVES := $(foreach EXT,zip tar.gz,$(PACKAGE).$(EXT))

EXECUTABLES := \
	bin2c$(EXESUFFIX) \
	simple_message_server_logic$(EXESUFFIX)

MANPAGES := \
	simple_message_server_logic.1

CFLAGS := $(CFLAGS11)
LFLAGS :=

##
## --------------------------------------------------------------- targets --
##

all: symlinks exes archs html

symlinks: $(SYMLINKS)

exes: $(EXECUTABLES)

archs: $(ARCHIVES)

bin2c$(EXESUFFIX): $(OBJECTS_BIN2C)
	$(CC) $(LFLAGS) -o $@ $^

simple_message_server_logic$(EXESUFFIX): $(OBJECTS_SERVER_LOGIC)
	$(CC) $(LFLAGS) -o $@ $^

$(GEN_FILES_BIN):
	./bin2c -c $* $@

$(GEN_FILES_TEXT):
	./bin2c -c -z $* $@

global.mak: $(GLOBAL_MAK)
	$(LN) $< $@

clean:
	$(RM) $(OBJECTS) $(GEN_FILES_BIN) $(GEN_FILES_TEXT) *~

clobber: clean
	$(RM) $(EXECUTABLES) $(ARCHIVES)

distclean: clobber
	$(RM) -r doc $(SYMLINKS)

$(PACKAGE).zip: $(ARCHIVE_SOURCES)
	$(CD) ..; $(ZIP) -r $(PACKAGE)/$@ $(foreach src,$^,$(PACKAGE)/$(src))

$(PACKAGE).tar.gz: $(ARCHIVE_SOURCES)
	$(CD) ..; $(TAR) -czhf $(PACKAGE)/$@ $(foreach src,$^,$(PACKAGE)/$(src))

doc: html pdf

html: doc/html/index.html

doc/html/index.html doc/latex/refman.tex: doxygen.dcf
	$(DOXYGEN) doxygen.dcf

pdf: html doc/latex/refman.pdf
doc/latex/refman.pdf: doc/latex/refman.tex
	$(CD) doc/latex; \
	$(MV) refman.tex refman_save.tex; \
	$(GREP) -v $(EXCLUDE_PATTERN) refman_save.tex > refman.tex; \
	$(RM) refman_save.tex; \
	make; \
	$(MV) refman.pdf refman.save; \
	$(RM) *.pdf *.html *.tex *.aux *.sty *.log *.eps *.out *.ind *.idx \
	      *.ilg *.toc *.tps *.ttf Makefile; \
	$(MV) refman.save refman.pdf

install: exes archs $(MANPAGES)
	for i in $(EXECUTABLES); do $(INSTALL) -sD $$i $(INSTALL_BIN_DIR)/$$i; done
	for i in $(ARCHIVES); do $(INSTALL) -D -m 0644 $$i $(INSTALL_SRC_DIR)/$$i; done
	for i in $(MANPAGES); do $(INSTALL) -D -m 0644 $$i $(INSTALL_MAN_DIR)/$$i; done

##
## ---------------------------------------------------------- dependencies --
##

simple_message_server_logic.o: simple_message_server_logic.c $(GEN_FILES_TEXT) $(GEN_FILES_BIN)
vcs_tcpip_bulletin_board.php.h: vcs_tcpip_bulletin_board.php bin2c$(EXESUFFIX)
vcs_tcpip_bulletin_board_response_error.thtml.h: vcs_tcpip_bulletin_board_response_error.thtml bin2c$(EXESUFFIX)
vcs_tcpip_bulletin_board_response_ok.thtml.h: vcs_tcpip_bulletin_board_response_ok.thtml bin2c$(EXESUFFIX)
content_entry_with_img.thtml.h: content_entry_with_img.thtml bin2c$(EXESUFFIX)
content_entry_without_img.thtml.h: content_entry_without_img.thtml bin2c$(EXESUFFIX)
ok.png.h: ok.png bin2c$(EXESUFFIX)
error.png.h: error.png bin2c$(EXESUFFIX)

##
## =================================================================== eof ==
##
