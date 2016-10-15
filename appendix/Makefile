all:
	$(MAKE) -C cg_event_set_flags
	$(MAKE) -C cg_post_event
	$(MAKE) -C codesign
	$(MAKE) -C control_led
	$(MAKE) -C dump_hid_report
	$(MAKE) -C dump_hid_value
	$(MAKE) -C dump_system_preferences
	$(MAKE) -C eventtap
	$(MAKE) -C post_key
	$(MAKE) -C session

clean:
	$(MAKE) -C cg_event_set_flags clean
	$(MAKE) -C cg_post_event clean
	$(MAKE) -C codesign clean
	$(MAKE) -C control_led clean
	$(MAKE) -C dump_hid_report clean
	$(MAKE) -C dump_hid_value clean
	$(MAKE) -C dump_system_preferences clean
	$(MAKE) -C eventtap clean
	$(MAKE) -C post_key clean
	$(MAKE) -C session clean
