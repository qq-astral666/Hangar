cmake_minimum_required(VERSION 3.21)

project(Hangar
    VERSION 1.0.0
    DESCRIPTION "All your projects in one window, sorted by what they are"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(HANGAR_BUILD_TESTS "Build unit tests" ON)

# --------------------------------------------------------------------------
# Core: finding, classifying and organising projects. Plain C++20
# (std::filesystem), no Qt -> unit-tested on a fake home folder.
# --------------------------------------------------------------------------
add_library(hangar_core STATIC
    src/core/Project.h
    src/core/Project.cpp
    src/core/Organizer.h
    src/core/Organizer.cpp
)
target_include_directories(hangar_core PUBLIC src/core)

if(HANGAR_BUILD_TESTS)
    enable_testing()
    add_executable(tst_core tests/tst_core.cpp)
    target_link_libraries(tst_core PRIVATE hangar_core)
    add_test(NAME core COMMAND tst_core)
endif()

# --------------------------------------------------------------------------
# Application
# --------------------------------------------------------------------------
find_package(Qt6 6.9 REQUIRED COMPONENTS Core Gui Quick QuickControls2 Concurrent)
qt_standard_project_setup(REQUIRES 6.9)

qt_add_executable(Hangar MACOSX_BUNDLE
    src/main.cpp
    src/app/AppController.h
    src/app/AppController.cpp
    src/app/ProjectModel.h
    src/app/ProjectModel.cpp
)
target_include_directories(Hangar PRIVATE src/app)
target_compile_definitions(Hangar PRIVATE PROJECT_VERSION_STRING="${PROJECT_VERSION}")

set(HANGAR_QML_FILES
    qml/Main.qml
    qml/Theme.qml
    qml/Icon.qml
    qml/Sidebar.qml
    qml/ProjectCard.qml
    qml/OrganizeDialog.qml
    qml/DeleteDialog.qml
    qml/EmptyState.qml
    qml/IconButton.qml
)
# Flat module: every file next to the generated qmldir.
foreach(qml_file IN LISTS HANGAR_QML_FILES)
    get_filename_component(qml_name "${qml_file}" NAME)
    set_source_files_properties("${qml_file}" PROPERTIES QT_RESOURCE_ALIAS "${qml_name}")
endforeach()
set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(Hangar
    URI Hangar
    VERSION 1.0
    QML_FILES ${HANGAR_QML_FILES}
)

target_link_libraries(Hangar PRIVATE hangar_core Qt6::Gui Qt6::Quick Qt6::QuickControls2 Qt6::Concurrent)

set_target_properties(Hangar PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Info.plist.in"
    MACOSX_BUNDLE_GUI_IDENTIFIER "app.hangar.Hangar"
    MACOSX_BUNDLE_BUNDLE_NAME "Hangar"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
)

# resources/AppIcon.icns is generated from AppIcon.png by scripts/package.sh.
if(APPLE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/AppIcon.icns")
    set(app_icon "${CMAKE_CURRENT_SOURCE_DIR}/resources/AppIcon.icns")
    set_source_files_properties(${app_icon} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    target_sources(Hangar PRIVATE ${app_icon})
    set_target_properties(Hangar PROPERTIES MACOSX_BUNDLE_ICON_FILE "AppIcon.icns")
endif()
