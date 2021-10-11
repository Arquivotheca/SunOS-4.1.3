#
# @(#)Makefile 1.1 92/07/30 SMI; from System III
#
.KEEP_STATE:
COPTS = -O
CFLAGS = $(COPTS)

OBJ1 = chkid.o chksid.o del_ba.o  date_ba.o dodelt.o
OBJ2 = del_ab.o dofile.o dohist.o doie.o dolist.o eqsid.o flushto.o newstats.o
OBJ3 = permiss.o logname.o pf_ab.o date_ab.o rdmod.o setup.o
OBJ4 = sid_ab.o sid_ba.o sidtoser.o sinit.o stats_ab.o
OBJ5 = fmterr.o getline.o putline.o auxf.o encode.o gdsid.o open_sccs_log.o

comobj.a:	$(OBJ1) $(OBJ2) $(OBJ3) $(OBJ4) $(OBJ5)
	-rm -f comobj.a
	ar r comobj.a $(OBJ1) $(OBJ2) $(OBJ3) $(OBJ4) $(OBJ5)
	ranlib comobj.a

clean:
	-rm -f $(OBJ1) $(OBJ2) $(OBJ3) $(OBJ4) $(OBJ5)
