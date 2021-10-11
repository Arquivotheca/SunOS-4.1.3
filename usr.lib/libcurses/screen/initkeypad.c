#ifndef lint
static	char sccsid[] = "@(#)initkeypad.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

#ifdef	 	KEYPAD
static struct map *_addone();
/*
 * Make up the needed array of map structures for dealing with the keypad.
 */
#define MAXKEYS 50	/* number of keys we understand */

struct map *
_init_keypad()
{
	struct map *r, *p;

	r = (struct map *) calloc(MAXKEYS, sizeof (struct map));
	p = r;

	/* If down arrow key sends \n, don't map it. */
	if (key_down && strcmp(key_down, "\n"))
		p = _addone(p, key_down,	KEY_DOWN,	"down");
	p = _addone(p, key_up,		KEY_UP,		"up");
	/* If left arrow key sends \b, don't map it. */
	if (key_left && strcmp(key_left, "\b"))
		p = _addone(p, key_left,	KEY_LEFT,	"left");
	p = _addone(p, key_right,	KEY_RIGHT,	"right");
	p = _addone(p, key_home,	KEY_HOME,	"home");
	/* If backspace key sends \b, don't map it. */
	if (key_backspace && strcmp(key_backspace, "\b"))
		p = _addone(p, key_backspace,	KEY_BACKSPACE,	"backspace");
	p = _addone(p, key_f0,		KEY_F(0),	lab_f0?lab_f0:"f0");
	p = _addone(p, key_f1,		KEY_F(1),	lab_f1?lab_f1:"f1");
	p = _addone(p, key_f2,		KEY_F(2),	lab_f2?lab_f2:"f2");
	p = _addone(p, key_f3,		KEY_F(3),	lab_f3?lab_f3:"f3");
	p = _addone(p, key_f4,		KEY_F(4),	lab_f4?lab_f4:"f4");
	p = _addone(p, key_f5,		KEY_F(5),	lab_f5?lab_f5:"f5");
	p = _addone(p, key_f6,		KEY_F(6),	lab_f6?lab_f6:"f6");
	p = _addone(p, key_f7,		KEY_F(7),	lab_f7?lab_f7:"f7");
	p = _addone(p, key_f8,		KEY_F(8),	lab_f8?lab_f8:"f8");
	p = _addone(p, key_f9,		KEY_F(9),	lab_f9?lab_f9:"f9");
	p = _addone(p, key_dl,		KEY_DL,		"dl");
	p = _addone(p, key_il,		KEY_IL,		"il");
	p = _addone(p, key_dc,		KEY_DC,		"dc");
	p = _addone(p, key_ic,		KEY_IC,		"ic");
	p = _addone(p, key_eic,		KEY_EIC,	"eic");
	p = _addone(p, key_clear,	KEY_CLEAR,	"clear");
	p = _addone(p, key_eos,		KEY_EOS,	"eos");
	p = _addone(p, key_eol,		KEY_EOL,	"eol");
	p = _addone(p, key_sf,		KEY_SF,		"sf");
	p = _addone(p, key_sr,		KEY_SR,		"sr");
	p = _addone(p, key_npage,	KEY_NPAGE,	"npage");
	p = _addone(p, key_ppage,	KEY_PPAGE,	"ppage");
	p = _addone(p, key_stab,	KEY_STAB,	"stab");
	p = _addone(p, key_ctab,	KEY_CTAB,	"ctab");
	p = _addone(p, key_catab,	KEY_CATAB,	"catab");
	p = _addone(p, key_ll,		KEY_LL,		"ll");
	p = _addone(p, key_a1,		KEY_A1,		"a1");
	p = _addone(p, key_a3,		KEY_A3,		"a3");
	p = _addone(p, key_b2,		KEY_B2,		"b2");
	p = _addone(p, key_c1,		KEY_C1,		"c1");
	p = _addone(p, key_c3,		KEY_C3,		"c3");
	p -> keynum = 0;		/* termination convention */
#ifdef DEBUG
	if(outf) fprintf(outf, "return key structure %x, ending at %x\n", r, p);
#endif
	return r;
}

/*
 * Map text into num, updating the map structure p.
 * label is currently unused, but is an English description
 * of what the key is labelled, similar to the name in vi.
 */
static
struct map *
_addone(p, text, num, label)
struct map *p;
char *text;
int num;
char *label;
{
	if (text) {
		strcpy(p->label, label);
		strcpy(p->sends, text);
		p->keynum = num;
#ifdef DEBUG
		if(outf) fprintf(outf, "have key label %s, sends '%s', value %o\n", p->label, p->sends, p->keynum);
#endif
		p++;
	}
	return p;
}

#endif		KEYPAD
