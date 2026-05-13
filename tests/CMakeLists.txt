cmake_minimum_required(VERSION 3.16)

find_package(Qt6 REQUIRED COMPONENTS Test Sql)

qt_add_executable(tst_AuthManager
    tst_AuthManager.cpp
)

target_link_libraries(tst_AuthManager
    PRIVATE
        Qt6::Test
        Qt6::Sql
        siem_models
        siem_services
        siem_managers   
        siem_core
)

target_include_directories(tst_AuthManager PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

add_test(NAME tst_AuthManager COMMAND tst_AuthManager)

qt_add_executable(tst_CorrelationEngine
    tst_CorrelationEngine.cpp
)

target_link_libraries(tst_CorrelationEngine
    PRIVATE
        Qt6::Test
        Qt6::Sql
        siem_models
        siem_services
        siem_engine
        siem_managers   
        siem_core
)

target_include_directories(tst_CorrelationEngine PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

add_test(NAME tst_CorrelationEngine COMMAND tst_CorrelationEngine)