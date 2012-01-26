uidir = $(pkgdatadir)/ui
ui_DATA = mate-nettool.ui

desktopdir = $(datadir)/applications
desktop_in_in_files = mate-nettool.desktop.in.in
desktop_in_files = $(desktop_in_in_files:.desktop.in.in=.desktop.in)
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

if MATECONF_SCHEMAS_INSTALL
install-data-local:
        @$(NORMAL_INSTALL)
endif

EXTRA_DIST =                    \
	$(desktop_in_in_files)  \
	$(ui_DATA)

DISTCLEANFILES =                \
	$(desktop_in_files)

CLEANFILES =                    \
	$(desktop_DATA)
