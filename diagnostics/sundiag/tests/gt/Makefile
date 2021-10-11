#
# @(#)Makefile 1.1 92/07/30 SMI
#
#	Makefile to build GT Sundiag
#
#	Usage: make or make all 	* build gttest, this is all that
#					* is required for an OS release.
#
#					* required for new gttest.data build.
#	       make bldhasm		* builds i_*.hasm and extracts 
#					* other *.hasm files
#	       make data		* build new "data" file to replace
#					* gttest.data (do make bldhdl first).
#
#	Note: if changes are require for *.hasm or FE i_*.c
#	      (diag escape microcode) file, make the needed source
#	      file changes and then run the following:
#		% make bldhasm
#		% make data
#
#	      To install a new gttest.data into the SCCS database do:
#		% mv gttest.data gttest.data.old
#		% mv data gttest.data
#		% mv SCCS/s.gttest.data SCCS/s.gttest.data.rel.xxx
#		% sccs create gttest.data

#	      To generate new *.red, *.green, *.blue files, run the
#	      gttest with the "-write" option on a known good system.
#	      New RGB files will be stored in the /tmp directory.
#	      On the software build system do:
#		% tar xvf gttest.data	# tar files to /tmp
#		% cp /net/<testsystem>/tmp/*.red /tmp
#		% cp /net/<testsystem>/tmp/*.green /tmp
#		% cp /net/<testsystem>/tmp/*.blue /tmp
#		% tar cf data /tmp/*.hdl /tmp/*.red /tmp/*.green /tmp/*.blue
# Set environment variable WIN=OL for openwin version,
# otherwise WIN= for sunview
#
#	The following environment variables need to be set before a make 
#	is attempted:
#
#	setenv OPENWINHOME /usr/dist/sun4/openwin,v3.0
#	setenv LD_LIBRARY_PATH $OPENWINHOME/lib:/usr/lib
#

.DEFAULT:
	sccs get -G$@ $@

include ../../include/Makefile.macros
REV=
# specify "REV=" if built for 4.0.X

RM= /bin/rm -f
#MAKE= /usr/bin/make
TAR= /bin/tar
CP= /bin/cp
CPP= /lib/cpp
ED= /bin/ed
INSTALL= /bin/install
LINT= /usr/bin/lint
HASM= /usr/shbam2/sim/release/sun4/hasm
RANLIB= /usr/bin/ranlib
CHMOD= /usr/bin/chmod
ARCH= `arch`

# Openwin include and library defines.
#OL_LIB= -lX11 $(OPENWINHOME)/lib/libdga.so
#OL_INC= -I$(OPENWINHOME)/include
#OL_LIBPATH= -L$(OPENWINHOME)/lib
OL_LIB= 
OL_INC= 
OL_LIBPATH= 

# specify DBX=-g for dbx version
DBX=-O

SUNDIAG_DIR=	/usr/diag/sundiag

DEFINES=

# system include directory
STDINCL= -I.

# used during development to include new or modified include files
#DEVEL_INCLPATH= -I/net/ruble/usr2/hawk/src/usr.lib/libpixrect -I/net/ruble/usr2/hawk/src/sys

# used during development to include new or modified include files
#DEVEL_LIBPATH= -L/net/ruble/usr3/dlx/demo/sv

INCDIR= -I../../include -I../../../lib/include
INCLUDES= $(STDINCL) $(INCDIR) $(OW_INC)

SDLIBS= ../../lib/libtest.a ../../../lib/libonline.a
HK_LIB= libhcom.a
HAWK1_LIBS= $(HK_LIB) -lsunwindow -lpixrect
HK_I860LIB= $(HK_LIB)
LIBS= $(HK_I860LIB) -lsunwindow -lpixrect  $(OL_LIB)

LINT_LIB_PATH= /usr/lib/lint

SOURCES	= 		gttest.c\
			pr_test.c\
			desktop.c\
			subtest_list.c\
			chksum.c\
			msg.c\
			getfname.c\
			fb_video_mem_host.c\
			fb_video_mem_i860.c\
			fb_luts_shawdow_ram.c\
			rp_ew_si_asics.c\
			fb_output_section.c\
			rp_shared_ram.c\
			fe_local_data_mem.c\
			hdl_aa_pgons.c\
			hdl_aa_triangles.c\
			hdl_aa_vectors.c\
			hdl_animation.c\
			hdl_clip.c\
			hdl_depth_cueing.c\
			hdl_hidden.c\
			hdl_lite_shad.c\
			hdl_markers.c\
			hdl_pgon_edge_hili.c\
			hdl_pgons.c\
			hdl_picking.c\
			hdl_splines.c\
			hdl_surf_fill.c\
			hdl_text.c\
			hdl_triangles.c\
			hdl_uti.c\
			hdl_vectors.c\
			hdl_xsparency.c\
			hdl_stereo.c\
			hdl_lightpen.c

OBJECTS	=		$(SOURCES:.c=.o)

ESC_DIAG_HASM=		i_wcs_mem_test.hasm\
			i_rp_shared_ram.hasm\
			i_rendering_pipeline.hasm\
			i_video_mem_i860_image_a.hasm\
			i_video_mem_i860_image_b.hasm\
			i_video_mem_i860_depth.hasm\
			i_video_mem_i860_wid.hasm\
			i_video_mem_i860_cursor.hasm\
			i_video_mem_i860_fcs_a.hasm\
			i_video_mem_i860_fcs_b.hasm\
			i_wlut.hasm\
			i_clut.hasm\
			i_fb_output_section.hasm

INTEGRATION_HASM=	rendering_pipeline.hasm\
			vectors.hasm\
			aa_vectors.hasm\
			wide_vectors.hasm\
			textured_vectors.hasm\
			triangles.hasm\
			aa_triangles.hasm\
			flat_triangles.hasm\
			gouraud_triangles.hasm\
			spline_curves.hasm\
			clipping.hasm\
			hidden_surface_removal.hasm\
			polygon_edge_hilite.hasm\
			transparency.hasm\
			depth_cueing.hasm\
			lighting_shading.hasm\
			lighted_sun_logo.hasm\
			molec1.hasm\
			blender_c.hasm\
			text.hasm\
			pick_detect.hasm\
			picking.hasm\
			arbitration_db.hasm\
			save_fb_mode.hasm\
			restore_fb_mode.hasm\
			stereo.hasm

HDL_FILES=		$(ESC_DIAG_HASM:.hasm=.hdl)\
			$(INTEGRATION_HASM:.hasm=.hdl)

TEST_IMAGES=		$(INTEGRATION_HASM:.hasm=.red)\
			$(INTEGRATION_HASM:.hasm=.green)\
			$(INTEGRATION_HASM:.hasm=.blue)

TEST_DATA= gttest.data
HAWK1_TEST_PROGRAM= gt1test
TEST_PROGRAM= gttest

#Development environment
#TARGETS= $(TEST_PROGRAM) $(HAWK1_TEST_PROGRAM) $(TEST_DATA)
#OS build
TARGETS= $(TEST_PROGRAM) $(TEST_DATA)


CPPFLAGS= -D$(REV) -D$(WIN) $(INCLUDES) $(DEFINES)
CFLAGS	= $(DBX)
LDFLAGS	= $(DBX) $(OL_LIBPATH)
LINT_FLAGS= -abh -D$(REV) $(INCLUDES) $(DEFINES)

LINT_LIBS= $(LINT_LIB_PATH)/llib-lpixrect.ln\
	   $(LINT_LIB_PATH)/llib-lsunwindow.ln\
	   $(LINT_LIB_PATH)/llib-lpixrect.ln


.KEEP_STATE:

##### beginning of dependency lines #####

all:	$(TARGETS)


$(TEST_PROGRAM): $(OBJECTS) $(HK_I860LIB)
	$(CHMOD) +w $(HK_I860LIB)
	$(RANLIB) $(HK_I860LIB)
	$(CHMOD) -w $(HK_I860LIB)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@ $(SDLIBS) $(LIBS)

