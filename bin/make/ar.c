#ident "@(#)ar.c 1.1 92/07/30 Copyright 1986,1987,1988 Sun Micro"

/*
 *	ar.c
 *
 *	Deal with the lib.a(member.o) and lib.a((entry-point)) notations
 *
 * look inside archives for notations a(b) and a((b))
 *	a(b)	is file member   b  in archive a
 *	a((b))	is entry point   b  in object archive a
 *
 * For 6.0, create a make which can understand all archive
 * formats.  This is kind of tricky, and <ar.h> isnt any help.
 */


/*
 * Included files
 */
#include "defs.h"
#include <ar.h>
#include <fcntl.h>
#include <ranlib.h>
#include <errno.h>

/*
 * Defined macros
 */
#ifndef S5EMUL
#undef BITSPERBYTE
#define BITSPERBYTE	8
#endif

/*
 * Defines for all the different archive formats.  See next comment
 * block for justification for not using <ar.h>s versions.
 */
#define AR_5_MAGIC		"<ar>"		/* 5.0 format magic string */
#define AR_5_MAGIC_LENGTH	4		/* 5.0 format string length */

#define AR_PORT_MAGIC		"!<arch>\n"	/* Port. (6.0) magic string */
#define AR_PORT_MAGIC_LENGTH	8		/* Port. (6.0) string length */
#define AR_PORT_END_MAGIC	"`\n"		/* Port. (6.0) end of header */

/*
 * typedefs & structs
 */
/*
 * These are the archive file headers for the formats.  Note
 * that it really doesnt matter if these structures are defined
 * here.  They are correct as of the respective archive format
 * releases.  If the archive format is changed, then since backwards
 * compatability is the desired behavior, a new structure is added
 * to the list.
 */
typedef struct {	/* 5.0 ar header format: vax family; 3b family */
	char			ar_magic[AR_5_MAGIC_LENGTH];	/* AR_5_MAGIC*/
	char			ar_name[16];	/* Space terminated */
	char			ar_date[4];	/* sgetl() accessed */
	char			ar_syms[4];	/* sgetl() accessed */
}			Arh_5;

typedef struct {	/* 5.0 ar symbol format: vax family; 3b family */
	char			sym_name[8];	/* Space terminated */
	char			sym_ptr[4];	/* sgetl() accessed */
}			Ars_5;

typedef struct {	/* 5.0 ar member format: vax family; 3b family */
	char			arf_name[16];	/* Space terminated */
	char			arf_date[4];	/* sgetl() accessed */
	char			arf_uid[4];	/* sgetl() accessed */
	char			arf_gid[4];	/* sgetl() accessed */
	char			arf_mode[4];	/* sgetl() accessed */
	char			arf_size[4];	/* sgetl() accessed */
}			Arf_5;

typedef struct {	/* Portable (6.0) ar format: vax family; 3b family */
	char			ar_name[16];	/* Space terminated */
	/* left-adjusted fields; decimal ascii; blank filled */
	char			ar_date[12];	
	char			ar_uid[6];
	char			ar_gid[6];
	char			ar_mode[8];	/* octal ascii */
	char			ar_size[10];
	/* special end-of-header string (AR_PORT_END_MAGIC) */
	char			ar_fmag[2];	
}			Ar_port;

typedef struct {
	FILE			*fd;
	/* to distiguish ar format */
	enum {
		ar_5,
		ar_port
	}			type;
	/* where first ar member header is at */
	long			first_ar_mem;
	/* where the symbol lookup starts */
	long			sym_begin;
	/* the number of symbols available */
	long			num_symbols;
	/* length of symbol directory file */
	long			sym_size;
	Arh_5			arh_5;
	Ars_5			ars_5;
	Arf_5			arf_5;
	Ar_port			ar_port;
}			Ar;

/*
 * Static variables
 */

/*
 * File table of contents
 */
extern	time_t		read_archive();
extern	Boolean		open_archive();
extern	void		close_archive();
extern	void		read_archive_dir();
extern	void		translate_entry();
extern	long		sgetl();

