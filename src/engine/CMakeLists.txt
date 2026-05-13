add_library(siem_engine STATIC
    CorrelationEngine.cpp
    Rule.cpp
)

target_link_libraries(siem_engine PUBLIC
    Qt6::Core
    siem_models
)
