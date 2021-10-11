/*      @(#)errmsg.h 1.1 92/07/30 SMI      */

/*
* Copyright (c) 1990 by Sun Microsystems, Inc.
*/



/*  4 */
#define DLXERR_OPEN	"Unable to open %s. Check device for existence and/or permission.\n"
#define DLXERR_OPEN_DEVICE	"Unable to open %s. Check device for existence and/or permission: "
/* 31 */
#define DLXERR_OPEN_MEM_PIXRECT	"Can't open memory pixrect. Suspect incomplete or incorrect software installation.\n"
/* 32 */
#define DLXERR_PIXEL_MODE_AT	"Pixel Access Mode error at x=%d y=%d, bank=%d, exp=0x%x,obs=0x%x, xor=0x%x. Suspect faulty HFB.\n"

#define DLXERR_BYTE_MODE_AT	"Byte/Stencil Access Mode error at x=%d y=%d, exp=0x%x,obs=0x%x, xor=0x%x. Suspect faulty HFB.\n"
/* 33 */
#define DLXERR_BUFF_A	"Direct Port Buffer A: "
/* 34 */
#define DLXERR_BUFF_B	"Direct Port Buffer B: "
/* 35 */
#define DLXERR_SIMULT_WRITE_BOTH_READ_A	"Direct port simultaneous write to both buffers, read from buffer A: "
/* 36 */
#define DLXERR_SIMULT_WRITE_BOTH_READ_B	"Direct port simultaneous write to both buffers, read from buffer B: "
/* 37 */
#define DLXERR_PIXRECT	"Pixrect error: "
/* 38 */
#define DLXERR_PIXRECT_VISUAL	"Pixrect Visual test: "
/* 39 */
#define DLXERR_LOOKUP_TABLE	"Look up table error at index %d, write %x, read %x. Suspect faulty HFB.\n"

/* for compatibility reasons new errors are assigned a number greater
 * than 51
 */