/*
 *	read_archive(target)
 *
 *	Read the contents of an ar file.
 *
 *	Return value:
 *				The time the member was created
 *
 *	Parameters:
 *		target		The member to find time for
 *
 *	Global variables used:
 *		empty_name 	The Name ""
 */
time_t
read_archive(target)
	register Name           target;
{
	register Property       member;
	Ar                      ar;
	register Name		true_member = NULL;
	String_rec		true_member_name;
	char			buffer[STRING_BUFFER_LENGTH];
	char			*slash;

	member = get_prop(target->prop, member_prop);
	/* Check if the member has directory component */
	/* If so remove the dir and see if we know the date */
	if ((member->body.member.member != NULL) &&
	    (slash = strrchr(member->body.member.member->string,
			     (int) slash_char)) != NULL) {
		INIT_STRING_FROM_STACK(true_member_name, buffer);
		append_string(member->body.member.library->string,
			      &true_member_name,
			      FIND_LENGTH);
		append_char((int) parenleft_char, &true_member_name);
		append_string(slash+1, &true_member_name, FIND_LENGTH);
		append_char((int) parenright_char, &true_member_name);
		true_member = GETNAME(true_member_name.buffer.start,
				      FIND_LENGTH);
		if (true_member->stat.time != (int) file_no_time) {
			target->stat.time = true_member->stat.time;
			return target->stat.time;
		}
	}
	if (open_archive(member->body.member.library->string, &ar) == failed) {
		if (errno == ENOENT) {
			target->stat.errno = ENOENT;
			close_archive(&ar);
			if (member->body.member.member == NULL) {
				member->body.member.member = empty_name;
			}
			return target->stat.time = (int) file_doesnt_exist;
		} else {
			fatal("Can't access archive `%s': %s",
			      member->body.member.library->string,
			      errmsg(errno));
		}
	}
	if (target->stat.time == (int) file_no_time) {
		read_archive_dir(&ar, member->body.member.library);
	}
	if (member->body.member.entry != NULL) {
		translate_entry(&ar, target, member);
	}
	close_archive(&ar);
	if (true_member != NULL) {
		target->stat.time = true_member->stat.time;
	}
	if (target->stat.time == (int) file_no_time) {
		target->stat.time = (int) file_doesnt_exist;
	}
	return target->stat.time;
}

/*
 *	open_archive(filename, arp)
 *
 *	Return value:
 *				Indicates if open failed or not
 *
 *	Parameters:
 *		filename	The name of the archive we need to read
 *		arp		Pointer to ar file description block
 *
 *	Global variables used:
 */
