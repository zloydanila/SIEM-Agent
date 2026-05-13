#include "AlertFilterProxyModel.h"
#include "AlertListModel.h"

AlertFilterProxyModel::AlertFilterProxyModel(QObject *parent) : QSortFilterProxyModel(parent), m_statusFilter(""){
}

QString AlertFilterProxyModel::statusFilter() const{
    return m_statusFilter;
}

void AlertFilterProxyModel::setStatusFilter(const QString &filter){
    if(m_statusFilter != filter){
        m_statusFilter = filter;
        emit statusFilterChanged();
        invalidateFilter(); 
    }
}

bool AlertFilterProxyModel::filterAcceptsRow(int sourceRow,
                                              const QModelIndex &sourceParent) const {
    if (m_statusFilter.isEmpty()) return true;

    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    QString status = sourceModel()->data(index, AlertListModel::StatusRole).toString();
    return (status == m_statusFilter);
}
