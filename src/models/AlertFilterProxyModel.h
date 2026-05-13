#ifndef ALERTFILTERPROXYMODEL_H
#define ALERTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>
#include <QString>


class AlertFilterProxyModel : public QSortFilterProxyModel{
    Q_OBJECT
    Q_PROPERTY(QString statusFilter READ statusFilter WRITE setStatusFilter NOTIFY statusFilterChanged)
    Q_PROPERTY(int count READ count NOTIFY statusFilterChanged)
public:
    explicit AlertFilterProxyModel(QObject *parent = nullptr);
    QString statusFilter() const;
    void setStatusFilter(const QString &filter);
    int count() const { return rowCount(); }

signals:
    void statusFilterChanged();
protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    QString m_statusFilter;
};

#endif