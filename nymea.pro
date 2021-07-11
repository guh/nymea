include(nymea.pri)

# Parse and export NYMEA_VERSION_STRING
NYMEA_VERSION_STRING=$$system('dpkg-parsechangelog | sed -n -e "s/^Version: //p"')

# define protocol versions
JSON_PROTOCOL_VERSION_MAJOR=5
JSON_PROTOCOL_VERSION_MINOR=7
JSON_PROTOCOL_VERSION="$${JSON_PROTOCOL_VERSION_MAJOR}.$${JSON_PROTOCOL_VERSION_MINOR}"
LIBNYMEA_API_VERSION_MAJOR=7
LIBNYMEA_API_VERSION_MINOR=2
LIBNYMEA_API_VERSION_PATCH=0
LIBNYMEA_API_VERSION="$${LIBNYMEA_API_VERSION_MAJOR}.$${LIBNYMEA_API_VERSION_MINOR}.$${LIBNYMEA_API_VERSION_PATCH}"

QMAKE_SUBSTITUTES += version.h.in


TEMPLATE=subdirs

piconly {
    message("Plugininfocompiler build. Only nymea-plugininfocompiler will be built.")
    SUBDIRS += tools
} else:minimal {
    message("Minimal build. Only libraries required to build nymea-plugins will be built.")
    SUBDIRS += libnymea tools
} else {
    SUBDIRS += libnymea server libnymea-core plugins tools
    libnymea-core.depends = libnymea
    plugins.depends = libnymea tools
    server.depends = libnymea libnymea-core plugins

    # Build tests
    disabletesting {
        message("Building nymea without tests")
    } else {
        message("Building nymea with tests")
        SUBDIRS += tests
        tests.depends = libnymea libnymea-core
    }
}

doc.depends = FORCE
# Note: some how extraimages in qdocconf did not the trick
doc.commands += cd $$top_srcdir/doc; ./generate-interfaces-qdoc.py;
doc.commands += cd $$top_srcdir/doc; ./generate-api-qdoc.py;
doc.commands += cd $$top_srcdir/doc; qdoc --highlighting config.qdocconf; cp -r images/* html/images/; \
                cp -r favicons/* html/; cp -r $$top_srcdir/doc/html $$top_builddir/
QMAKE_EXTRA_TARGETS += doc

# Translations:
# make lupdate to update .ts files
CORE_TRANSLATIONS += $$files($${top_srcdir}/translations/*.ts, true)
lupdate.commands = lupdate -no-obsolete -recursive $${top_srcdir}/libnymea $${top_srcdir}/libnymea-core $${top_srcdir}/server $${top_srcdir}/libnymea $${top_srcdir}/tools -ts $${CORE_TRANSLATIONS};
lupdate.commands += make -C plugins/mock plugininfo;
PLUGIN_TRANSLATIONS += $$files($${top_srcdir}/plugins/mock/translations/*.ts, true)
lupdate.commands += lupdate -no-obsolete -recursive $${top_srcdir}/plugins/mock/ $${top_builddir}/plugins/mock/ -ts $${PLUGIN_TRANSLATIONS};
lupdate.depends = FORCE qmake_all
TRANSLATIONS = $${CORE_TRANSLATIONS} $${PLUGIN_TRANSLATIONS}

# make lrelease to compile .ts to .qm
lrelease.depends = FORCE
lrelease.commands = lrelease $$_FILE_; \
                    rsync -a $$top_srcdir/translations/*.qm $$top_builddir/translations/; \
                    rsync -a $$top_srcdir/plugins/mock/translations/*.qm $$top_builddir/plugins/mock/translations/;
first.depends = $(first) lrelease

# Install translation files
translations.path = /usr/share/nymea/translations
translations.depends = lrelease
INSTALLS += translations

QMAKE_EXTRA_TARGETS += lupdate lrelease

test.depends += lrelease
test.commands = LD_LIBRARY_PATH=$$top_builddir/libnymea-core:$$top_builddir/libnymea:$$top_builddir/tests/testlib make check TESTRUNNER=\"dbus-test-runner --bus-type=both --task\"
QMAKE_EXTRA_TARGETS += test

# Show doc files in project tree
OTHER_FILES += doc/*.qdoc* \
               doc/tutorials/*.qdoc*

# Inform about nymea build
message(============================================)
message("Qt version:" $$[QT_VERSION])
message("Copyright $${COPYRIGHT_YEAR_FROM} - $${COPYRIGHT_YEAR_TO}")
message("Building nymea version $${NYMEA_VERSION_STRING}")
message("JSON-RPC API version $${JSON_PROTOCOL_VERSION_MAJOR}.$${JSON_PROTOCOL_VERSION_MINOR}")
message("Source directory: $${top_srcdir}")
message("Build directory: $${top_builddir}")
message("Translations: $${TRANSLATIONS}")

# Check debug mode
CONFIG(debug, debug|release) {
    message("Debug build")
} else {
    message("Release build")
}

# Build coverage
coverage {
    message("Building coverage.")
}

# Build using ccache
ccache {
    message("Using ccache.")
}