static Boolean
open_archive(filename, arp)
	char			*filename;
	register Ar		*arp;
{
	char			mag_5[AR_5_MAGIC_LENGTH];
	char			mag_port[AR_PORT_MAGIC_LENGTH];
	char			buffer[4];
	int			fd;

	arp->fd = NULL;
	fd = open_vroot(filename, O_RDONLY, 0, NULL, VROOT_DEFAULT);
	if ((fd < 0) || ((arp->fd = fdopen(fd, "r")) == NULL)) {
		return failed;
	}
	(void) fcntl(fileno(arp->fd), F_SETFD, 1);

	/* Read enough of the archive to distinguish between the formats */
	if (fread(mag_5, AR_5_MAGIC_LENGTH, 1, arp->fd) != 1) {
		return failed;
	}
	if (IS_EQUALN(mag_5, AR_5_MAGIC, AR_5_MAGIC_LENGTH)) {
		arp->type = ar_5;
		/* Must read in header to set necessary info */
		if (fseek(arp->fd, 0L, 0) != 0 ||
		    fread((char *) &arp->arh_5, sizeof (Arh_5), 1, arp->fd) !=
									1) {
			return failed;
		}
		arp->sym_begin = ftell(arp->fd);
		arp->num_symbols = sgetl(arp->arh_5.ar_syms);
		arp->first_ar_mem = arp->sym_begin +
					sizeof (Ars_5) * arp->num_symbols;
		arp->sym_size = 0L;
		return succeeded;
	}
	if (fseek(arp->fd, 0L, 0) != 0 ||
	    fread(mag_port, AR_PORT_MAGIC_LENGTH, 1, arp->fd) != 1) {
		return failed;
	}
	if (IS_EQUALN(mag_port, AR_PORT_MAGIC, AR_PORT_MAGIC_LENGTH)) {
		arp->type = ar_port;
		/* Read in first member header to find out if there is a dir */
		if ((fread((char *) &arp->ar_port,
			   sizeof (Ar_port),
			   1,
			   arp->fd) != 1) ||
		    !IS_EQUALN(AR_PORT_END_MAGIC,
			       arp->ar_port.ar_fmag,
			       sizeof arp->ar_port.ar_fmag)) {
			return failed;
		}
		if (IS_EQUALN(arp->ar_port.ar_name, "__.SYMDEF       ", 16)) {
			if (sscanf(arp->ar_port.ar_size,
				   "%ld",
				   &arp->sym_size) != 1) {
				return failed;
			}
			arp->sym_size += (arp->sym_size & 1);	/* round up */
			if (fread(buffer, sizeof buffer, 1, arp->fd) != 1) {
				return failed;
			}
			arp->num_symbols = sgetl(buffer) /
							sizeof (struct ranlib);
			arp->sym_begin = ftell(arp->fd);
			arp->first_ar_mem = arp->sym_begin +
						arp->sym_size - sizeof buffer;
		} else {
			/* there is no symbol directory */
			arp->sym_size = arp->num_symbols = arp->sym_begin = 0L;
			arp->first_ar_mem = ftell(arp->fd) - sizeof (Ar_port);
		}
		return succeeded;
	}
	fatal("`%s' is not an archive", filename);
	/* NOTREACHED */
}

/*
 *	close_archive(arp)
 *
 *	Parameters:
 *		arp		Pointer to ar file description block
 *
 *	Global variables used:
 */
static void
close_archive(arp)
	register Ar		*arp;
{
	if (arp->fd != NULL) {
		(void) fclose(arp->fd);
	}
}

/*
 *	read_archive_dir(arp, library)
 *
 *	Reads the directory of an archive and enters all
 *	the members into the make symboltable in lib(member) format
 *	with their dates.
 *
 *	Parameters:
 *		arp		Pointer to ar file description block
 *		library		Name of lib to enter members for.
 *				Used to form "lib(member)" string.
 *
 *	Global variables used:
 */
