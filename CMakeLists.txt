cmake_minimum_required(VERSION 3.21)
project(SIEMAgent VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

option(BUILD_DESKTOP "Build QML desktop app" ON)
option(BUILD_SERVER "Build headless server app" ON)
option(BUILD_TESTS "Build tests" ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    Sql
    WebSockets
    Network
    Test
)

add_subdirectory(src)

if(BUILD_DESKTOP)
    qt_add_executable(siem-desktop
        main_desktop.cpp
    )

    qt6_add_resources(DESKTOP_RESOURCES qml.qrc)
    target_sources(siem-desktop PRIVATE ${DESKTOP_RESOURCES})

    target_link_libraries(siem-desktop PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::Sql
        Qt6::WebSockets
        Qt6::Network
        siem_core
        siem_models
        siem_managers
        siem_services
        siem_engine
    )
endif()

if(BUILD_SERVER)
    qt_add_executable(siem-server
        main_server.cpp
    )

    target_link_libraries(siem-server PRIVATE
        Qt6::Core
        Qt6::Sql
        Qt6::WebSockets
        Qt6::Network
        siem_core
        siem_models
        siem_managers
        siem_services
        siem_engine
    )
endif()

enable_testing()

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()