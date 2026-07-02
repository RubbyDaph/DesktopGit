cmake_minimum_required(VERSION 3.21)

project(DesktopGit
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
)

qt_standard_project_setup(REQUIRES 6.5)

if(COMMAND qt_policy)
    qt_policy(SET QTP0004 NEW)
endif()

qt_add_executable(desktop_git
    src/main.cpp
)

set_source_files_properties(src/ui/Main.qml
    PROPERTIES
        QT_RESOURCE_ALIAS Main.qml
)

qt_add_qml_module(desktop_git
    URI DesktopGit
    VERSION 1.0
    QML_FILES
        src/ui/Main.qml
)

target_link_libraries(desktop_git
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
)
