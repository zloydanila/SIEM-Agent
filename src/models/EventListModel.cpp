#include "EventListModel.h"
#include "../services/DatabaseService.h"

EventListModel::EventListModel(DatabaseService *db, QObject *parent)
    : QAbstractListModel(parent), m_db(db)
{
    refresh();
}

int EventListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_events.size();
}

QVariant EventListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_events.size())
        return QVariant();

    const Event &e = m_events.at(index.row());
    switch (role) {
    case IdRole: return e.id;
    case DeviceNameRole: return e.deviceName;
    case EventTypeRole: return e.eventType;
    case ActionRole: return e.action;
    case SeverityRole: return e.severity;
    case TimestampRole: return e.timestamp.toString(Qt::ISODate);
    case RawLogRole: return e.rawLog;
    case LocationRole: return e.location;
    default: return QVariant();
    }
}

QHash<int, QByteArray> EventListModel::roleNames() const
{
    return {
        {IdRole, "id"},
        {DeviceNameRole, "deviceName"},
        {EventTypeRole, "eventType"},
        {ActionRole, "action"},
        {SeverityRole, "severity"},
        {TimestampRole, "timestamp"},
        {RawLogRole, "rawLog"},
        {LocationRole, "location"},
    };
}

void EventListModel::refresh()
{
    beginResetModel();
    m_events.clear();
    m_lastId.clear();
    m_lastTimestamp = QDateTime();
    m_hasMore = false;
    m_totalCount = 0;
    endResetModel();
    emit hasMoreChanged();
    emit totalCountChanged();
    loadMore();
}

void EventListModel::appendNew()
{
    if (!m_db)
        return;

    QVector<Event> fresh = m_db->getEventsPaged(1, QString(), QDateTime());
    if (fresh.isEmpty())
        return;

    if (!m_events.isEmpty() && m_events.first().id == fresh.first().id)
        return;

    beginInsertRows(QModelIndex(), 0, 0);
    m_events.prepend(fresh.first());
    endInsertRows();

    m_totalCount++;
    emit totalCountChanged();
}

void EventListModel::loadMore()
{
    if (!m_db)
        return;

    QVector<Event> page = m_db->getEventsPaged(PAGE_SIZE, m_lastId, m_lastTimestamp);

    if (m_events.isEmpty()) {
        m_totalCount = m_db->getTotalEventsCount();
        emit totalCountChanged();
    }

    if (page.isEmpty()) {
        m_hasMore = false;
        emit hasMoreChanged();
        return;
    }

    beginInsertRows(QModelIndex(), m_events.size(), m_events.size() + page.size() - 1);
    m_events.append(page);
    endInsertRows();

    const Event &last = m_events.last();
    m_lastId = last.id;
    m_lastTimestamp = last.timestamp;

    m_hasMore = (page.size() == PAGE_SIZE);
    emit hasMoreChanged();
}

bool EventListModel::hasMore() const
{
    return m_hasMore;
}

int EventListModel::totalCount() const
{
    return m_totalCount;
}

int EventListModel::count()
{
    return m_events.size();
}