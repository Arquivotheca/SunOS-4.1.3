#define META 01
#define TERMINAL 02

static char            funny[128];

void
initfunny()
{
	register char	*s;

	for (s = "#|=^();&<>*?[]:$`'\"\\\n"; *s; ++s)
		funny[*s] |= META;
	for (s = "\n\t :=;{}&>|"; *s; ++s)
		funny[*s] |= TERMINAL;
	funny['\0'] |= TERMINAL;
}

int
is_meta(c)
	char	c;
{
	return( (funny[c] & META) != 0 );
}

int
is_terminal(c)
	char	c;
{
	return( (funny[c] & TERMINAL) != 0 );
}
