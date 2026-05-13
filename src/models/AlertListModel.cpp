#include "AlertListModel.h"
#include "../services/DatabaseService.h"
#include <QDebug>

AlertListModel::AlertListModel(DatabaseService *db, QObject *parent)
    : QAbstractListModel(parent), m_db(db)
{
    refresh();
}

int AlertListModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);
    return m_alerts.size();
}

QVariant AlertListModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_alerts.size()) return QVariant();
    const Alert &a = m_alerts[index.row()];
    switch (role) {
        case IdRole:             return a.id;
        case TitleRole:          return a.title;
        case DescriptionRole:    return a.description;
        case SeverityRole:       return a.severity;
        case StatusRole:         return a.status;
        case DeviceNameRole:     return a.deviceName;
        case TriggeredAtRole:    return a.triggeredAt.toString("yyyy-MM-ddTHH:mm:ss");
        case RuleIdRole:         return a.ruleId;
        case AssignedToRole:     return a.assignedTo;
        case CommentRole:        return a.comment;
        case RelatedEventIdsRole:return a.relatedEventIds;
        default:                 return QVariant();
    }
}

QHash<int, QByteArray> AlertListModel::roleNames() const {
    return {
        {IdRole,             "id"},
        {TitleRole,          "title"},
        {DescriptionRole,    "description"},
        {SeverityRole,       "severity"},
        {StatusRole,         "status"},
        {DeviceNameRole,     "deviceName"},
        {TriggeredAtRole,    "triggeredAt"},
        {RuleIdRole,         "ruleId"},
        {AssignedToRole,     "assignedTo"},
        {CommentRole,        "comment"},
        {RelatedEventIdsRole,"relatedEventIds"}
    };
}

void AlertListModel::refresh() {

    QVector<Alert> newAlerts = m_db->getAllAlerts();
    qDebug() << "[AlertListModel] refresh() —" << newAlerts.size() << "алертов";

    beginResetModel();
    m_alerts = newAlerts;
    endResetModel();

    int newTotal = m_db->getAlertCount();
    if (newTotal != m_totalCount) {
        m_totalCount = newTotal;
        emit totalCountChanged();
    }
}

void AlertListModel::appendNew() {

    QVector<Alert> fresh = m_db->getAllAlerts();
    if (fresh.isEmpty()) return;

    const Alert &newest = fresh.first(); 

    if (!m_alerts.isEmpty() && m_alerts.first().id == newest.id) return;

    beginInsertRows(QModelIndex(), 0, 0);
    m_alerts.prepend(newest);
    endInsertRows();

    m_totalCount++;
    emit totalCountChanged();
}

int AlertListModel::totalCount() const {
    return m_totalCount;
}

void AlertListModel::updateStatus(const QString &id, const QString &status) {
    for (int i = 0; i < m_alerts.size(); i++) {
        if (m_alerts[i].id == id) {
            m_alerts[i].status = status;
            m_db->updateAlert(m_alerts[i]);
            QModelIndex idx = index(i);
            emit dataChanged(idx, idx, {StatusRole});
            return;
        }
    }
}