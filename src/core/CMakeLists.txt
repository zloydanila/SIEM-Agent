add_library(siem_core STATIC
    Application.cpp
    Logger.cpp
)

target_link_libraries(siem_core PUBLIC
    Qt6::Core
)