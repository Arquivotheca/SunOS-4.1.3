README for creating a miniroot from tape
1. use "format" to format and/or label the disk where the miniroot
	is to be created (not needed if disk already formatted/labeled)
2. rewind and skip forward to miniroot image - "tape=st0"
	example# mt -f /dev/nr${tape} rewind
	example# mt -f /dev/nr${tape} fsf  XXX
	  where XXX is 2 for sun4c and 4 for all other machines
3. copy the miniroot image onto disk - "disk=sd0" or sd1, etc.
	example# dd if=/dev/nr{$tape} of=/dev/r${disk}b bs=100k
4. when the shell prompts (with "#"), enter:
	reboot 'sd(0,<UNIT#>,1) -sw'
  where "<UNIT#>" is the unit number, ie 0 for sd0, 1 for sd1, etc.
  if that fails, do an L1-A or power cycle to reboot.
