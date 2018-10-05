# Modify this if your locally compiled Cateyes isn't next to this directory
CATEYES = $$absolute_path("../")

win32 {
    win32-msvc*:contains(QMAKE_TARGET.arch, x86_64): {
        CATEYES_HOST = x64-Release
    } else {
        CATEYES_HOST = Win32-Release
    }
}
macx {
    CATEYES_BUILD = macos-x86_64
    CATEYES_HOST = macos-x86_64
}
linux {
    CATEYES_BUILD = linux-x86_64
    CATEYES_HOST = linux-x86_64
}

TEMPLATE = lib
TARGET = cateyes-qml
TARGETPATH = Cateyes
QT += qml quick
CONFIG += qt plugin create_prl c++11

TARGET = $$qtLibraryTarget($$TARGET)
QMAKE_MOC_OPTIONS += -Muri=$$TARGETPATH

# Input
SOURCES += \
    plugin.cpp \
    device.cpp \
    process.cpp \
    maincontext.cpp \
    cateyes.cpp \
    script.cpp \
    devicelistmodel.cpp \
    processlistmodel.cpp \
    iconprovider.cpp

HEADERS += \
    plugin.h \
    device.h \
    process.h \
    maincontext.h \
    cateyes.h \
    script.h \
    devicelistmodel.h \
    processlistmodel.h \
    iconprovider.h

OTHER_FILES = qmldir cateyes-qml.qmltypes

qmldir.files = qmldir
qmltypes.files = cateyes-qml.qmltypes
prlmeta.files = cateyes-qml.prl
win32:installPath = $${CATEYES}/build/cateyes-windows/$${CATEYES_HOST}/lib/qt5/qml/Cateyes
unix:installPath = $${CATEYES}/build/cateyes-$${CATEYES_HOST}/lib/qt5/qml/Cateyes
target.path = $$installPath
qmldir.path = $$installPath
qmltypes.path = $$installPath
prlmeta.path = $$installPath
INSTALLS += target qmldir qmltypes prlmeta

win32 {
    CATEYES_SDK_LIBS = \
        intl.lib \
        ffi.lib \
        z.lib \
        glib-2.0.lib gmodule-2.0.lib gobject-2.0.lib gthread-2.0.lib gio-2.0.lib \
        gee-0.8.lib \
        json-glib-1.0.lib

    INCLUDEPATH += "$${CATEYES}/build/sdk-windows/$${CATEYES_HOST}/include/glib-2.0"
    INCLUDEPATH += "$${CATEYES}/build/sdk-windows/$${CATEYES_HOST}/lib/glib-2.0/include"
    INCLUDEPATH += "$${CATEYES}/build/sdk-windows/$${CATEYES_HOST}/include/gee-0.8"
    INCLUDEPATH += "$${CATEYES}/build/sdk-windows/$${CATEYES_HOST}/include/json-glib-1.0"
    INCLUDEPATH += "$${CATEYES}/build/tmp-windows/$${CATEYES_HOST}/cateyes-core"

    LIBS += dnsapi.lib iphlpapi.lib ole32.lib psapi.lib shlwapi.lib winmm.lib ws2_32.lib
    LIBS += -L"$${CATEYES}/build/sdk-windows/$${CATEYES_HOST}/lib" $${CATEYES_SDK_LIBS}
    LIBS += -L"$${CATEYES}/build/tmp-windows/$${CATEYES_HOST}/cateyes-core" cateyes-core.lib
    QMAKE_LFLAGS_DEBUG += /LTCG /NODEFAULTLIB:libcmtd.lib
    QMAKE_LFLAGS_RELEASE += /LTCG /NODEFAULTLIB:libcmt.lib

    QMAKE_LIBFLAGS += /LTCG
}

!win32 {
    QT_CONFIG -= no-pkg-config
    CONFIG += link_pkgconfig
    PKG_CONFIG = PKG_CONFIG_PATH=$${CATEYES}/build/sdk-$${CATEYES_HOST}/lib/pkgconfig:$${CATEYES}/build/cateyes-$${CATEYES_HOST}/lib/pkgconfig $${CATEYES}/build/toolchain-$${CATEYES_BUILD}/bin/pkg-config --define-variable=cateyes_sdk_prefix=$${CATEYES}/build/sdk-$${CATEYES_HOST} --static
    PKGCONFIG += cateyes-core-1.0
}

macx {
    QMAKE_CXXFLAGS = -stdlib=libc++ -Wno-deprecated-register
    QMAKE_LFLAGS += -Wl,-exported_symbol,_qt_plugin_query_metadata -Wl,-exported_symbol,_qt_plugin_instance -Wl,-dead_strip
}

linux {
    QMAKE_LFLAGS += -Wl,--version-script -Wl,cateyes-qml.version -Wl,--gc-sections -Wl,-z,noexecstack
}
