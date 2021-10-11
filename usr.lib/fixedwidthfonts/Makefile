#
# @(#)Makefile 1.1 92/07/30 SMI
#

DIR=$(DESTDIR)/usr/lib/fonts/fixedwidthfonts

OWNER=	bin

FONTS=	README \
	apl.r.10 \
	cmr.b.14 cmr.r.14 \
	cour.b.10 cour.b.12 cour.b.14 cour.b.16 cour.b.18 cour.b.24 \
	cour.r.10 cour.r.12 cour.r.14 cour.r.16 cour.r.18 cour.r.24 \
	gallant.r.19 \
	screen.b.12 screen.b.14 screen.b.16 \
	screen.r.6 screen.r.7 screen.r.11 screen.r.12 screen.r.14 screen.r.16 \
	serif.r.10 serif.r.11 serif.r.12 serif.r.14 serif.r.16

# filename,linkname
LINKS=	cmr.b.14,cmr.b.8 \
	cmr.r.14,cmr.r.8 \
	gallant.r.19,gallant.r.10 \
	screen.b.12,gacha.b.7 \
	screen.b.14,gacha.b.8 \
	screen.r.7,sail.r.6 \
	screen.r.12,gacha.r.7 \
	screen.r.14,gacha.r.8 \
	screen.r.14,screen.r.13

all: $(FONTS)

clean:
	$(RM) $(FONTS)

install: $(FONTS)
	-install -d -o $(OWNER) -m 755 $(DIR)
	install -o $(OWNER) -m 644 $(FONTS) $(DIR)
	@cd $(DIR); \
	for i in $(LINKS); do \
		set `echo $$i | tr ',' ' '` ; \
		echo "ln -s $$1 $$2"; \
		$(RM) $$2; ln -s $$1 $$2; \
	done
