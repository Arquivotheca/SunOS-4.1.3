/*	@(#)cgicbind.h 1.1 92/07/30 Copyr 1985-9 Sun Micro		*/

/*
 * Copyright (c) 1985, 1986, 1987, 1988, 1989 by Sun Microsystems, Inc. 
 * Permission to use, copy, modify, and distribute this software for any 
 * purpose and without fee is hereby granted, provided that the above 
 * copyright notice appear in all copies and that both that copyright 
 * notice and this permission notice are retained, and that the name 
 * of Sun Microsystems, Inc., not be used in advertising or publicity 
 * pertaining to this software without specific, written prior permission. 
 * Sun Microsystems, Inc., makes no representations about the suitability 
 * of this software or the interface defined in this software for any 
 * purpose. It is provided "as is" without express or implied warranty.
 */

/*
 * CGI C-language bindings
 */

#define Copencgi		open_cgi
#define Copenvws		open_vws
#define Cactvws			activate_vws
#define Cdeactvws		deactivate_vws
#define Cclosevws		close_vws
#define Cclosecgi		close_cgi
#define Cqdevid			inquire_device_identification
#define Cqdevclass		inquire_device_class
#define Cqphyscsys		inquire_physical_coordinate_system
#define Cqoutfunset		inquire_output_function_set
#define Cqvdctype		inquire_vdc_type
#define Cqoutcap		inquire_output_capabilities
#define Cqinpcaps		inquire_input_capabilities
#define Cqlidcaps		inquire_lid_capabilities
#define Cqtrigcaps		inquire_trigger_capabilities
#define Cvdcext			vdc_extent
#define Cdevvpt			device_viewport
#define Cclipind		clip_indicator
#define Ccliprect		clip_rectangle
#define Chardrst		hard_reset
#define Crsttodefs		reset_to_defaults
#define Cclrvws			clear_view_surface
#define Cclrcont		clear_control
#define Cserrwarnmk		set_error_warning_mask
#define Csupsig			set_up_sigwinch
#define Cpolyline		polyline
#define Cdpolyline		disjoint_polyline
#define Cpolymarker		polymarker
#define Cpolygon		polygon
#define Cppolygon		partial_polygon
#define Crectangle		rectangle
#define Ccircle			circle
#define Ccircarccent		circular_arc_center
#define Ccircarccentcl		circular_arc_center_close
#define Ccircarcthree		circular_arc_3pt
#define Ccircarcthreecl		circular_arc_3pt_close
#define Cellipse		ellipse
#define Celliparc		elliptical_arc
#define Celliparccl		elliptical_arc_close
#define Ctext			text
#define Cvdmtext		vdm_text
#define Captext			append_text
#define Cqtextext		inquire_text_extent
#define Ccellarr		cell_array
#define Cpixarr			pixel_array
#define Cbtblsouarr		bitblt_source_array
#define Cbtblpatarr		bitblt_pattern_array
#define Cbtblpatsouarr		bitblt_patterned_source_array
#define Cqcellarr		inquire_cell_array
#define Cqpixarr		inquire_pixel_array
#define Cqdevbtmp		inquire_device_bitmap
#define Cqbtblalign		inquire_bitblt_alignments
#define Csdrawmode		set_drawing_mode
#define Csgldrawmode		set_global_drawing_mode
#define Cqdrawmode		inquire_drawing_mode
#define Csaspsouflags		set_aspect_source_flags
#define Cdefbundix		define_bundle_index
#define Cpolylnbundix		polyline_bundle_index
#define Clntype			line_type
#define Clnendstyle		line_endstyle
#define Clnwidthspecmode	line_width_specification_mode
#define Clnwidth		line_width
#define Clncolor		line_color
#define Cpolymkbundix		polymarker_bundle_index
#define Cmktype			marker_type
#define Cmksizespecmode		marker_size_specification_mode
#define Cmksize			marker_size
#define Cmkcolor		marker_color
#define Cflareabundix		fill_area_bundle_index
#define Cintstyle		interior_style
#define Cflcolor		fill_color
#define Chatchix		hatch_index
#define Cpatix			pattern_index
#define Cpattable		pattern_table
#define Cpatrefpt		pattern_reference_point
#define Cpatsize		pattern_size
#define Cpatfillcolor		pattern_with_fill_color
#define Cperimtype		perimeter_type
#define Cperimwidth		perimeter_width
#define Cperimwidthspecmode	perimeter_width_specification_mode
#define Cperimcolor		perimeter_color
#define Ctextbundix		text_bundle_index
#define Ctextprec		text_precision
#define Ccharsetix		character_set_index
#define Ctextfontix		text_font_index
#define Ccharexpfac		character_expansion_factor
#define Ccharspacing		character_spacing
#define Ccharheight		character_height
#define Cfixedfont		fixed_font
#define Ctextcolor		text_color
#define Ccharorientation	character_orientation
#define Ccharpath		character_path
#define Ctextalign		text_alignment
#define Ccotable		color_table
#define Cqlnatts		inquire_line_attributes
#define Cqmkatts		inquire_marker_attributes
#define Cqflareaatts		inquire_fill_area_attributes
#define Cqpatatts		inquire_pattern_attributes
#define Cqtextatts		inquire_text_attributes
#define Cqasfs			inquire_aspect_source_flags
#define Cinitlid		initialize_lid
#define Crelidev		release_input_device
#define Cflusheventqu		flush_event_queue
#define Cselectflusheventqu	selective_flush_of_event_queue
#define Cassoc			associate
#define Csdefatrigassoc		set_default_trigger_associations
#define Cdissoc			dissociate
#define Csinitval		set_initial_value
#define Csvalrange		set_valuator_range
#define Cechoon			echo_on
#define Cechoupd		echo_update
#define Cechooff		echo_off
#define Ctrackon		track_on
#define Ctrackoff		track_off
#define Csampinp		sample_input
#define Cinitreq		initiate_request
#define Creqinp			request_input
#define Cgetlastreqinp		get_last_requested_input
#define Cenevents		enable_events
#define Cdaevents		disable_events
#define Cawaitev		await_event
#define Cqlidstatelis		inquire_lid_state_list
#define Cqlidstate		inquire_lid_state
#define Cqtrigstate		inquire_trigger_state
#define Cqevquestate	        inquire_event_queue_state
