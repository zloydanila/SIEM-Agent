add_library(siem_services STATIC
    DatabaseService.cpp
    WebSocketService.cpp
    WebSocketWorker.cpp
    ReportService.cpp
    ReportService.h
    HttpServer.cpp
)

target_link_libraries(siem_services PUBLIC
    Qt6::Core
    Qt6::Sql
    Qt6::WebSockets
    Qt6::Network
    siem_models
    siem_engine
)