#ifndef REPORTSERVICE_H
#define REPORTSERVICE_H

#include <QObject>
#include <QString>

class DatabaseService;

class ReportService : public QObject {
    Q_OBJECT

public:
    explicit ReportService(DatabaseService *db, QObject *parent = nullptr);

    Q_INVOKABLE bool exportEventsCsv(const QString &filePath);
    Q_INVOKABLE bool exportAlertsCsv(const QString &filePath);
    Q_INVOKABLE bool exportReportJson(const QString &filePath);
    Q_INVOKABLE QString defaultExportPath(const QString &name) const;

signals:
    void exportSuccess(const QString &filePath);
    void exportFailed(const QString &reason);

private:
    DatabaseService *m_db;
};

#endif