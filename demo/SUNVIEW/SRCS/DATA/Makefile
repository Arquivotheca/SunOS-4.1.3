# @(#)Makefile 1.1 92/07/30 SMI

DESTDIR	=

OWNER	= bin
INSTALL = install -o $(OWNER)
INSDIR	= $(DESTDIR)/usr/demo/SUNVIEW/SRCS/DATA
MAKEFILE= Makefile
README	= README

FILES	= $(FRAMES) $(GPDATA) $(MAPS) $(PIX) $(SHADED)

# for cframedemo
FRAMES= frame.0 frame.1 frame.2 frame.3 frame.4 frame.5 frame.6 frame.7 \
	frame.8 frame.9 frame.10 frame.11 frame.12 frame.13 frame.14 \
	frame.15 frame.16 frame.17 frame.18 frame.19 frame.20 frame.21 \
	frame.22 frame.23 frame.24 frame.25 frame.26 frame.27 frame.28 \
	frame.29 frame.30

# for rotobj??
GPDATA= mtxs.rotobj

# for showmap
MAPS=	map.1 map.2 map.3 map.4 map.5 map.6 map.7 map.8 map.9 map.10

# for show etc.
PIX=	colorimage.1 colorimage.2

# for shaded
SHADED= bottle.dat egg.dat glass.dat icosa.dat pyramid.dat \
	socbal.dat space.dat

all: $(FILES) $(MAKEFILE) $(README)

install: all FRC
	$(INSTALL) -d -m 755 $(INSDIR)
	$(INSTALL) -m 644 $(FILES) $(MAKEFILE) $(README) $(INSDIR)

clean:

FRC:
