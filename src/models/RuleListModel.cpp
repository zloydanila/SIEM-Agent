#include "RuleListModel.h"
#include "../services/DatabaseService.h"
#include <QUuid>

RuleListModel::RuleListModel(DatabaseService *db, QObject *parent)
    : QAbstractListModel(parent), m_db(db) {
    if (m_db) {
        m_db->seedDefaultRules();
        refresh();
    }
}

int RuleListModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return m_rules.size();
}

QVariant RuleListModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rules.size())
        return {};

    const Rule &r = m_rules[index.row()];

    switch (role) {
    case IdRole: return r.id;
    case NameRole: return r.name;
    case RuleTypeRole: return r.ruleType;
    case MatchEventTypeRole: return r.matchEventType;
    case SecondaryEventTypeRole: return r.secondaryEventType;
    case ThresholdRole: return r.threshold;
    case WindowSecondsRole: return r.windowSeconds;
    case CooldownSecondsRole: return r.cooldownSeconds;
    case AlertSeverityRole: return r.alertSeverity;
    case AlertTitleRole: return r.alertTitle;
    case AlertDescriptionRole: return r.alertDescription;
    case IsEnabledRole: return r.isEnabled;
    }
    return {};
}

QHash<int, QByteArray> RuleListModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole] = "ruleId";
    roles[NameRole] = "name";
    roles[RuleTypeRole] = "ruleType";
    roles[MatchEventTypeRole] = "matchEventType";
    roles[SecondaryEventTypeRole] = "secondaryEventType";
    roles[ThresholdRole] = "threshold";
    roles[WindowSecondsRole] = "windowSeconds";
    roles[CooldownSecondsRole] = "cooldownSeconds";
    roles[AlertSeverityRole] = "alertSeverity";
    roles[AlertTitleRole] = "alertTitle";
    roles[AlertDescriptionRole] = "alertDescription";
    roles[IsEnabledRole] = "isEnabled";
    return roles;
}

void RuleListModel::refresh() {
    if (!m_db) return;
    beginResetModel();
    m_rules = m_db->getAllRules();
    endResetModel();
}

bool RuleListModel::addRule(const QString &name, const QString &ruleType,
                             const QString &matchEventType, const QString &secondaryEventType,
                             int threshold, int windowSeconds, int cooldownSeconds,
                             const QString &alertSeverity, const QString &alertTitle,
                             const QString &alertDescription) {
    Rule rule;
    rule.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    rule.name = name;
    rule.ruleType = ruleType;
    rule.matchEventType = matchEventType;
    rule.secondaryEventType = secondaryEventType;
    rule.threshold = threshold;
    rule.windowSeconds = windowSeconds;
    rule.cooldownSeconds = cooldownSeconds;
    rule.alertSeverity = alertSeverity;
    rule.alertTitle = alertTitle;
    rule.alertDescription = alertDescription;
    rule.isEnabled = true;

    if (!m_db->createRule(rule)) return false;
    refresh();
    return true;
}

bool RuleListModel::removeRule(const QString &ruleId) {
    if (!m_db->deleteRule(ruleId)) return false;
    refresh();
    return true;
}

bool RuleListModel::toggleRule(const QString &ruleId, bool enabled) {
    if (!m_db->setRuleEnabled(ruleId, enabled)) return false;
    refresh();
    return true;
}