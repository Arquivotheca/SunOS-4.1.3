#
#  @(#)Makefile 1.1 92/07/30 SMI
#

NEWPROTO   = /new_proto
OLDPROTO   = /old_proto
SHELLSC    = package protocmp
DIFFS      = proto.diffs
PACK       = package.list

all:	 	$(PACK)	

$(DIFFS):	protocmp
		rm -f $(DIFFS) $(DIFFS).tmp
		protocmp $(OLDPROTO) $(NEWPROTO) > $(DIFFS).tmp
		grep -v "/usr/openwin" $(DIFFS).tmp > $(DIFFS) 
		rm -f $(DIFFS).tmp

$(PACK):	$(DIFFS) package
		rm -f $(PACK)
		package $(DIFFS) $(PACK)

package:	package.sh;	rm -f $@; cp -p $? $@; chmod +x $@

protocmp:	protocmp.sh;	rm -f $@; cp -p $? $@; chmod +x $@

clean:
		rm -f $(DIFFS) $(DIFFS).tmp $(PACK) $(SHELLSC)
