cmake_minimum_required(VERSION 3.16)

project(AulaF75Linux
    VERSION 1.0.0
    DESCRIPTION "Native KDE/Linux configurator for the AULA F75 keyboard"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

add_executable(aula-f75
    src/main.cpp
    src/device/f75protocol.cpp
    src/device/f75device.cpp
    src/device/devicemanager.cpp
    src/model/keymap.cpp
    src/model/profile.cpp
    src/ui/mainwindow.cpp
    src/ui/keyboardview.cpp
    src/ui/colorbutton.cpp
    src/ui/devicepage.cpp
    src/ui/lightingpage.cpp
    src/ui/remappage.cpp
    src/ui/macrospage.cpp
    src/ui/settingspage.cpp
    resources/resources.qrc
)

target_include_directories(aula-f75 PRIVATE src)

target_link_libraries(aula-f75 PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)

install(TARGETS aula-f75 RUNTIME DESTINATION bin)
install(FILES resources/org.aula.F75.desktop DESTINATION share/applications)
install(FILES resources/org.aula.F75.svg DESTINATION share/icons/hicolor/scalable/apps RENAME org.aula.F75.svg)
