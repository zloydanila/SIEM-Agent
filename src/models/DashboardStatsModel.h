#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariant>

class DatabaseService;

class DashboardStatsModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(int criticalCount READ criticalCount NOTIFY statsChanged)
    Q_PROPERTY(int highCount READ highCount NOTIFY statsChanged)
    Q_PROPERTY(int mediumCount READ mediumCount NOTIFY statsChanged)
    Q_PROPERTY(int lowCount READ lowCount NOTIFY statsChanged)
    Q_PROPERTY(int totalEvents READ totalEvents NOTIFY statsChanged)
    Q_PROPERTY(int totalAlerts READ totalAlerts NOTIFY statsChanged)
    Q_PROPERTY(int openAlerts READ openAlerts NOTIFY statsChanged)
    Q_PROPERTY(QVariantList topDevices READ topDevices NOTIFY statsChanged)
    Q_PROPERTY(QVariantList activityData READ activityData NOTIFY statsChanged)
    Q_PROPERTY(QStringList activityLabels READ activityLabels NOTIFY statsChanged)

public:
    explicit DashboardStatsModel(DatabaseService *dbService, QObject *parent = nullptr);

    int criticalCount() const { return m_criticalCount; }
    int highCount() const { return m_highCount; }
    int mediumCount() const { return m_mediumCount; }
    int lowCount() const { return m_lowCount; }
    int totalEvents() const { return m_totalEvents; }
    int totalAlerts() const { return m_totalAlerts; }
    int openAlerts() const { return m_openAlerts; }
    QVariantList topDevices() const { return m_topDevices; }
    QVariantList activityData() const { return m_activityData; }
    QStringList activityLabels() const { return m_activityLabels; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void clearEvents();
    Q_INVOKABLE void clearAlerts();

signals:
    void statsChanged();
    void eventsCleared();
    void alertsCleared();

private:
    void updateCounts();
    void updateTopDevices();
    void updateActivity();

    DatabaseService *m_db = nullptr;

    int m_criticalCount = 0;
    int m_highCount = 0;
    int m_mediumCount = 0;
    int m_lowCount = 0;
    int m_totalEvents = 0;
    int m_totalAlerts = 0;
    int m_openAlerts = 0;

    QVariantList m_topDevices;
    QVariantList m_activityData;
    QStringList m_activityLabels;
};