/* 52 */
#define DLXERR_HDL_EXEC		"Failed with error code %d: %s\n"
/* 53 */
#define DLXERR_OPEN_HDL_FILE	"Can't open display list file %s. Suspect incomplete or incorrect software installation. Files may also have been corrupted because file system ran out of space in /tmp. Appr. 1.5MB in /tmp is required.\n"
/* 54 */
#define DLXERR_READ_HDL_FILE	"Can't read diplay list file %s. Suspect incomplete or incorrect software installation. Files may also have been corrupted because file system ran out of space in /tmp. Appr. 1.5MB in /tmp is required.\n"
/* 55 */
#define DLXERR_CONNECT		"Communication with Graphics Engine failed. Suspect incomplete or incorrect software installation. Front-End Firmware may also be dead.\n"
/* 56 */
#define DLXERR_SYSTEM_INITIALIZATION	"System initialization failed. Suspect incomplete or incorrect software installation. Front-End Firmware may also be dead.\n"
/* 57 */
#define DLXERR_ENUMERATING_WINDOWS	"problem enumerating windows. Suspect incomplete or incorrect software installation.\n"
/* 58 */
#define DLXERR_GETTING_FRAME_BUFFER_TYPE	"problem getting frame buffer type. Suspect incomplete or incorrect software installation.\n"
/* 59 */
#define DLXERR_FOUND_IN	"Error(s) found in "
/* 60 */
#define DLXERR_RED	"RED "
/* 61 */
#define DLXERR_GREEN	"GREEN "
/* 62 */
#define DLXERR_BLUE	"BLUE "
/* 63 */
#define DLXERR_COMPONENTS	"component(s).\n"
/* 64 */
#define DLXERR_TAR	"tar: %s\n"
/* 65 */
#define DLXERR_VFORK	"vfork: %s\n"
/* 66 */
#define DLXERR_TAR_NEVER_FINISHED	"'tar' never finished. System software problem.\n"
/* 67 */
#define DLXERR_REDUCE_RED	"Can't reduce result image, RED component.  Suspect incomplete or incorrect software installation.\n"
/* 68 */
#define DLXERR_REDUCE_GREEN	"Can't reduce result image, GREEN component.  Suspect incomplete or incorrect software installation.\n"
/* 69 */
#define DLXERR_REDUCE_BLUE	"Can't reduce result image, BLUE component.  Suspect incomplete or incorrect software installation.\n"
/* 70 */
#define DLXERR_FORKING		"Failed to fork a process. System error.\n"
/* 71 */
#define DLXERR_BACKGRND_PROCCESS_HANGS	"Background process wouldn't die. System error.\n"
/* 72 */
#define DLXERR_INTEGRATION_TEST_FAILED	"failed with %d error(s).\n"
/* 73 */
#define DLXERR_CLUT_TEST	"%s CLUT #%d: %s"
/* 74 */
#define DLXERR_HDL_FILE_TOO_BIG "Display list file is too big for the remaining VM! hdl_size=0x%X, vm_size=0x%X\n"
/* 75 */
#define DLXERR_UNKNOWN_CTX_ATTRIBUTE "Unknown Context Attribute!.\n"
#define DLXERR_OPEN_FILE_TO_DUMP "Can't open %s to dump frame buffer fordebug switch.\n"
#define DLXERR_UNIQUE_WID "Failed to allocate unique WID for 24-bit plane.  Suspect incomplete or incorrect software installation.\n"
#define DLXERR_ACCELERATOR_PORT_DRAWING_TO_A "Arbitration Test: Accelerator port drawing to buffer A, "
#define DLXERR_ACCELERATOR_PORT_DRAWING_TO_B "Arbitration Test: Accelerator port drawing to buffer B, "
#define DLXERR_ACCELERATOR_PORT_DRAWING_TO_DB "Arbitration Test: Accelerator port drawing in double buffer mode, "
#define DLXERR_ARBITRATION_FAILED "Arbitration test failed.\n"
#define DLXERR_VIDEO_MEMORY_FAILED "Direct Port Video Memory test failed as indicated in the last %d message(s).\n"
#define DLXERR_FE_TIMEOUT "Front End (Firmware) not responding. This may indicate that the Firmware has died. Try to run gtconfig.\n"
#define DLXERR_NEVER_FINISHED "Front End (Firmware) not responding. This may indicate that the Firmware has died. Try to run gtconfig.\n"
#define DLXERR_USER_TEST_ABORTED "The execution of display list is aborted.\n"
#define DLXERR_HK_OPEN_FAILED "hk_open failed. GT system is either not initialized or not connected.\n"
#define DLXERR_HK_DISCONNECT_FAILED "hk_disconnect failed. System software error.\n"
#define DLXERR_HK_MUNMAP_FAILED "hk_munmap failed. System software error.\n"
#define DLXERR_ERROR_CONDITION "HGPFE firmware has detected an error in the display list.  Error code = %d : %s\n"
#define DLXERR_UNKNOWN_TRAP_INSTRUCTION "Got XCPU interrupt, but user_mcb_ptr->trap_instruction = 0x%X, expect 0x%X. System software error.\n"
#define DLXERR_NOT_TRAP_INSTRUCTION "Got XCPU interrupt, but it's not a trap instruction, error code = %d : %s. This may indicate that the Firmware has died. Try to run gtconfig.\n"
#define DLXERR_ERR_INSTRUCTION "At FE firmware program counter 0x%x, expected display list instruction 0x%x, observed 0x%x.\n"
#define DLXERR_DPC_OUT_OF_RANGE "The HGPFE pgrogram counter value 0x%x is out of VM range.  The HGPFE firmware maybe fetching instruction from bogus memory area.\n"
#define DLXERR_NOT_IMPLEMENTED "Test not implemented.\n"
#define DLXERR_LDM_FAILED "LDM error: %s Suspect faulty HGPFE.\n"
#define DLXERR_SU_SHARED_RAM "SU Shared RAM error: %s Suspect faulty HGPRP.\n"
#define DLXERR_RENDERING_PIPELINE "Rendering Pipeline error: %s Suspect faulty HGPRP.\n"
#define DLXERR_VIDEO_MEM_I860_IMAGE_A "Image plane A error: %s Suspect faulty HFB.\n"
#define DLXERR_VIDEO_MEM_I860_IMAGE_B "Image plane B error: %s Suspect faulty HFB.\n"
#define DLXERR_VIDEO_MEM_I860_DEPTH "Depth plane error: %s Suspect faulty HFB.\n"
#define DLXERR_VIDEO_MEM_I860_WID "WID plane error: %s Suspect faulty HFB.\n"
#define DLXERR_VIDEO_MEM_I860_CURSOR "Cursor plane error: %s Suspect faulty HFB.\n"
#define DLXERR_VIDEO_MEM_I860_FCS_A "Fast Clear Plane A error: %s Suspect faulty HFB.\n"
#define DLXERR_VIDEO_MEM_I860_FCS_B "Fast Clear Plane B error: %s Suspect faulty HFB.\n"
#define DLXERR_PICK_MISSES "Pick Detect misses: %d lines and/or %d triangles inside the pickbox and/or %d lines and triangles outside the pickbox.\n"
#define DLXERR_PICK_ECHO "Pick Echo failed: %s"
#define DLXERR_FB_GETMONITOR "Failed to get monitor mode: %s. Software error.\n"
#define DLXERR_FB_SETMONITOR "Failed to set monitor mode to %d : %s. Software error.\n"
#define DLXERR_FB_SETDIAGMODE "Failed to set diagnostic mode: %s. Software error.\n"
#define DLXERR_WLUT "WLUT Test: %s Suspect faulty HFB.\n"
#define DLXERR_CLUT "CLUT Test: %s Suspect faulty HFB.\n"
#define MEMORY_ERROR "Memory error."
#define HARDWARE_TIMEOUT "Hardware timeout."
#define CHECKSUMS_DONT_MATCH "Checksums don't match."
#define UNKNOWN_ERROR "Unknown error."
#define GTTEST_DATA_MISSING "Data file %s missing in the current test directory and in /usr/diag/sundiag.\n"
#define DLXERR_CLUT_ALLOC "Error allocating Color LUTs.\n"
#define DLXERR_OPEN_REGIONS "Fail to open subpixrect.\n"
#define DLXERR_WID_FREE "Error freeing WID. Suspect incomplete or incorrect software installation.\n"
#define DLXERR_CLUT_FREE "Error freeing CLUT. Suspect incomplete or incorrect software installation.\n"
#define DLXERR_GET_WPART "Error getting WPART.\n"
#define DLXERR_SET_WPART "Error setting WPART.\n"
#define DLXERR_LIGHTPEN_ENABLE "Failed to enable the lightpen hardware. Lightpen support may not be present or the hardware may be completely unusable.\n"
#define DLXERR_IOCTL_FB_LIGHTPENENABLE "Ioctl call FB_LIGHTPENENABLE failed: "
#define DLXERR_IOCTL_FB_GETLIGHTPENPARAM "Ioctl call FB_GETLIGHTPENPARAM failed: "
#define DLXERR_IOCTL_FB_SETLIGHTPENPARAM "Ioctl call FB_SETLIGHTPENPARAM failed: "
#define DLXERR_LIGHTPEN_EVENT "Error reading lightpen event.\n"
#define DLXERR_OPEN_FONT "Unable to open font file %s. Please verify existence and/or permission: "
#define DLXERR_F_SETFL_NONBLOCKING "Error setting non-blocking read: "
#define DLXERR_TAR_KILLED "Tar died due to signal %d.\n"
#define DLXERR_TAR_ERROR "TAR failed. Note: A space of appr. 1.5MB in /tmp is required for the test to run correctly.\n"
#define DLXERR_LIGHTPEN_CALIBRATION "Lightpen calibration ioctl failed.\n"
#define GRAB_POINTER_FAILED "XGrabPointer failed: %s.  Software error.\n"
#define GRAB_KEYBOARD_FAILED "XGrabKeyboard failed: %s.  Software error.\n"
#define ALREADY_GRABBED "already grabbed by another client.\n"
#define GRAB_INVALID_TIME "grabbed at invalid time.\n"
#define GRAB_NOTVIEWABLE "grab window is not viewable.\n"
#define GRAB_FROZEN "pointer/keyboard is frozen.\n"
#define USER_ABORT_MESSAGE "Ctrl-C detected. Test aborted.\n"
#define NO_WINDOW "Unable to open display.  No window running.\n"
