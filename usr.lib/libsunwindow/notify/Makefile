#
# @(#)Makefile 1.1 92/07/30 SMI;
#
WHOAMI=	notify
#HDRSPUBLIC=notify.h
#HDRSPRIVATE= ndet.h nint.h ntfy.h ndis.h
CFILESLIB=\
	ndet_auto.c	ndet_g_itimer.c	ndet_p_event.c	ndet_s_out.c \
	ndet_event.c	ndet_g_out.c	ndet_read.c	ndet_s_pri.c \
	ndet_fcntl.c	ndet_g_pri.c	ndet_remove.c	ndet_s_sig.c \
	ndet_fd.c	ndet_g_sig.c	ndet_s_death.c	ndet_s_wait.c \
	ndet_g_death.c	ndet_g_wait.c	ndet_s_event.c	ndet_select.c \
	ndet_g_event.c	ndet_get_func.c	ndet_s_except.c	ndet_sig.c \
	ndet_g_except.c	ndet_itimer.c	ndet_s_fd.c	ndet_value.c \
	ndet_g_fd.c	ndet_loop.c	ndet_s_in.c	ndet_wait.c \
	ndet_g_in.c	ndet_p_death.c	ndet_s_itimer.c ndet_death.c \
	ndet_die.c	ndet_veto.c	ndet_do_dis.c	ndet_no_dis.c \
	ndet_single.c \
	ndis_d_pri.c	ndis_d_sched.c	ndis_dispatch.c	ndis_g_sched.c \
	ndis_s_sched.c	ndis_d_wait.c \
	ntfy_client.c	ntfy_cond.c	ntfy_debug.c	ntfy_list.c \
	ntfy_node.c	ntfy_protect.c	ntfy_perror.c	ntfy_dump.c \
	sys_select.c	sys_read.c	sys_fcntl.c \
	nint_copy.c	nint_i_sig.c	nint_n_out.c	nint_r_itimer.c \
	nint_get.c	nint_i_wait.c	nint_n_sig.c	nint_r_out.c \
	nint_i_death.c	nint_inter.c	nint_n_wait.c	nint_r_sig.c \
	nint_i_event.c	nint_n_death.c	nint_next.c	nint_r_wait.c \
	nint_i_except.c	nint_n_event.c	nint_r_death.c	nint_remove.c \
	nint_i_fd.c	nint_n_except.c	nint_r_event.c	nint_set.c \
	nint_i_in.c	nint_n_fd.c	nint_r_except.c	nint_stack.c \
	nint_i_itimer.c	nint_n_in.c	nint_r_fd.c \
	nint_i_out.c	nint_n_itimer.c	nint_r_in.c	ntfy_fd_op.c \
	notify_data.c
SPCFILESLIB=\
	ndet_auto.c	ndet_loop.c	ndet_read.c	ndet_select.c \
	ndis_dispatch.c	notify_data.c	nint_stack.c 	ntfy_client.c \
	ntfy_protect.c	ntfy_node.c
NO_XSTR_FILES= notify_data.c

OBJS = $(CFILESLIB:%.c=$(VARIANT)/%.o)
TAGFILESALL= $(CFILESLIB:%=$(FULLPATH)/%)

all : xall
#include common stuff from ../Makefile.master
include ../Makefile.master

#	Can change ndis_default_scheduler in ndis_dispatch.c

$(SPCFILESLIB:%.c=obj/%.o) \
		$(SPCFILESLIB:%.c=obj_p/%.o) \
		$(SPCFILESLIB:%.c=shared/%.o)  :=  ROI_DATA =

$(NO_XSTR_FILES:%.c=obj/%.o) \
		$(NO_XSTR_FILES:%.c=obj_p/%.o) \
		$(NO_XSTR_FILES:%.c=shared/%.o)  :=  NO_XSTR = ON

xall : $$(LIBS)
$(LIBS) : $$(VARIANT) $(HDRSPUBLIC) $(HDRSPRIVATE) $$(OBJS)

clean: master.clean
