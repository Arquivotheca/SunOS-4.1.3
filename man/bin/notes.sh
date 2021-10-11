#! /bin/sh
# %Z%%M% %I% %E% SMI;
# update /usr/src/man/arch/notes.file.

notesfile=notes.file
cd /usr/src/man/arch

# check it out (dude...)
sccs edit -s $notesfile

# cat instructions to stdout
cat - <<'EOT'

Please enter the information in this format:

oldname newname classification description

classification is one of "add", "arch", "rename", and ".so".
For "arch" entries, newname should be of the form "arch/filename".
For "add" entries, oldname should be "none".
For ".so" entries, description should specify the file pointed to.

Separate entries with RETURN.
Type CTRL-D to terminate input.

EOT

# append date and new entries to the file
date >> $notesfile
cat - >> $notesfile

# check it in (dude...) with no comment
sccs delget -s -y $notesfile
