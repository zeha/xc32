## Process this file with automake to generate Makefile.in

AUTOMAKE_OPTIONS = cygnus

noinst_PROGRAMS = conv

conv_SOURCES =  convert.c elfio.c elfout.c mipsflash.c \
		srec.c lsifast.c idtbin.c stagbin.c bin.c \
		elf.h elfmips.h sysdep.h \
		conv.1

#EXTRA_conv_SOURCES = getopt.h # alloca.c getopt.c strtoul.c

LIBIBERTY = ../libiberty/libiberty.a

conv_LDADD = $(LIBIBERTY)

man_MANS = conv.1

info_TEXINFOS = conv.texi

conv_TEXI = $(srcdir)/conv.texi

conv.html: $(conv_TEXI) 
	@rm -f conv.html
	texi2html -monolithic -glossary -menu -I . -I $(srcdir) $(conv_TEXI) 

conv.dvi:  $(conv_TEXI) 

conv.info: $(conv_TEXI) 

.PHONY: install-exec-local

install-exec-local: $(noinst_PROGRAMS)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(bindir)
	@list='$(noinst_PROGRAMS)'; for p in $$list; do \
	  if test -f $$p; then \
	    t="$(DESTDIR)$(bindir)/`echo $$p|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`"; \
	    l="$(DESTDIR)$(bindir)/$$p"; \
	    echo "  $(INSTALL_PROGRAM) $$p $$t"; \
	     $(INSTALL_PROGRAM) $$p $$t; \
	    if [ "$$l" != "$$t" ]; then \
	      rm -f $$l; \
	      echo "  ln $$t $$l"; \
	       ln $$t $$l >/dev/null 2>/dev/null \
		|| $(INSTALL_PROGRAM) $$p $$l; \
	    else :; fi; \
	  else :; fi; \
	done
