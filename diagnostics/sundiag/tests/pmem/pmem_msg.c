/*
 * @(#)pmem_msg.c - Rev 1.1 - 7/30/92
 *
 * pmem_msg.c:  message file for pmem.c.
 *
 */

char *test_usage_msg   = "[s={size}m]";
char *kvmopen_err_msg  = "kvm_open() failed: %s.";
char *kvmnlist_err_msg = "kvm_nlist() failed: %s.";
char *namelist_err_msg = "Defective namelist in '/vmunix'.";
char *read_err_msg     = "Couldn't read value of %s in '/vmunix': %s.";
char *no_memory_msg    = "Found no physical memory.";
char *test_msg         = "Testing 0x%x (%1.0f Meg) bytes of memory.";
char *memory_size_msg  = "Physical memory = 0x%x (%1.0f Meg) bytes.";
char *read_end_msg     = "Read 0x%x bytes (%d pages) starting at address 0x%x.";
char *open_err_msg     = "Couldn't open file /dev/mem: %s.";
char *valloc_err_msg   = "'valloc' results incorrect: %s.";
char *memlist_err_msg  = "Couln't read physical memory list: %s.";
char *found_err_msg    = "Found no physical memory.";
char *pmlist_err_msg   = "Couln't read physical memory address and size: %s.";
char *read_mem_msg     = "Reading physical memory at address 0x%x.\n";
char *mmap_err_msg     = "Bad 'mmap' results: %s, page = 0x%x/%d, addr = 0x%x.";
char *size_err_msg     = "The specified memory size (%1.1f Meg) is not the\
 same as the size found (%1.1f Meg).";
char *routine_msg      = "%s specific arguments [defaults]:\n\
	s={size}m  = memory size to test in 1m blocks [auto size]\n\n";
