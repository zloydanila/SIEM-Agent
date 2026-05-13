cmake_minimum_required(VERSION 3.21)
project(SIEMAgent VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(QML_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    Sql
    WebSockets
    Network
)

add_subdirectory(src)


add_executable(${PROJECT_NAME}
    main.cpp
)

qt6_add_resources(RESOURCES qml.qrc)
target_sources(${PROJECT_NAME} PRIVATE ${RESOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE
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

enable_testing()
add_subdirectory(tests)