#include <stdio.h>
#include <ctype.h>
#include <sys/wait.h>
#include "smerge.h"
#include "defs.h"

#define TRUE	1
#define FALSE	0

static char	*statenames[] = {
	"none",
	"old",
	"new",
	"nobase",
	"ambiguous",
	"sccserror",
	(char *)NULL
};

char *
statename(one, two)
{
	static char	name[40];

	if( one == ERRSTATE || two == ERRSTATE )
		return(statenames[ERRSTATE]);

	if( one > NEW )
		return(statenames[one]);

	sprintf(name, "%s.%s", statenames[one], statenames[two]);
	return(name);
}

int depth = 0;

int
defaultaction()
{
	register STATEBLOCK	sb;
	char	line[80];
	char	outline[MAXPATHLEN+80];

	if( (sb = findstate("default")) != (STATEBLOCK)0 )
		return(runstate(sb));

	(void)sprintf(line, "$(%s).$(%s)\t$(%s)/$(%s)",
		VARSTATE1, VARSTATE2, VARDIR, VARFILE);
	subst(line, outline);
	(void)puts(outline);
	return(0);
}

int
runstate(sb)
	STATEBLOCK	sb;
{
	NAMEBLOCK	others;
	int		status = 0;

	if( depth > 50 )
		fatal("looping");
	++depth;

	for( others = sb->othercommands; others; others = others->nextname )
	{
		register STATEBLOCK	othersb;

		if( (othersb = findstate(others->namep)) == (STATEBLOCK)0 )
			status = defaultaction();
		else
			status = runstate(othersb);
	}

	return( status != 0 ? status : docom(sb->commands) );
}

int
decision(state1, state2)
{
	STATEBLOCK	sb;
	int		status;

	setvar(VARSTATE1, statenames[state1]);
	setvar(VARSTATE2, statenames[state2]);

	depth = 0;
	if( (sb = findstate(statename(state1,state2))) == (STATEBLOCK)0 )
		status = defaultaction();
	else
		status = runstate(sb);
	return(status);
}

char *
skipwhite(s)
	char	*s;
{
	while( *s && isspace(*s) )
		++s;
	return(s);
}