bldhasm:	
	$(MAKE) -f make-hasm
	sccs get $(INTEGRATION_HASM)

$(HAWK1_TEST_PROGRAM):	$(OBJECTS) $(HK_LIB)
	$(CHMOD) +w $(HK_LIB)
	$(RANLIB) $(HK_LIB)
	$(CHMOD) -w $(HK_LIB)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@ $(SDLIBS) $(HAWK1_LIBS)


# make these only in the development environment:
#$(ESC_DIAG_HASM):	FRC
#	$(MAKE) -f make-hasm $@

# make these only in the development environment:
data:	$(HDL_FILES) gttest.data
	$(TAR) xvf gttest.data
	$(CP) $(HDL_FILES) /tmp
	$(TAR) cf $@ /tmp/*.hdl /tmp/*.red /tmp/*.green /tmp/*.blue
	$(RM) /tmp/*.hdl /tmp/*.red /tmp/*.green /tmp/*.blue

install: all FRC
	@if [ $(DESTDIR) ]; then \
	    set -x; \
            $(INSTALL) -s $(TEST_PROGRAM) $(DESTDIR); \
            $(INSTALL) $(TEST_DATA) $(DESTDIR); \
        else \
            set -x; \
            $(INSTALL) $(TARGETS) ../../bin; \
        fi

clean:
	$(RM) $(TARGETS) $(OBJECTS) $(LINTFILES) core
cleanhasm:
	$(RM) *.out *.lst *.hdl i_*.hasm *.s i_*.o

lint:	$(SOURCES) $(LINT_LIBS)
	$(LINT) $(LINT_FLAGS) $(SOURCES) $(LINT_LIBS)

info:
	sccs info


%.hdl:	%.hasm $(HASM)
	$(HASM) < $< > $@

gttest.o: gttest.h errmsg.h
pr_test.o: gttest.h errmsg.h
desktop.o: errmsg.h
subtest_list.o: gttest.h
chksum.o: gttest.h errmsg.h
getfname.o: gttest.h
fb_video_mem_host.o: gttest.h errmsg.h
fb_luts_shawdow_ram.o: gttest.h errmsg.h
fe_i860.o: gttest.h errmsg.h
fe_local_data_mem.o: gttest.h errmsg.h
integration_test.o: gttest.h errmsg.h
hdl_aa_pgons.o: gttest.h
hdl_aa_triangles.o: gttest.h
hdl_aa_vectors.o: gttest.h
hdl_animation.o: gttest.h errmsg.h
hdl_clip.o: gttest.h
hdl_depth_cueing.o: gttest.h
hdl_hidden.o: gttest.h
hdl_lite_shad.o: gttest.h
hdl_markers.o: gttest.h
hdl_pgon_edge_hili.o: gttest.h
hdl_pgons.o: gttest.h
hdl_picking.o: gttest.h
hdl_splines.o: gttest.h
hdl_surf_fill.o: gttest.h
hdl_text.o: gttest.h
hdl_triangles.o: gttest.h
hdl_uti.o: gtmcb.h hk_public.h hk_comm.h gttest.h errmsg.h
hdl_vectors.o: gttest.h
hdl_xsparency.o: gttest.h
hdl_lightpen.o: gttest.h


$(HDL_FILES): ewsi_cmds.h
FRC:

depend: $(SOURCES)
	$(RM) makedep eddep
	for i in $(SOURCES) ; do \
	    ( $(CPP) -M $(CPPFLAGS) $$i >> makedep ); \
	    echo `basename $$i .c`.h: $$i >> makedep ; \
	done
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	$(CP) Makefile Makefile.bak
	$(ED) - Makefile < eddep
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile
	$(RM) eddep makedep

include ../../include/Makefile.end
	
# DO NOT DELETE THIS LINE -- make depend uses it
# DEPENDENCIES MUST END AT END OF FILE

