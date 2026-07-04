# fangcore: everything except main.cpp, as a static lib backing the "Fang"
# QML module, so tests can link and exercise the core headlessly.

set(FANG_CORE_SOURCES
    app/Application.h app/Application.cpp
    app/WindowController.h app/WindowController.cpp
    app/TabController.h app/TabController.cpp
    app/PaneController.h app/PaneController.cpp
    fs/DirectoryModel.h fs/DirectoryModel.cpp
    fs/FileSystemWatcher.h fs/FileSystemWatcher.cpp
    fs/MimeResolver.h fs/MimeResolver.cpp
    fs/IconResolver.h fs/IconResolver.cpp
    fs/FileInfo.h fs/FileInfo.cpp
    nav/Navigation.h nav/Navigation.cpp
    nav/Selection.h nav/Selection.cpp
    places/Places.h places/Places.cpp
    places/MountMonitor.h places/MountMonitor.cpp
    config/Theme.h config/Theme.cpp
    util/Enums.h
    util/HumanFormat.h util/HumanFormat.cpp
    util/Logging.h util/Logging.cpp
)

set(FANG_QML_FILES
    ui/Main.qml
    ui/MainWindow.qml
    ui/TabBar.qml
    ui/TabButton.qml
    ui/Toolbar.qml
    ui/LocationBar.qml
    ui/ViewModeSwitch.qml
    ui/Sidebar.qml
    ui/PlaceRow.qml
    ui/StatusBar.qml
    ui/FileView.qml
    ui/FileDelegate.qml
    ui/IconsView.qml
    ui/ListView.qml
    ui/DetailsView.qml
    ui/TreeView.qml
    ui/ColumnHeader.qml
    ui/ContextMenu.qml
    ui/CreateDialog.qml
    ui/NotificationOverlay.qml
    ui/Shortcuts.qml
    ui/controls/Badge.qml
    ui/controls/Button.qml
    ui/controls/Checkbox.qml
    ui/controls/IconButton.qml
    ui/controls/Menu.qml
    ui/controls/MenuItem.qml
    ui/controls/Popup.qml
    ui/controls/ScrollBar.qml
    ui/controls/Separator.qml
    ui/controls/Slider.qml
    ui/controls/Spinner.qml
    ui/controls/Splitter.qml
    ui/controls/TextField.qml
    ui/controls/ToolButton.qml
    ui/controls/Tooltip.qml
)

set(FANG_RESOURCE_FILES
    ui/themes/dark.toml
    ui/themes/light.toml
    ui/themes/actions.toml
)

# Bundled data lands at qrc:/Fang/themes/... (docs/06).
set_source_files_properties(ui/themes/dark.toml PROPERTIES QT_RESOURCE_ALIAS themes/dark.toml)
set_source_files_properties(ui/themes/light.toml PROPERTIES QT_RESOURCE_ALIAS themes/light.toml)
set_source_files_properties(ui/themes/actions.toml PROPERTIES QT_RESOURCE_ALIAS themes/actions.toml)

qt_policy(SET QTP0004 NEW) # generate qmldir entries for QML files in subdirs

qt_add_library(fangcore STATIC ${FANG_CORE_SOURCES})

# The project includes with subdir prefixes ("config/Theme.h"); the extra
# per-subdir entries let the generated QML type registration find bare
# header names (<Application.h>).
target_include_directories(fangcore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app
    ${CMAKE_CURRENT_SOURCE_DIR}/fs
    ${CMAKE_CURRENT_SOURCE_DIR}/ops
    ${CMAKE_CURRENT_SOURCE_DIR}/nav
    ${CMAKE_CURRENT_SOURCE_DIR}/search
    ${CMAKE_CURRENT_SOURCE_DIR}/preview
    ${CMAKE_CURRENT_SOURCE_DIR}/places
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${CMAKE_CURRENT_SOURCE_DIR}/util)
target_compile_definitions(fangcore PUBLIC FANG_VERSION="${PROJECT_VERSION}")
target_compile_options(fangcore PRIVATE -Wall -Wextra -Werror)
target_link_libraries(fangcore PUBLIC
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2
    Qt6::Concurrent Qt6::DBus
    tomlplusplus::tomlplusplus)

qt_add_qml_module(fangcore
    URI Fang
    VERSION 1.0
    RESOURCE_PREFIX /
    QML_FILES ${FANG_QML_FILES}
    RESOURCES ${FANG_RESOURCE_FILES}
)

qt_add_executable(fang main.cpp)
target_compile_options(fang PRIVATE -Wall -Wextra -Werror)
target_link_libraries(fang PRIVATE fangcore fangcoreplugin)

install(TARGETS fang RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
