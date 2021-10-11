;GIL-3
;@(#)gaintool.G	1.1	92/07/30
(
(
	:type                   :menu
	:name                   props_menu
	:help                   ""
	:columns                1
	:menu-type              :command
	:menu-handler           nil
	:menu-title             "Audio Control"
	:menu-item-labels       ("Status..." )
	:menu-item-label-types  (:string )
	:menu-item-states       (:active )
	:menu-item-defaults     (t )
	:initial-selections     (nil )
	:menu-item-handlers     (menu_status )
	:menu-item-menus        (nil )
	:menu-item-colors       ("" )
	:pinnable               nil
	:user-data              ()
	:actions                (
		(
		:from                   (props_menu "Status...")
		:when                   (Notify )
		:to                     (props_menu "Status...")
		:function_type          CallFunction
		:arg_type               ()
		:action                 (menu_status)
		)
	)
)
(
	:type                   :base-window
	:name                   BaseFrame
	:owner                  nil
	:width                  290
	:height                 94
	:background-color       ""
	:foreground-color       ""
	:label                  "Audio Control Panel"
	:label-type             :string
	:initial-state          :open
	:show-footer            nil
	:resizable              nil
	:icon-file              "gaintool.icon"
	:icon-label             ""
	:icon-mask-file         "gaintool.mask.icon"
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :control-area
	:name                   GainPanel
	:owner                  BaseFrame
	:help                   ""
	:x                      0
	:y                      0
	:width                  290
	:height                 94
	:background-color       ""
	:foreground-color       ""
	:initial-state          :visible
	:show-border            t
	:menu                   nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :slider
	:name                   PlaySlider
	:owner                  GainPanel
	:help                   ""
	:x                      30
	:y                      6
	:width                  254
	:height                 20
	:value-x                121
	:value-y                6
	:slider-width           100
	:ticks                  0
	:foreground-color       ""
	:label                  "Play Volume:"
	:label-type             :string
	:layout-type            :horizontal
	:orientation            :horizontal
	:show-endboxes          nil
	:show-range             nil
	:show-value             t
	:min-value              0
	:max-value              100
	:min-value-string       ""
	:max-value-string       ""
	:min-tick-string        ""
	:max-tick-string        ""
	:initial-value          0
	:initial-state          :active
	:notify-handler         play_volume_proc
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (BaseFrame PlaySlider)
		:when                   (Notify )
		:to                     (BaseFrame PlaySlider)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (play_volume_proc)
		)
	)
)
(
	:type                   :slider
	:name                   RecordSlider
	:owner                  GainPanel
	:help                   ""
	:x                      13
	:y                      24
	:width                  271
	:height                 20
	:value-x                121
	:value-y                24
	:slider-width           100
	:ticks                  0
	:foreground-color       ""
	:label                  "Record Volume:"
	:label-type             :string
	:layout-type            :horizontal
	:orientation            :horizontal
	:show-endboxes          nil
	:show-range             nil
	:show-value             t
	:min-value              0
	:max-value              100
	:min-value-string       ""
	:max-value-string       ""
	:min-tick-string        ""
	:max-tick-string        ""
	:initial-value          0
	:initial-state          :active
	:notify-handler         record_volume_proc
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (BaseFrame RecordSlider)
		:when                   (Notify )
		:to                     (BaseFrame RecordSlider)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (record_volume_proc)
		)
	)
)
(
	:type                   :slider
	:name                   MonitorSlider
	:owner                  GainPanel
	:help                   ""
	:x                      6
	:y                      42
	:width                  278
	:height                 20
	:value-x                121
	:value-y                42
	:slider-width           100
	:ticks                  0
	:foreground-color       ""
	:label                  "Monitor Volume:"
	:label-type             :string
	:layout-type            :horizontal
	:orientation            :horizontal
	:show-endboxes          nil
	:show-range             nil
	:show-value             t
	:min-value              0
	:max-value              100
	:min-value-string       ""
	:max-value-string       ""
	:min-tick-string        ""
	:max-tick-string        ""
	:initial-value          0
	:initial-state          :active
	:notify-handler         monitor_volume_proc
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (BaseFrame MonitorSlider)
		:when                   (Notify )
		:to                     (BaseFrame MonitorSlider)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (monitor_volume_proc)
		)
	)
)
(
	:type                   :setting
	:name                   OutputSwitch
	:owner                  GainPanel
	:help                   ""
	:x                      14
	:y                      67
	:width                  142
	:height                 23
	:value-x                73
	:value-y                67
	:rows                   1
	:columns                0
	:layout-type            :horizontal
	:foreground-color       ""
	:setting-type           :exclusive
	:selection-required     t
	:label                  "Output:"
	:label-type             :string
	:notify-handler         control_output_proc
	:event-handler          nil
	:choices                ("Spkr" "Jack" )
	:choice-label-types     (:string :string )
	:choice-colors          ("" "" )
	:initial-selections     (nil nil )
	:initial-state          :active
	:user-data              ()
	:actions                (
		(
		:from                   (BaseFrame OutputSwitch)
		:when                   (Notify )
		:to                     (BaseFrame OutputSwitch)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (control_output_proc)
		)
	)
)
(
	:type                   :button
	:name                   Pause_button
	:owner                  GainPanel
	:help                   ""
	:x                      180
	:y                      69
	:width                  78
	:height                 19
	:constant-width         t
	:button-type            :normal
	:foreground-color       ""
	:label                  "Resume Play"
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         pause_proc
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (BaseFrame Pause_button)
		:when                   (Notify )
		:to                     (BaseFrame Pause_button)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (pause_proc)
		)
	)
)
(
	:type                   :popup-window
	:name                   StatusPanel
	:owner                  nil
	:width                  379
	:height                 308
	:background-color       ""
	:foreground-color       ""
	:label                  "Audio Device Status"
	:label-type             :string
	:initial-state          :invisible
	:show-footer            nil
	:resizable              nil
	:pinned                 t
	:done-handler           StatusPanel_done_proc
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (StatusPanel)
		:when                   (Done )
		:to                     (StatusPanel)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (StatusPanel_done_proc)
		)
	)
)
(
	:type                   :control-area
	:name                   PlayStatusPanel
	:owner                  StatusPanel
	:help                   "The Play Status panel displays the current state
of the audio device output stream.

If the Update Mode is set to \"Status Change\",
the `Active' and `Samples' fields are only
updated when other audio device status changes.

If the Update Mode is set to \"Continuous\", then
the `Active' and `Samples' fields are updated
several times per second."
	:x                      0
	:y                      0
	:width                  190
	:height                 274
	:background-color       ""
	:foreground-color       ""
	:initial-state          :visible
	:show-border            t
	:menu                   nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :message
	:name                   Ppanel_label
	:owner                  PlayStatusPanel
	:help                   ""
	:x                      57
	:y                      10
	:width                  76
	:height                 13
	:foreground-color       ""
	:label                  "Play Status"
	:label-type             :string
	:label-bold             t
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :setting
	:name                   Popen_flag
	:owner                  PlayStatusPanel
	:help                   ""
	:x                      10
	:y                      33
	:width                  67
	:height                 23
	:value-x                10
	:value-y                33
	:rows                   1
	:columns                0
	:layout-type            :horizontal
	:foreground-color       ""
	:setting-type           :check
	:selection-required     t
	:label                  ""
	:label-type             :string
	:notify-handler         nil
	:event-handler          StatusPanel_null_event_proc
	:choices                ("Open" )
	:choice-label-types     (:string )
	:choice-colors          ("" )
	:initial-selections     (nil )
	:initial-state          :active
	:user-data              ()
	:actions                (
		(
		:from                   (StatusPanel Popen_flag)
		:when                   (AnyEvent )
		:to                     (StatusPanel Popen_flag)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (StatusPanel_null_event_proc)
		)
	)
)
(
	:type                   :setting
	:name                   Ppaused_flag
	:owner                  PlayStatusPanel
	:help                   ""
	:x                      10
	:y                      52
	:width                  78
	:height                 23
	:value-x                10
	:value-y                52
	:rows                   1
	:columns                0
	:layout-type            :horizontal
	:foreground-color       ""
	:setting-type           :check
	:selection-required     t
	:label                  ""
	:label-type             :string
	:notify-handler         nil
	:event-handler          StatusPanel_null_event_proc
	:choices                ("Paused" )
	:choice-label-types     (:string )
	:choice-colors          ("" )
	:initial-selections     (nil )
	:initial-state          :active
	:user-data              ()
	:actions                (
		(
		:from                   (StatusPanel Ppaused_flag)
		:when                   (AnyEvent )
		:to                     (StatusPanel Ppaused_flag)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (StatusPanel_null_event_proc)
		)
	)
)
(
	:type                   :setting
	:name                   Pactive_flag
	:owner                  PlayStatusPanel
	:help                   ""
	:x                      10
	:y                      81
	:width                  71
	:height                 23
	:value-x                10
	:value-y                81
	:rows                   1
	:columns                0
	:layout-type            :horizontal
	:foreground-color       ""
	:setting-type           :check
	:selection-required     t
	:label                  ""
	:label-type             :string
	:notify-handler         nil
	:event-handler          StatusPanel_null_event_proc
	:choices                ("Active" )
	:choice-label-types     (:string )
	:choice-colors          ("" )
	:initial-selections     (nil )
	:initial-state          :active
	:user-data              ()
	:actions                (
		(
		:from                   (StatusPanel Pactive_flag)
		:when                   (AnyEvent )
		:to                     (StatusPanel Pactive_flag)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (StatusPanel_null_event_proc)
		)
	)
)
(
	:type                   :setting
	:name                   Perror_flag
	:owner                  PlayStatusPanel
	:help                   ""
	:x                      10
	:y                      100
	:width                  98
	:height                 23
	:value-x                10
	:value-y                100
	:rows                   1
	:columns                0
	:layout-type            :horizontal
	:foreground-color       ""
	:setting-type           :check
	:selection-required     t
	:label                  ""
	:label-type             :string
	:notify-handler         nil
	:event-handler          StatusPanel_null_event_proc
	:choices                ("Underflow" )
	:choice-label-types     (:string )
	:choice-colors          ("" )
	:initial-selections     (nil )
	:initial-state          :active
	:user-data              ()
	:actions                (
		(
		:from                   (StatusPanel Perror_flag)
		:when                   (AnyEvent )
		:to                     (StatusPanel Perror_flag)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (StatusPanel_null_event_proc)
		)
	)
)
(
	:type                   :setting
	:name                   Pwaiting_flag
	:owner                  PlayStatusPanel
	:help                   ""
	:x                      10
	:y                      119
	:width                  121
	:height                 23
	:value-x                10
	:value-y                119
	:rows                   1
	:columns                0
	:layout-type            :horizontal
	:foreground-color       ""
	:setting-type           :check
	:selection-required     t
	:label                  ""
	:label-type             :string
	:notify-handler         nil
	:event-handler          StatusPanel_null_event_proc
	:choices                ("Open-Waiting" )
	:choice-label-types     (:string )
	:choice-colors          ("" )
	:initial-selections     (nil )
	:initial-state          :active
	:user-data              ()
	:actions                (
		(
		:from                   (StatusPanel Pwaiting_flag)
		:when                   (AnyEvent )
		:to                     (StatusPanel Pwaiting_flag)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (StatusPanel_null_event_proc)
		)
	)
)
(
	:type                   :message
	:name                   Peof_label
	:owner                  PlayStatusPanel
	:help                   ""
	:x                      21
	:y                      152
	:width                  68
	:height                 13
	:foreground-color       ""
	:label                  "EOF Count:"
	:label-type             :string
	:label-bold             nil
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :message
	:name                   Peof_value
	:owner                  PlayStatusPanel
	:help                   ""
	:x                      98
	:y                      152
	:width                  4
	:height                 13
	:foreground-color       ""
	:label                  " "
	:label-type             :string
	:label-bold             t
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :message
	:name                   Psam_label
	:owner                  PlayStatusPanel
	:help                   ""
	:x                      35
	:y                      169
	:width                  54
	:height                 13
	:foreground-color       ""
	:label                  "Samples:"
	:label-type             :string
	:label-bold             nil
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :message
	:name                   Psam_value
	:owner                  PlayStatusPanel
	:help                   ""
	:x                      98
	:y                      169
	:width                  84
	:height                 13
	:foreground-color       ""
	:label                  " 0000000000"
	:label-type             :string
	:label-bold             t
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :message
	:name                   Prate_label
	:owner                  PlayStatusPanel
	:help                   ""
	:x                      10
	:y                      198
	:width                  79
	:height                 13
	:foreground-color       ""
	:label                  "Sample Rate:"
	:label-type             :string
	:label-bold             nil
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :message
	:name                   Prate_value
	:owner                  PlayStatusPanel
	:help                   ""
	:x                      98
	:y                      198
	:width                  4
	:height                 13
	:foreground-color       ""
	:label                  " "
	:label-type             :string
	:label-bold             t
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :message
	:name                   Pchan_label
	:owner                  PlayStatusPanel
	:help                   ""
	:x                      29
	:y                      215
	:width                  60
	:height                 13
	:foreground-color       ""
	:label                  "Channels:"
	:label-type             :string
	:label-bold             nil
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :message
	:name                   Pchan_value
	:owner                  PlayStatusPanel
	:help                   ""
	:x                      98
	:y                      215
	:width                  4
	:height                 13
	:foreground-color       ""
	:label                  " "
	:label-type             :string
	:label-bold             t
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :message
	:name                   Pprec_label
	:owner                  PlayStatusPanel
	:help                   ""
	:x                      31
	:y                      232
	:width                  58
	:height                 13
	:foreground-color       ""
	:label                  "Precision:"
	:label-type             :string
	:label-bold             nil
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :message
	:name                   Pprec_value
	:owner                  PlayStatusPanel
	:help                   ""
	:x                      98
	:y                      232
	:width                  4
	:height                 13
	:foreground-color       ""
	:label                  " "
	:label-type             :string
	:label-bold             t
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :message
	:name                   Pencode_label
	:owner                  PlayStatusPanel
	:help                   ""
	:x                      31
	:y                      249
	:width                  58
	:height                 13
	:foreground-color       ""
	:label                  "Encoding:"
	:label-type             :string
	:label-bold             nil
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :message
	:name                   Pencode_value
	:owner                  PlayStatusPanel
	:help                   ""
	:x                      98
	:y                      249
	:width                  4
	:height                 13
	:foreground-color       ""
	:label                  " "
	:label-type             :string
	:label-bold             t
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :control-area
	:name                   RecStatusPanel
	:owner                  StatusPanel
	:help                   "The Record Status panel displays the current
state of the audio device input stream.

If the Update Mode is set to \"Status Change\",
the `Active' and `Samples' fields are only
updated when other audio device status changes.

If the Update Mode is set to \"Continuous\", then
the `Active' and `Samples' fields are updated
several times per second."
	:x                      189
	:y                      0
	:width                  190
	:height                 274
	:background-color       ""
	:foreground-color       ""
	:initial-state          :visible
	:show-border            t
	:menu                   nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :message
	:name                   Rpanel_label
	:owner                  RecStatusPanel
	:help                   ""
	:x                      45
	:y                      10
	:width                  93
	:height                 13
	:foreground-color       ""
	:label                  "Record Status"
	:label-type             :string
	:label-bold             t
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :setting
	:name                   Ropen_flag
	:owner                  RecStatusPanel
	:help                   ""
	:x                      10
	:y                      33
	:width                  67
	:height                 23
	:value-x                10
	:value-y                33
	:rows                   1
	:columns                0
	:layout-type            :horizontal
	:foreground-color       ""
	:setting-type           :check
	:selection-required     t
	:label                  ""
	:label-type             :string
	:notify-handler         nil
	:event-handler          StatusPanel_null_event_proc
	:choices                ("Open" )
	:choice-label-types     (:string )
	:choice-colors          ("" )
	:initial-selections     (nil )
	:initial-state          :active
	:user-data              ()
	:actions                (
		(
		:from                   (StatusPanel Ropen_flag)
		:when                   (AnyEvent )
		:to                     (StatusPanel Ropen_flag)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (StatusPanel_null_event_proc)
		)
	)
)
(
	:type                   :setting
	:name                   Rpaused_flag
	:owner                  RecStatusPanel
	:help                   ""
	:x                      10
	:y                      52
	:width                  78
	:height                 23
	:value-x                10
	:value-y                52
	:rows                   1
	:columns                0
	:layout-type            :horizontal
	:foreground-color       ""
	:setting-type           :check
	:selection-required     t
	:label                  ""
	:label-type             :string
	:notify-handler         nil
	:event-handler          StatusPanel_null_event_proc
	:choices                ("Paused" )
	:choice-label-types     (:string )
	:choice-colors          ("" )
	:initial-selections     (nil )
	:initial-state          :active
	:user-data              ()
	:actions                (
		(
		:from                   (StatusPanel Rpaused_flag)
		:when                   (AnyEvent )
		:to                     (StatusPanel Rpaused_flag)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (StatusPanel_null_event_proc)
		)
	)
)
(
	:type                   :setting
	:name                   Ractive_flag
	:owner                  RecStatusPanel
	:help                   ""
	:x                      10
	:y                      81
	:width                  71
	:height                 23
	:value-x                10
	:value-y                81
	:rows                   1
	:columns                0
	:layout-type            :horizontal
	:foreground-color       ""
	:setting-type           :check
	:selection-required     t
	:label                  ""
	:label-type             :string
	:notify-handler         nil
	:event-handler          StatusPanel_null_event_proc
	:choices                ("Active" )
	:choice-label-types     (:string )
	:choice-colors          ("" )
	:initial-selections     (nil )
	:initial-state          :active
	:user-data              ()
	:actions                (
		(
		:from                   (StatusPanel Ractive_flag)
		:when                   (AnyEvent )
		:to                     (StatusPanel Ractive_flag)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (StatusPanel_null_event_proc)
		)
	)
)
(
	:type                   :setting
	:name                   Rerror_flag
	:owner                  RecStatusPanel
	:help                   ""
	:x                      10
	:y                      100
	:width                  90
	:height                 23
	:value-x                10
	:value-y                100
	:rows                   1
	:columns                0
	:layout-type            :horizontal
	:foreground-color       ""
	:setting-type           :check
	:selection-required     t
	:label                  ""
	:label-type             :string
	:notify-handler         nil
	:event-handler          StatusPanel_null_event_proc
	:choices                ("Overflow" )
	:choice-label-types     (:string )
	:choice-colors          ("" )
	:initial-selections     (nil )
	:initial-state          :active
	:user-data              ()
	:actions                (
		(
		:from                   (StatusPanel Rerror_flag)
		:when                   (AnyEvent )
		:to                     (StatusPanel Rerror_flag)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (StatusPanel_null_event_proc)
		)
	)
)
(
	:type                   :setting
	:name                   Rwaiting_flag
	:owner                  RecStatusPanel
	:help                   ""
	:x                      10
	:y                      119
	:width                  121
	:height                 23
	:value-x                10
	:value-y                119
	:rows                   1
	:columns                0
	:layout-type            :horizontal
	:foreground-color       ""
	:setting-type           :check
	:selection-required     t
	:label                  ""
	:label-type             :string
	:notify-handler         nil
	:event-handler          StatusPanel_null_event_proc
	:choices                ("Open-Waiting" )
	:choice-label-types     (:string )
	:choice-colors          ("" )
	:initial-selections     (nil )
	:initial-state          :active
	:user-data              ()
	:actions                (
		(
		:from                   (StatusPanel Rwaiting_flag)
		:when                   (AnyEvent )
		:to                     (StatusPanel Rwaiting_flag)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (StatusPanel_null_event_proc)
		)
	)
)
(
	:type                   :message
	:name                   Rsam_label
	:owner                  RecStatusPanel
	:help                   ""
	:x                      35
	:y                      169
	:width                  54
	:height                 13
	:foreground-color       ""
	:label                  "Samples:"
	:label-type             :string
	:label-bold             nil
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :message
	:name                   Rsam_value
	:owner                  RecStatusPanel
	:help                   ""
	:x                      98
	:y                      169
	:width                  84
	:height                 13
	:foreground-color       ""
	:label                  " 0000000000"
	:label-type             :string
	:label-bold             t
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :message
	:name                   Rrate_label
	:owner                  RecStatusPanel
	:help                   ""
	:x                      10
	:y                      198
	:width                  79
	:height                 13
	:foreground-color       ""
	:label                  "Sample Rate:"
	:label-type             :string
	:label-bold             nil
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :message
	:name                   Rrate_value
	:owner                  RecStatusPanel
	:help                   ""
	:x                      98
	:y                      198
	:width                  4
	:height                 13
	:foreground-color       ""
	:label                  " "
	:label-type             :string
	:label-bold             t
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :message
	:name                   Rchan_label
	:owner                  RecStatusPanel
	:help                   ""
	:x                      29
	:y                      215
	:width                  60
	:height                 13
	:foreground-color       ""
	:label                  "Channels:"
	:label-type             :string
	:label-bold             nil
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :message
	:name                   Rchan_value
	:owner                  RecStatusPanel
	:help                   ""
	:x                      98
	:y                      215
	:width                  4
	:height                 13
	:foreground-color       ""
	:label                  " "
	:label-type             :string
	:label-bold             t
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :message
	:name                   Rprec_label
	:owner                  RecStatusPanel
	:help                   ""
	:x                      31
	:y                      232
	:width                  58
	:height                 13
	:foreground-color       ""
	:label                  "Precision:"
	:label-type             :string
	:label-bold             nil
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :message
	:name                   Rprec_value
	:owner                  RecStatusPanel
	:help                   ""
	:x                      98
	:y                      232
	:width                  4
	:height                 13
	:foreground-color       ""
	:label                  " "
	:label-type             :string
	:label-bold             t
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :message
	:name                   Rencode_label
	:owner                  RecStatusPanel
	:help                   ""
	:x                      31
	:y                      249
	:width                  58
	:height                 13
	:foreground-color       ""
	:label                  "Encoding:"
	:label-type             :string
	:label-bold             nil
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :message
	:name                   Rencode_value
	:owner                  RecStatusPanel
	:help                   ""
	:x                      98
	:y                      249
	:width                  4
	:height                 13
	:foreground-color       ""
	:label                  " "
	:label-type             :string
	:label-bold             t
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :control-area
	:name                   StatusControls
	:owner                  StatusPanel
	:help                   "When Update Mode is set to \"Status Change\", the
status panel is updated whenever a change in the
state of the audio device results in the trans-
mission of a SIGPOLL signal to registered appli-
cations.

When Update Mode is set to \"Continuous\", the
audio device status is updated several times
per second."
	:x                      0
	:y                      273
	:width                  379
	:height                 35
	:background-color       ""
	:foreground-color       ""
	:initial-state          :visible
	:show-border            t
	:menu                   nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :setting
	:name                   Update_switch
	:owner                  StatusControls
	:help                   ""
	:x                      45
	:y                      6
	:width                  289
	:height                 23
	:value-x                144
	:value-y                6
	:rows                   1
	:columns                0
	:layout-type            :horizontal
	:foreground-color       ""
	:setting-type           :exclusive
	:selection-required     t
	:label                  "Update Mode:"
	:label-type             :string
	:notify-handler         StatusPanel_update_proc
	:event-handler          nil
	:choices                ("Status Change" "Continuous" )
	:choice-label-types     (:string :string )
	:choice-colors          ("" "" )
	:initial-selections     (nil nil )
	:initial-state          :active
	:user-data              ()
	:actions                (
		(
		:from                   (StatusPanel Update_switch)
		:when                   (Notify )
		:to                     (StatusPanel Update_switch)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (StatusPanel_update_proc)
		)
	)
)
)
