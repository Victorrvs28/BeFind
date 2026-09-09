cmake_minimum_required(VERSION 3.16)

project(BeSearch VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui

)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(BeSearch
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        # REMOVE EA/main.cpp daqui se for subprojeto
    )
else()
    if(ANDROID)
        add_library(BeSearch SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(BeSearch
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(BeSearch PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.BeSearch)
endif()

set_target_properties(BeSearch PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS BeSearch
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(BeSearch)
endif()

# SÓ UM add_subdirectory, com verificação
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/EA/CMakeLists.txt")
    add_subdirectory(EA)
endif()
# REMOVE esse segundo add_subdirectory(EA)!
