cmake_minimum_required(VERSION 3.21)

project(DesktopGit
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS
    Concurrent
    Core
    Gui
    Qml
    Quick
    QuickControls2
    Test
)

qt_standard_project_setup(REQUIRES 6.5)

if(COMMAND qt_policy)
    qt_policy(SET QTP0004 NEW)
endif()

file(GLOB_RECURSE DESKTOP_GIT_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
)

file(GLOB_RECURSE DESKTOP_GIT_ICON_FILES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    CONFIGURE_DEPENDS
    assets/icons/lucide/*.svg
)

set(DESKTOP_GIT_CORE_SOURCES ${DESKTOP_GIT_SOURCES})
list(REMOVE_ITEM DESKTOP_GIT_CORE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

qt_add_library(desktop_git_core STATIC
    ${DESKTOP_GIT_CORE_SOURCES}
)

target_include_directories(desktop_git_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/app
        ${CMAKE_CURRENT_SOURCE_DIR}/src/git
        ${CMAKE_CURRENT_SOURCE_DIR}/src/models
)

target_link_libraries(desktop_git_core
    PUBLIC
        Qt6::Concurrent
        Qt6::Core
        Qt6::Gui
)

qt_add_executable(desktop_git
    src/main.cpp
)

set_source_files_properties(src/ui/Main.qml
    PROPERTIES
        QT_RESOURCE_ALIAS Main.qml
)

set_source_files_properties(src/ui/components/AppButton.qml
    PROPERTIES
        QT_RESOURCE_ALIAS AppButton.qml
)

set_source_files_properties(src/ui/components/AppIcon.qml
    PROPERTIES
        QT_RESOURCE_ALIAS AppIcon.qml
)

set_source_files_properties(src/ui/views/CommitHistoryView.qml
    PROPERTIES
        QT_RESOURCE_ALIAS CommitHistoryView.qml
)

set_source_files_properties(src/ui/views/WorkingTreeView.qml
    PROPERTIES
        QT_RESOURCE_ALIAS WorkingTreeView.qml
)

set_source_files_properties(src/ui/views/BranchesView.qml
    PROPERTIES
        QT_RESOURCE_ALIAS BranchesView.qml
)

set_source_files_properties(src/ui/views/StashView.qml
    PROPERTIES
        QT_RESOURCE_ALIAS StashView.qml
)

qt_add_qml_module(desktop_git
    URI DesktopGit
    VERSION 1.0
    QML_FILES
        src/ui/Main.qml
        src/ui/components/AppButton.qml
        src/ui/components/AppIcon.qml
        src/ui/views/BranchesView.qml
        src/ui/views/CommitHistoryView.qml
        src/ui/views/StashView.qml
        src/ui/views/WorkingTreeView.qml
)

qt_add_resources(desktop_git "desktop_git_icons"
    PREFIX "/"
    FILES
        ${DESKTOP_GIT_ICON_FILES}
)

target_include_directories(desktop_git
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/app
        ${CMAKE_CURRENT_SOURCE_DIR}/src/git
        ${CMAKE_CURRENT_SOURCE_DIR}/src/models
)

target_link_libraries(desktop_git
    PRIVATE
        desktop_git_core
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
)

enable_testing()

qt_add_executable(desktop_git_tests
    tests/TestDesktopGitCore.cpp
)

target_link_libraries(desktop_git_tests
    PRIVATE
        desktop_git_core
        Qt6::Test
)

add_test(NAME desktop_git_tests COMMAND desktop_git_tests)
set_tests_properties(desktop_git_tests
    PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
