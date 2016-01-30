TEMPLATE = app

QT += gui qml quick xml webengine widgets

CONFIG += c++11

SOURCES += src/main.cpp \
    ../app/src/GlslEditor.cpp

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

DISTFILES += \
    qml/main.qml \
    ../app/resources/qml/controller/hydra/HydraButtons.qml \
    ../app/resources/qml/controller/hydra/HydraStick.qml \
    ../app/resources/qml/controller/xbox/DPad.qml \
    ../app/resources/qml/controller/xbox/LeftAnalogStick.qml \
    ../app/resources/qml/controller/xbox/RightAnalogStick.qml \
    ../app/resources/qml/controller/xbox/XboxButtons.qml \
    ../app/resources/qml/controller/AnalogButton.qml \
    ../app/resources/qml/controller/AnalogStick.qml \
    ../app/resources/qml/controller/Hydra.qml \
    ../app/resources/qml/controller/Standard.qml \
    ../app/resources/qml/controller/ToggleButton.qml \
    ../app/resources/qml/controller/Xbox.qml \
    ../app/resources/qml/controls/ButtonAwesome.qml \
    ../app/resources/qml/controls/CheckBox.qml \
    ../app/resources/qml/controls/DialogBase.qml \
    ../app/resources/qml/controls/DialogContainer.qml \
    ../app/resources/qml/controls/FontAwesome.qml \
    ../app/resources/qml/controls/Player.qml \
    ../app/resources/qml/controls/Slider.qml \
    ../app/resources/qml/controls/Spacer.qml \
    ../app/resources/qml/controls/SpinBox.qml \
    ../app/resources/qml/controls/Text.qml \
    ../app/resources/qml/controls/TextAndSlider.qml \
    ../app/resources/qml/controls/TextAndSpinBox.qml \
    ../app/resources/qml/controls/TextArea.qml \
    ../app/resources/qml/controls/TextEdit.qml \
    ../app/resources/qml/controls/TextHeader.qml \
    ../app/resources/qml/controls/TextInput.qml \
    ../app/resources/qml/controls/TextInputAndButton.qml \
    ../app/resources/qml/controls/VrDialog.qml \
    ../app/resources/qml/styles/Border.qml \
    ../app/resources/qml/styles/ButtonStyle.qml \
    ../app/resources/qml/styles/HifiConstants.qml \
    ../app/resources/qml/styles/HifiPalette.qml \
    ../app/resources/qml/styles/IconButtonStyle.qml \
    ../app/resources/qml/Browser.qml \
    ../app/resources/qml/QmlWebWindow.qml \
    ../app/resources/qml/QmlWindow.qml \
    ../app/resources/qml/Root.qml \
    ../app/resources/qml/dialogs/FileDialog.qml \
    ../app/resources/qml/dialogs/MessageDialog.qml \
    ../app/resources/qml/windows/DefaultFrame.qml \
    ../app/resources/qml/windows/Fadable.qml \
    ../app/resources/qml/windows/Frame.qml \
    ../app/resources/qml/windows/ModalFrame.qml \
    ../app/resources/qml/windows/Window.qml \
    qml/Stubs.qml \
    ../app/resources/qml/menus/Menu.qml \
    ../app/resources/qml/menus/MenuBar.qml \
    ../app/resources/qml/menus/MenuItem.qml \
    ../app/resources/qml/menus/MenuMouseHandler.qml \
    ../app/resources/qml/menus/VrMenu.qml \
    ../app/resources/qml/menus/VrMenuItem.qml \
    ../app/resources/qml/menus/VrMenuView.qml \
    ../app/resources/qml/shadertoy/AppMenu.qml \
    ../app/resources/qml/Root.qml \
    ../app/resources/qml/desktop/Desktop.qml \
    ../app/resources/qml/shadertoy/AppDesktop.qml \
    ../app/resources/qml/desktop/FocusHack.qml \
    ../app/resources/qml/dialogs/fileDialog/FileTableView.qml \
    ../app/resources/qml/dialogs/PreferencesDialog.qml \
    ../app/resources/qml/dialogs/QueryDialog.qml \
    ../app/resources/qml/dialogs/RunningScripts.qml \
    ../app/resources/qml/dialogs/preferences/BrowsablePreference.qml \
    ../app/resources/qml/dialogs/preferences/ButtonPreference.qml \
    ../app/resources/qml/dialogs/preferences/CheckBoxPreference.qml \
    ../app/resources/qml/dialogs/preferences/EditablePreference.qml \
    ../app/resources/qml/dialogs/preferences/Preference.qml \
    ../app/resources/qml/dialogs/preferences/Section.qml \
    ../app/resources/qml/dialogs/preferences/SliderPreference.qml \
    ../app/resources/qml/dialogs/preferences/SpinBoxPreference.qml \
    ../app/resources/qml/windows/HiddenFrame.qml \
    ../app/resources/qml/windows/ModalWindow.qml \
    ../app/resources/qml/controls/Border.qml \
    ../app/resources/qml/controls/FontAwesome.qml \
    ../app/resources/presets/cube00_0.jpg \
    ../app/resources/presets/cube00_1.jpg \
    ../app/resources/presets/cube00_2.jpg \
    ../app/resources/presets/cube00_3.jpg \
    ../app/resources/presets/cube00_4.jpg \
    ../app/resources/presets/cube00_5.jpg \
    ../app/resources/presets/cube02_0.jpg \
    ../app/resources/presets/cube02_1.jpg \
    ../app/resources/presets/cube02_2.jpg \
    ../app/resources/presets/cube02_3.jpg \
    ../app/resources/presets/cube02_4.jpg \
    ../app/resources/presets/cube02_5.jpg \
    ../app/resources/presets/tex00.jpg \
    ../app/resources/presets/tex01.jpg \
    ../app/resources/presets/tex02.jpg \
    ../app/resources/presets/tex03.jpg \
    ../app/resources/presets/tex04.jpg \
    ../app/resources/presets/tex05.jpg \
    ../app/resources/presets/tex06.jpg \
    ../app/resources/presets/tex07.jpg \
    ../app/resources/presets/tex08.jpg \
    ../app/resources/presets/tex09.jpg \
    ../app/resources/presets/cube01_0.png \
    ../app/resources/presets/cube01_1.png \
    ../app/resources/presets/cube01_2.png \
    ../app/resources/presets/cube01_3.png \
    ../app/resources/presets/cube01_4.png \
    ../app/resources/presets/cube01_5.png \
    ../app/resources/presets/cube03_0.png \
    ../app/resources/presets/cube03_1.png \
    ../app/resources/presets/cube03_2.png \
    ../app/resources/presets/cube03_3.png \
    ../app/resources/presets/cube03_4.png \
    ../app/resources/presets/cube03_5.png \
    ../app/resources/presets/cube04_0.png \
    ../app/resources/presets/cube04_1.png \
    ../app/resources/presets/cube04_2.png \
    ../app/resources/presets/cube04_3.png \
    ../app/resources/presets/cube04_4.png \
    ../app/resources/presets/cube04_5.png \
    ../app/resources/presets/cube05_0.png \
    ../app/resources/presets/cube05_1.png \
    ../app/resources/presets/cube05_2.png \
    ../app/resources/presets/cube05_3.png \
    ../app/resources/presets/cube05_4.png \
    ../app/resources/presets/cube05_5.png \
    ../app/resources/presets/tex10.png \
    ../app/resources/presets/tex11.png \
    ../app/resources/presets/tex12.png \
    ../app/resources/presets/tex14.png \
    ../app/resources/presets/tex15.png \
    ../app/resources/presets/tex16.png \
    ../app/resources/qml/shadertoy/Editor.qml \
    ../app/resources/qml/shadertoy/TextureImage.qml

HEADERS += \
    ../app/src/GlslEditor.h

