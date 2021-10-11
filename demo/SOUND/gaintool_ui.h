#ifndef	gaintool_HEADER
#define	gaintool_HEADER

/*
 * gaintool_ui.h - User interface object and function declarations.
 */
#ifndef lint
/*
      "@(#)gaintool_ui.h 1.1 92/07/30 Copyr 1989 Sun Micro";
*/
#endif

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

extern Attr_attribute	INSTANCE;

extern Xv_opaque	gaintool_props_menu_create();

typedef struct {
	Xv_opaque	BaseFrame;
	Xv_opaque	GainPanel;
	Xv_opaque	PlaySlider;
	Xv_opaque	RecordSlider;
	Xv_opaque	MonitorSlider;
	Xv_opaque	OutputSwitch;
	Xv_opaque	Pause_button;
} gaintool_BaseFrame_objects;

extern gaintool_BaseFrame_objects	*gaintool_BaseFrame_objects_initialize();

extern Xv_opaque	gaintool_BaseFrame_BaseFrame_create();
extern Xv_opaque	gaintool_BaseFrame_GainPanel_create();
extern Xv_opaque	gaintool_BaseFrame_PlaySlider_create();
extern Xv_opaque	gaintool_BaseFrame_RecordSlider_create();
extern Xv_opaque	gaintool_BaseFrame_MonitorSlider_create();
extern Xv_opaque	gaintool_BaseFrame_OutputSwitch_create();
extern Xv_opaque	gaintool_BaseFrame_Pause_button_create();

typedef struct {
	Xv_opaque	StatusPanel;
	Xv_opaque	PlayStatusPanel;
	Xv_opaque	Ppanel_label;
	Xv_opaque	Popen_flag;
	Xv_opaque	Ppaused_flag;
	Xv_opaque	Pactive_flag;
	Xv_opaque	Perror_flag;
	Xv_opaque	Pwaiting_flag;
	Xv_opaque	Peof_label;
	Xv_opaque	Peof_value;
	Xv_opaque	Psam_label;
	Xv_opaque	Psam_value;
	Xv_opaque	Prate_label;
	Xv_opaque	Prate_value;
	Xv_opaque	Pchan_label;
	Xv_opaque	Pchan_value;
	Xv_opaque	Pprec_label;
	Xv_opaque	Pprec_value;
	Xv_opaque	Pencode_label;
	Xv_opaque	Pencode_value;
	Xv_opaque	RecStatusPanel;
	Xv_opaque	Rpanel_label;
	Xv_opaque	Ropen_flag;
	Xv_opaque	Rpaused_flag;
	Xv_opaque	Ractive_flag;
	Xv_opaque	Rerror_flag;
	Xv_opaque	Rwaiting_flag;
	Xv_opaque	Rsam_label;
	Xv_opaque	Rsam_value;
	Xv_opaque	Rrate_label;
	Xv_opaque	Rrate_value;
	Xv_opaque	Rchan_label;
	Xv_opaque	Rchan_value;
	Xv_opaque	Rprec_label;
	Xv_opaque	Rprec_value;
	Xv_opaque	Rencode_label;
	Xv_opaque	Rencode_value;
	Xv_opaque	StatusControls;
	Xv_opaque	Update_switch;
} gaintool_StatusPanel_objects;

extern gaintool_StatusPanel_objects	*gaintool_StatusPanel_objects_initialize();

extern Xv_opaque	gaintool_StatusPanel_StatusPanel_create();
extern Xv_opaque	gaintool_StatusPanel_PlayStatusPanel_create();
extern Xv_opaque	gaintool_StatusPanel_Ppanel_label_create();
extern Xv_opaque	gaintool_StatusPanel_Popen_flag_create();
extern Xv_opaque	gaintool_StatusPanel_Ppaused_flag_create();
extern Xv_opaque	gaintool_StatusPanel_Pactive_flag_create();
extern Xv_opaque	gaintool_StatusPanel_Perror_flag_create();
extern Xv_opaque	gaintool_StatusPanel_Pwaiting_flag_create();
extern Xv_opaque	gaintool_StatusPanel_Peof_label_create();
extern Xv_opaque	gaintool_StatusPanel_Peof_value_create();
extern Xv_opaque	gaintool_StatusPanel_Psam_label_create();
extern Xv_opaque	gaintool_StatusPanel_Psam_value_create();
extern Xv_opaque	gaintool_StatusPanel_Prate_label_create();
extern Xv_opaque	gaintool_StatusPanel_Prate_value_create();
extern Xv_opaque	gaintool_StatusPanel_Pchan_label_create();
extern Xv_opaque	gaintool_StatusPanel_Pchan_value_create();
extern Xv_opaque	gaintool_StatusPanel_Pprec_label_create();
extern Xv_opaque	gaintool_StatusPanel_Pprec_value_create();
extern Xv_opaque	gaintool_StatusPanel_Pencode_label_create();
extern Xv_opaque	gaintool_StatusPanel_Pencode_value_create();
extern Xv_opaque	gaintool_StatusPanel_RecStatusPanel_create();
extern Xv_opaque	gaintool_StatusPanel_Rpanel_label_create();
extern Xv_opaque	gaintool_StatusPanel_Ropen_flag_create();
extern Xv_opaque	gaintool_StatusPanel_Rpaused_flag_create();
extern Xv_opaque	gaintool_StatusPanel_Ractive_flag_create();
extern Xv_opaque	gaintool_StatusPanel_Rerror_flag_create();
extern Xv_opaque	gaintool_StatusPanel_Rwaiting_flag_create();
extern Xv_opaque	gaintool_StatusPanel_Rsam_label_create();
extern Xv_opaque	gaintool_StatusPanel_Rsam_value_create();
extern Xv_opaque	gaintool_StatusPanel_Rrate_label_create();
extern Xv_opaque	gaintool_StatusPanel_Rrate_value_create();
extern Xv_opaque	gaintool_StatusPanel_Rchan_label_create();
extern Xv_opaque	gaintool_StatusPanel_Rchan_value_create();
extern Xv_opaque	gaintool_StatusPanel_Rprec_label_create();
extern Xv_opaque	gaintool_StatusPanel_Rprec_value_create();
extern Xv_opaque	gaintool_StatusPanel_Rencode_label_create();
extern Xv_opaque	gaintool_StatusPanel_Rencode_value_create();
extern Xv_opaque	gaintool_StatusPanel_StatusControls_create();
extern Xv_opaque	gaintool_StatusPanel_Update_switch_create();
#endif
