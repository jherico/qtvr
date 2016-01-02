TEMPLATE = app
QT += gui qml quick xml webengine widgets
CONFIG += c++11
SOURCES += src/main.cpp \
    ../app/src/shadertoy/Cache.cpp

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

DISTFILES += \
    qml/main.qml \
    qml/Stubs.qml \
    ../app/resources/qml/*.qml \
    ../app/resources/qml/controls/*.qml \
    ../app/resources/qml/desktop/*.qml \
    ../app/resources/qml/dialogs/*.qml \
    ../app/resources/qml/dialogs/preferences/*.qml \
    ../app/resources/qml/dialogs/fileDialog/*.qml \
    ../app/resources/qml/styles/*.qml \
    ../app/resources/qml/windows/*.qml \
    ../app/resources/qml/menus/*.qml \
    ../app/resources/qml/shadertoy/*.* \
    ../app/resources/presets/*.* \
    ../app/resources/previz/*.*

HEADERS += \
    ../app/src/shadertoy/Cache.h


