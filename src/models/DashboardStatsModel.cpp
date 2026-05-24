#include "DashboardStatsModel.h"
#include "../services/DatabaseService.h"

DashboardStatsModel::DashboardStatsModel(DatabaseService *dbService, QObject *parent)
    : QObject(parent), m_db(dbService) {
    refresh();
}

void DashboardStatsModel::refresh() {
    if (!m_db) return;
    updateCounts();
    updateTopDevices();
    updateActivity();
    emit statsChanged();
}

void DashboardStatsModel::refreshCharts() {
    if (!m_db) return;
    updateTopDevices();
    updateActivity();
    emit statsChanged();
}

void DashboardStatsModel::onEventReceived() {
    if (!m_db) return;
    updateCounts();
    emit statsChanged();
}

void DashboardStatsModel::onAlertReceived() {
    if (!m_db) return;
    m_totalAlerts = m_db->getAlertCount();
    m_openAlerts = m_db->getAlertCountByStatus("open");
    emit statsChanged();
}

void DashboardStatsModel::updateCounts() {
    m_criticalCount = m_db->getEventCountBySeverity("critical");
    m_highCount = m_db->getEventCountBySeverity("high");
    m_mediumCount = m_db->getEventCountBySeverity("medium");
    m_lowCount = m_db->getEventCountBySeverity("low");
    m_totalEvents = m_db->getTotalEventsCount();
    m_totalAlerts = m_db->getAlertCount();
    m_openAlerts = m_db->getAlertCountByStatus("open");
}

void DashboardStatsModel::updateTopDevices() {
    m_topDevices = m_db->getTopDevices(5);
}

void DashboardStatsModel::updateActivity() {
    QVariantList raw = m_db->getActivityLast7Hours();
    m_activityLabels.clear();
    m_activityData.clear();
    for (const QVariant &item : raw) {
        QVariantMap map = item.toMap();
        m_activityLabels.append(map.value("hour").toString());
        m_activityData.append(map.value("count").toInt());
    }
}

void DashboardStatsModel::clearEvents() {
    if (!m_db) return;
    m_db->clearEvents();
    refresh();
    emit eventsCleared();
}

void DashboardStatsModel::clearAlerts() {
    if (!m_db) return;
    m_db->clearAlerts();
    refresh();
    emit alertsCleared();
}