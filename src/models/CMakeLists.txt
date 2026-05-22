add_library(siem_models STATIC
    User.cpp
    Event.cpp
    Alert.cpp
    UserListModel.cpp
    EventListModel.cpp
    AlertListModel.cpp
    DashboardStatsModel.cpp
    AlertFilterProxyModel.cpp
    RuleListModel.cpp
)

target_link_libraries(siem_models PUBLIC
    Qt6::Core
    Qt6::Network
    Qt6::Sql
)