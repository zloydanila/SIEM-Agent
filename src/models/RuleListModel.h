#ifndef RULELISTMODEL_H
#define RULELISTMODEL_H

#include <QAbstractListModel>
#include <QVector>
#include "../engine/CorrelationEngine.h"

class DatabaseService;

class RuleListModel : public QAbstractListModel{
    Q_OBJECT

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        RuleTypeRole,
        MatchEventTypeRole,
        SecondaryEventTypeRole,
        ThresholdRole,
        WindowSecondsRole,
        CooldownSecondsRole,
        AlertSeverityRole,
        AlertTitleRole,
        AlertDescriptionRole,
        IsEnabledRole
    };

    explicit RuleListModel(DatabaseService *db, QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE bool addRule(const QString &name, const QString &ruleType, const QString &matchEventType, const QString &secondaryEventType,
                            int threshold,  int windowSeconds, int cooldownSeconds, const QString &alertSeverity, 
                            const QString &alertTitle, const QString &alertDisctiption);

    Q_INVOKABLE bool removeRule(const QString &ruleId);
    Q_INVOKABLE bool toggleRule(const QString &ruleId, bool enabled);

private:
    DatabaseService *m_db;
    QVector<Rule> m_rules;
};

#endif