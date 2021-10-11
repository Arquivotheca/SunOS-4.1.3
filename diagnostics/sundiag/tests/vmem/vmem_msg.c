/*
 * @(#)vmem_msg.c - Rev 1.1 - 7/30/92
 *
 * vmem_msg.c:  message file for vmem.c.
 *
 */

char *open_err_msg     = "Open(%s) failed: %s";
char *mmap_err_msg     = "mmap() virtual address 0x%x to %s failed: %s";
char *munmap_err_msg   = "munmap(0x%x) failed: %s.\n";
char *close_err_msg    = "close() failed on %s: %s";
char *msync_err_msg    = "msync() failed: %s";
char *kvm_err_msg      = "kvm_%s() failed.\n";
char *root_err_msg     = "Must be root to execute!\n";
char *test_err_msg     = "Test terminated after finding miscompared data.";
char *found_msg        = "Found 0x%x (%d) bytes (%d pages) of virtual memory.";
char *complete_msg     = "Test completed successfully.";
char *swap_err_msg     = "Insufficient virtual memory.  Increase swap space.";
char *valloc_err_msg   = "valloc(0x%x) failed.";
char *testing_msg      = "Allocated and Testing 0x%x (%d) bytes (%d pages).";
char *simulate_msg     = "simulating %d contiguous errors. start address 0x%x";
char *sim_read_msg     = "Reading from memory from address 0x%x to 0x%x.";
char *sim_err_msg      = "Simulating error at address 0x%x.";
char *contig_err_msg   = "only %d contiguous errors are displayed\n";
char *show_err_msg     = "       0x%08x          0x%08x          0x%08x\n";
char *re_mis_msg       = "  Recompare %s: found %d contiguous bytes that\
 miscompared:\n";
char *miscompare_msg   = "FAILURE %d %s: found %d contiguous bytes\
 that miscompared:\n";
char *errheader_msg    = "      Virtual Addr      Observed Pattern    \
 Expected Pattern\n";
char *test_fail_msg    = "Test completed.  Found %d noncontiguous\
 miscompare errors.";
char *recomp_test_msg  = "Recomparing memory (read only, no write) from\
 address 0x%x to 0x%x.";
char *recompare_msg    = "Recompare of page %d found %d miscompare errors\
 instead of the %d found during the first read.";
char *recomp_err_msg   = "Recompare of memory: found %d miscompare errors\
 instead of the %d found during the first read.";
char *max_err_msg      = "Test terminated after finding maximum number (%d)\
 of noncontiguous errors.";
char *page_done_msg    = "Page %d completed.  Page %d started at virtual\
 address 0x%x.";
char *test_msg         = "Virtual memory test:\n  Testing virtual memory from\
 address 0x%x to 0x%x.\n  Will write and read memory 1 page at a time for %d\
 pages.";
char *memtest_msg      = "Virtual memory test:\n  Testing virtual memory from\
 address 0x%x to 0x%x.\n  Will write to entire range before checking.";
char *test_usage_msg   = "[U] [M=pat_mod] [m=margin] [R=reserve] [page] \n\
 [cerrs=#_of_contiguous_errors] [nerrs=#_of_noncontiguous_errors]\n";
char *routine1_msg     = "%s specific arguments [defaults]:\n\
 U:            test description.\n\
 M=pat_mod:    pattern modifier (put in MSB of pattern to write)\n\
 m=margin:     Mbytes of memory to reserve for OS instead of default.\n\
 R=reserve:    Mbytes of memory to reserve in addition to default.\n\
 page:         write/read memory 1 page at a time.\n";
char *routine2_msg     = " cerrs=number_of_contiguous_errors:\n\
       simulate a number of contiguous errors.\n\
 nerrs=number_of_noncontiguous_errors:\n\
       simulate a number of noncontigous errors.\n";
char *routine3_msg     = "\nNotes:\n\
    vmem saves a default amount of virtual memory for the OS, but the\n\
    m=margin argument will override the default and the R argument.\n\
    The test pattern is simply the memory location's virtual address\n\
    modified by the M=pat_mod argument,  which is or'ed into the MSB of\n\
    the virtual address.\n";
char *describe1_msg     = "Test description:\n\
    Vmem is the UNIX virtual memory test.  It is designed to isolate virtual\n\
    memory errors in the system under various stress levels.  Vmem probes\n\
    the kernel for the amount of virtual memory available.";
char *describe2_msg     = "  After reserving \n\
    some memory for other UNIX processes, vmem will allocate the rest for\n\
    testing.  The amount of virtual memory in the system depends on how\n\
    much swap space you have in the system.\n";
char *describe3_msg     = "\n\
    By default, vmem will write to the entire range of memory being tested,\n\
    and then go back to the starting address and begin reading and\n\
    comparing data.  If the run_on_error flag is not set (the default),\n";
char *describe4_msg     = "\
    then any miscompare errors will be displayed and the checking is\n\
    terminated.  The range will then be re-read (no write) and the data\n\
    will be displayed again.  If the run_on_error flag is set, then any\n";
char *describe5_msg     = "\
    miscompare errors found will be recorded in an array and displayed\n\
    after the entire range is checked, then the entire range will be\n\
    re-read (no write) and the miscompared data will be displayed again.\n";
char *describe6_msg     = "\
    If the \"page\" flag is used, then vmem will write and read 1 page at\n\
    a time.  The page is mmap()'ed to the file /tmp/vmem.page and flushed\n\
    out to disk after the write and will be paged-in again when read.\n";
char *describe7_msg     = "\
    In page mode, if the run_on_error flag is set, then any miscompare\n\
    errors found will be recorded in an array and displayed after each\n\
    page is checked.\n";
