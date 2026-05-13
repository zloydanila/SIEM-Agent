add_library(siem_managers STATIC
    AuthManager.cpp
)

target_link_libraries(siem_managers PUBLIC
    Qt6::Core
    siem_models
    siem_services
)