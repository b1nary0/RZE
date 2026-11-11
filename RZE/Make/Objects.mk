# Read after a project's generated Makefile by RZE/Makefile (make -f <Project>_Linux.make -f Objects.mk rze-objects):
# a goal that compiles the project's precompiled header and objects, without archiving or linking. GCH is empty for a
# project with no precompiled header; OBJECTS never is.

ifeq ($(strip $(OBJECTS)),)
  $(error Objects.mk: $(firstword $(MAKEFILE_LIST)) has no OBJECTS for config=$(config). Either config is wrong, or Sharpmake's Makefile generator renamed the variable, and Make/Objects.mk needs updating to match)
endif

.PHONY: rze-objects
# The empty recipe keeps make from printing "Nothing to be done for 'rze-objects'" for each project when it's up to date
rze-objects: $(GCH) $(OBJECTS)
	@:
