INSTALL = install
CFLAGS = -Wall -g
LFLAGS =
OBJS = packnam.o

prefix = /usr/local
datarootdir = $(prefix)/share
datadir = $(datarootdir)
exec_prefix = $(prefix)
bindir = $(exec_prefix)/bin
infodir = $(datarootdir)/info
mandir = $(datarootdir)/man
docbookxsldir ?=

# --- Auto-detect a suitable man base dir (prefer under $(prefix)) ---
# Try: manpath entries under $(prefix) -> any manpath entry -> fallback to $(mandir)
MANBASE := $(shell manpath 2>/dev/null | tr ':' '\n' | grep -E '^$(prefix)(/|$$)' | head -n1)
ifeq ($(strip $(MANBASE)),)
  MANBASE := $(shell manpath -q 2>/dev/null | tr ':' '\n' | head -n1)
endif
ifeq ($(strip $(MANBASE)),)
  MANBASE := $(mandir)
endif

MAN1DIR := $(MANBASE)/man1

packnam: $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o packnam

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

install: packnam
	@echo "Installing binary to $(bindir)"
	@echo "Installing man(1) to $(MAN1DIR)"
	$(INSTALL) -d -m 0755 $(bindir) $(MAN1DIR)
	$(INSTALL) -m 0755 packnam $(bindir)/packnam
	# 0644 is conventional for man pages
	$(INSTALL) -m 0644 packnam.1 $(MAN1DIR)/packnam.1
	# Compress if gzip is available (both Linux/macOS man read .gz)
	- gzip -f $(MAN1DIR)/packnam.1 >/dev/null 2>&1 || true
	# Refresh whatis DB if supported (Linux/BSD/macOS variants)
	- mandb -q >/dev/null 2>&1 || \
	  makewhatis $(MANBASE) >/dev/null 2>&1 || \
	  /usr/libexec/makewhatis $(MANBASE) >/dev/null 2>&1 || true

uninstall:
	-rm -f $(bindir)/packnam
	-rm -f $(MAN1DIR)/packnam.1 $(MAN1DIR)/packnam.1.gz

doc: packnam-refentry.docbook
	@echo "Generating man + HTML docs..."
	@XSLDIR="$(docbookxsldir)"; \
	if [ -z "$$XSLDIR" ]; then \
	  if command -v brew >/dev/null 2>&1; then \
	    PFX="$$(brew --prefix docbook-xsl 2>/dev/null)"; \
	  else \
	    PFX=""; \
	  fi; \
	  for d in \
	    "$$PFX/docbook-xsl" \
	    "$$PFX/docbook-xsl-nons" \
	    "$$PFX/share/xsl/docbook-xsl" \
	    "$$PFX/share/xsl/docbook-xsl-nons" \
	    /usr/share/xml/docbook/stylesheet/docbook-xsl-nons \
	    /usr/share/xml/docbook/stylesheet/docbook-xsl \
	    /opt/local/share/xsl/docbook-xsl-nons \
	    /opt/local/share/xsl/docbook-xsl \
	    /usr/share/sgml/docbook/xsl-stylesheets \
	    /sw/share/xml/xsl/docbook-xsl ; do \
	    if [ -f "$$d/manpages/docbook.xsl" ]; then \
	      XSLDIR="$$d"; \
	      break; \
	    fi; \
	  done; \
	fi; \
	if [ -z "$$XSLDIR" ]; then \
	  echo "Error: DocBook XSL stylesheets not found."; \
	  echo "Try: make docbookxsldir=\"$$PFX/docbook-xsl\" doc"; \
	  exit 1; \
	fi; \
	mkdir -p doc; \
	xsltproc "$$XSLDIR/manpages/docbook.xsl" $<; \
	xsltproc "$$XSLDIR/html/docbook.xsl" $< > doc/index.html; \
	echo "Documentation generated."

clean:
	rm -f $(OBJS) packnam packnam.exe

.PHONY: clean install uninstall doc