static void
read_archive_dir(arp, library)
	register Ar		*arp;
	Name			library;
{
	char			*name_string;
	long			ptr;
	long			date;
	register long		len;
	char			*member_string;
	Property		member;
	register Name		name;
	register char		*p;
	register char		*q;

	name_string = (char *) alloca((int) (library->hash.length +
						(int) ar_member_name_len * 2));
	(void) strncpy(name_string,
		       library->string,
		       (int) library->hash.length);
	member_string = name_string + library->hash.length;
	*member_string++ = (int) parenleft_char;

	if (fseek(arp->fd, arp->first_ar_mem, 0) != 0) {
		goto read_error;
	}
	/* Read the directory using the appropriate format */
	switch (arp->type) {
	case ar_5:
	    for (;;) {
		if (fread((char *) &arp->arf_5, sizeof arp->arf_5, 1, arp->fd)
		    != 1) {
			if (feof(arp->fd)) {
				return;
			}
			break;
		}
		len = sizeof arp->arf_5.arf_name;
		for (p = member_string, q = arp->arf_5.arf_name;
		     (len > 0) && (*q != (int) nul_char) && !isspace(*q);
		     ) {
			*p++ = *q++;
		}
		*p++ = (int) parenright_char;
		*p = (int) nul_char;
		name = GETNAME(name_string, FIND_LENGTH);
		name->stat.time = sgetl(arp->arf_5.arf_date);
		name->is_member = library->is_member;
		member = maybe_append_prop(name, member_prop);
		member->body.member.library = library;
		*--p = (int) nul_char;
		if (member->body.member.member == NULL) {
			member->body.member.member =
			  GETNAME(member_string, FIND_LENGTH);
		}
		ptr = sgetl(arp->arf_5.arf_size);
		ptr += (ptr & 1);
		if (fseek(arp->fd, ptr, 1) != 0) {
			goto read_error;
		}
	    }
	    break;
	case ar_port:
	    for (;;) {
		if ((fread((char *) &arp->ar_port,
			   sizeof arp->ar_port,
			   1,
			   arp->fd) != 1) ||
		    !IS_EQUALN(arp->ar_port.ar_fmag,
			       AR_PORT_END_MAGIC,
			       sizeof arp->ar_port.ar_fmag)) {
			if (feof(arp->fd)) {
				return;
			}
			break;
		}
		len = sizeof arp->ar_port.ar_name;
		for (p = member_string, q = arp->ar_port.ar_name;
		     (len > 0) &&
		     (*q != (int) nul_char) &&
		     !isspace(*q) &&
		     (*q != (int) slash_char);
		     ) {
			*p++ = *q++;
		}
		*p++ = (int) parenright_char;
		*p = (int) nul_char;
		name = GETNAME(name_string, FIND_LENGTH);
		name->is_member = library->is_member;
		member = maybe_append_prop(name, member_prop);
		member->body.member.library = library;
		*--p = (int) nul_char;
		if (member->body.member.member == NULL) {
			member->body.member.member =
			  GETNAME(member_string, FIND_LENGTH);
		}
		if (sscanf(arp->ar_port.ar_date, "%ld", &date) != 1) {
			fatal("Bad date field for member `%s' in archive `%s'",
			      name_string,
			      library->string);
		}
		name->stat.time = date;
		if (sscanf(arp->ar_port.ar_size, "%ld", &ptr) != 1) {
			fatal("Bad size field for member `%s' in archive `%s'",
			      name_string,
			      library->string);
		}
		ptr += (ptr & 1);
		if (fseek(arp->fd, ptr, 1) != 0) {
			goto read_error;
		}
	    }
	    break;
	}

	/* Only here if fread() [or IS_EQUALN()] failed and not at EOF */
read_error:
	fatal("Read error in archive `%s': %s",
	      library->string,
	      errmsg(errno));
	/* NOTREACHED */
}

/*
 *	translate_entry(arp, target, member)
 *
 *	Finds the member for one lib.a((entry))
 *
 *	Parameters:
 *		arp		Pointer to ar file description block
 *		target		Target to find member name for
 *		member		Property to fill in with info
 *
 *	Global variables used:
 */
static void
translate_entry(arp, target, member)
	register Ar		*arp;
	Name			target;
	register Property	member;
{
	register int		i;
	register int		len;
	int			strtablen;
	int			date;
	char			member_string[sizeof arp->arf_5.arf_name + 1];
	struct ranlib		*offs;
	char			*syms;		 /* string table */
	struct ranlib		*offend;	 /* end of offsets table */
	register char		*ap;
	register char		*hp;


	if (arp->sym_begin == 0L || arp->num_symbols == 0L) {
		fatal("Cannot find symbol `%s' in archive `%s'",
		      member->body.member.entry->string,
		      member->body.member.library->string);
	}

	if (fseek(arp->fd, arp->sym_begin, 0) != 0) {
		goto read_error;
	}

	switch (arp->type) {
	case ar_5:
		if ((len = member->body.member.entry->hash.length) > 8) {
			len = 8;
		}
		for (i = 0; i < arp->num_symbols; i++) {
			if (fread((char *) &arp->ars_5,
				  sizeof arp->ars_5,
				  1,
				  arp->fd) != 1) {
				goto read_error;
			}
			if (IS_EQUALN(arp->ars_5.sym_name,
				      member->body.member.entry->string,
				      len)) {
				if ((fseek(arp->fd,
					  sgetl(arp->ars_5.sym_ptr),
					  0) != 0) ||
				    (fread((char *) &arp->arf_5,
					   sizeof arp->arf_5,
					   1,
					   arp->fd) != 1)) {
					goto read_error;
				}
				(void) strncpy(member_string,
					       arp->arf_5.arf_name,
					       sizeof arp->arf_5.arf_name);
				member_string[sizeof arp->arf_5.arf_name] =
								(int) nul_char;
				member->body.member.member =
					GETNAME(member_string, FIND_LENGTH);
				target->stat.time = sgetl(arp->arf_5.arf_date);
				return;
			}
		}
		break;
	case ar_port:
/*
 *	Format of the symbol directory for this format is as follows:
 *		[sputl()d number_of_symbols * sizeof (struct ranlib)]
 *		[sputl()d first_symbol_offset]
 *		[sputl()d first_string_table_offset]
 *			...
 *		[sputl()d number_of_symbols_offset]
 *		[sputl()d last_string_table_offset]
 *		[null_terminated_string_table_of_symbols]
 */
		offs = (struct ranlib *) alloca((int) (arp->num_symbols *
						 sizeof (struct ranlib)));
		if (fread((char *) offs,
			  sizeof (struct ranlib),
			  (int) arp->num_symbols,
			  arp->fd) != arp->num_symbols) {
			goto read_error;
		}
		if (fread((char *) &strtablen, sizeof (int), 1, arp->fd) != 1){
			goto read_error;
		}
		syms = alloca(strtablen);
		if (fread(syms,
			  sizeof (char),
			  strtablen,
			  arp->fd) != strtablen) {
			goto read_error;
		}
		offend = &offs[arp->num_symbols];
		while (offs < offend) {
			if (IS_EQUALN(&syms[offs->ran_un.ran_strx],
				      member->body.member.entry->string,
				      (int)member->body.member.entry->
				      hash.length)) {
				if (fseek(arp->fd,
					  (long) offs->ran_off,
					  0) != 0) {
					goto read_error;
				}
				if ((fread((char *) &arp->ar_port,
					   sizeof arp->ar_port,
					   1,
					   arp->fd) != 1) ||
				    !IS_EQUALN(arp->ar_port.ar_fmag,
					       AR_PORT_END_MAGIC,
					       sizeof arp->ar_port.ar_fmag)) {
					goto read_error;
				}
				if (sscanf(arp->ar_port.ar_date,
					   "%ld",
					   &date) != 1) {
					fatal("Bad date field for member `%s' in archive `%s'",
					      arp->ar_port.ar_name,
					      target->string);
				}
				ap = member_string;
				hp = arp->ar_port.ar_name;
				while (*hp &&
				       (*hp != (int) slash_char) &&
				       (ap < &member_string[sizeof
							    member_string])) {
					*ap++ = *hp++;
				}
				*ap = (int) nul_char;
				member->body.member.member =
					GETNAME(member_string, FIND_LENGTH);
				target->stat.time = date;
				return;
			}
			offs += sizeof (struct ranlib);
		}
	}
	fatal("Cannot find symbol `%s' in archive `%s'",
	      member->body.member.entry->string,
	      member->body.member.library->string);
	/*NOTREACHED*/

    read_error:
	if (ferror(arp->fd)) {
		fatal("Read error in archive `%s': %s",
		      member->body.member.library->string,
		      errmsg(errno));
	} else {
		fatal("Read error in archive `%s': Premature EOF",
		      member->body.member.library->string);
	}
}

/*
 *	sgetl(buffer)
 *
 *	The intent here is to provide a means to make the value of
 *	bytes in an io-buffer correspond to the value of a long
 *	in the memory while doing the io a long at a time.
 *	Files written and read in this way are machine-independent.
 *
 *	Return value:
 *				Long int read from buffer
 *	Parameters:
 *		buffer		buffer we need to read long int from
 *
 *	Global variables used:
 */
static long
sgetl(buffer)
	register char		*buffer;
{
	register long		w = 0;
	register int		i = BITSPERBYTE * sizeof (long);

	while ((i -= BITSPERBYTE) >= 0) {
		w |= (long) ((unsigned char) *buffer++) << i;
	}
	return w;
}

