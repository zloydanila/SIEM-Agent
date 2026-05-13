import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../styles"
import Utils 1.0

Item {
    id: alertsPage

    Theme { id: theme }

    property string activeFilter: ""
    property string currentUserRole: "viewer"
    property bool canManage: currentUserRole === "admin" || currentUserRole === "operator"

    function statusColor(s) {
        if (s === "open")          return theme.danger
        if (s === "investigating") return theme.warning
        if (s === "closed")        return theme.success
        return theme.textMuted
    }
    function statusLabel(s) {
        if (s === "open")          return "Открыт"
        if (s === "investigating") return "Изучается"
        if (s === "closed")        return "Закрыт"
        return s
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        RowLayout {
            Layout.fillWidth: true

            Column {
                spacing: 4
                Text {
                    text: "Алерты"
                    color: theme.textPrimary
                    font.pixelSize: theme.fontSizeXL
                    font.bold: true
                    font.family: theme.fontFamily
                }
                Row {
                    spacing: 6
                    Text {
                        text: "Всего: " + (alertListModel ? alertListModel.totalCount : 0)
                        color: theme.textMuted
                        font.pixelSize: theme.fontSizeSM
                        font.family: theme.fontFamily
                    }
                    Text {
                        visible: alertsPage.activeFilter !== ""
                        text: "· показано: " + alertsList.count
                        color: theme.accent
                        font.pixelSize: theme.fontSizeSM
                        font.family: theme.fontFamily
                    }
                }
            }

            Item { Layout.fillWidth: true }

            Rectangle {
                visible: !alertsPage.canManage
                width: roleLabel.implicitWidth + 20; height: 28
                radius: theme.radiusFull
                color: Qt.rgba(0.345, 0.651, 1.0, 0.12)
                border.color: Qt.rgba(0.345, 0.651, 1.0, 0.3); border.width: 1
                Text {
                    id: roleLabel; anchors.centerIn: parent
                    text: "Только просмотр"
                    color: theme.roleViewer
                    font.pixelSize: theme.fontSizeXS; font.bold: true; font.family: theme.fontFamily
                }
            }

            Item { width: 8; visible: !alertsPage.canManage }

            Row {
                spacing: 8
                Repeater {
                    model: [
                        { label: "Все",       value: ""              },
                        { label: "Открытые",  value: "open"          },
                        { label: "Изучаются", value: "investigating" },
                        { label: "Закрытые",  value: "closed"        }
                    ]
                    Rectangle {
                        width: filterText.implicitWidth + 24; height: 32
                        radius: theme.radiusFull
                        color: alertsPage.activeFilter === modelData.value
                               ? theme.accent
                               : filterMouse.containsMouse ? theme.bgTertiary : theme.bgSecondary
                        border.color: alertsPage.activeFilter === modelData.value ? theme.accent : theme.border
                        border.width: 1
                        Behavior on color { ColorAnimation { duration: theme.animFast } }

                        MouseArea {
                            id: filterMouse; anchors.fill: parent
                            hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                alertsPage.activeFilter = modelData.value
                                if (filteredAlertModel)
                                    filteredAlertModel.statusFilter = modelData.value
                            }
                        }
                        Text {
                            id: filterText; anchors.centerIn: parent
                            text: modelData.label
                            color: alertsPage.activeFilter === modelData.value ? theme.bgPrimary : theme.textSecondary
                            font.pixelSize: theme.fontSizeXS
                            font.bold: alertsPage.activeFilter === modelData.value
                            font.family: theme.fontFamily
                        }
                    }
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: theme.border }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Column {
                anchors.centerIn: parent
                spacing: 12
                visible: alertsList.count === 0

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: alertsPage.activeFilter !== "" ? "Нет алертов с таким статусом" : "Алертов нет"
                    color: theme.textPrimary
                    font.pixelSize: theme.fontSizeLG; font.bold: true; font.family: theme.fontFamily
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "Critical и High события автоматически создают алерты"
                    color: theme.textMuted
                    font.pixelSize: theme.fontSizeSM; font.family: theme.fontFamily
                    visible: alertsPage.activeFilter === ""
                }
            }

            ListView {
                id: alertsList
                anchors.fill: parent
                clip: true
                spacing: 8
                model: filteredAlertModel
                boundsBehavior: Flickable.StopAtBounds
                flickDeceleration: 2500
                maximumFlickVelocity: 1500

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                    minimumSize: 0.05
                }

                delegate: Rectangle {
                    id: alertDelegate
                    width: alertsList.width
                    height: alertContent.implicitHeight + 24
                    radius: theme.radiusMedium
                    color: cardMouse.containsMouse ? theme.bgTertiary : theme.bgSecondary
                    border.color: theme.border; border.width: 1
                    clip: true
                    Behavior on color { ColorAnimation { duration: theme.animFast } }

                    readonly property string alertId:       model.id          ?? ""
                    readonly property string alertStatus:   model.status      ?? ""
                    readonly property string alertSeverity: model.severity    ?? ""
                    readonly property string alertTitle:    model.title       ?? ""
                    readonly property string alertDesc:     model.description ?? ""
                    readonly property string alertDevice:   model.deviceName  ?? ""
                    readonly property string alertTime:     model.triggeredAt ?? ""

                    MouseArea { id: cardMouse; anchors.fill: parent; hoverEnabled: true }

                    RowLayout {
                        id: alertContent
                        anchors { left: parent.left; right: parent.right; top: parent.top; margins: 16 }
                        spacing: 14

                        Rectangle {
                            width: 4
                            height: alertContent.implicitHeight + 8
                            radius: 2
                            color: Utils.severityColor(alertDelegate.alertSeverity)
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                Text {
                                    text: alertDelegate.alertTitle
                                    color: theme.textPrimary
                                    font.pixelSize: theme.fontSizeSM; font.bold: true; font.family: theme.fontFamily
                                    Layout.fillWidth: true; elide: Text.ElideRight
                                }

                                Rectangle {
                                    width: sevText.implicitWidth + 16; height: 22
                                    radius: theme.radiusFull
                                    color: Qt.rgba(Qt.color(Utils.severityColor(alertDelegate.alertSeverity)).r,
                                                   Qt.color(Utils.severityColor(alertDelegate.alertSeverity)).g,
                                                   Qt.color(Utils.severityColor(alertDelegate.alertSeverity)).b, 0.15)
                                    Text {
                                        id: sevText; anchors.centerIn: parent
                                        text: Utils.severityLabel(alertDelegate.alertSeverity)
                                        color: Utils.severityColor(alertDelegate.alertSeverity)
                                        font.pixelSize: theme.fontSizeXS; font.bold: true; font.family: theme.fontFamily
                                    }
                                }

                                Rectangle {
                                    width: statusText.implicitWidth + 16; height: 22
                                    radius: theme.radiusFull
                                    color: Qt.rgba(Qt.color(statusColor(alertDelegate.alertStatus)).r,
                                                   Qt.color(statusColor(alertDelegate.alertStatus)).g,
                                                   Qt.color(statusColor(alertDelegate.alertStatus)).b, 0.15)
                                    Text {
                                        id: statusText; anchors.centerIn: parent
                                        text: statusLabel(alertDelegate.alertStatus)
                                        color: statusColor(alertDelegate.alertStatus)
                                        font.pixelSize: theme.fontSizeXS; font.bold: true; font.family: theme.fontFamily
                                    }
                                }
                            }

                            Text {
                                Layout.fillWidth: true
                                text: alertDelegate.alertDesc
                                color: theme.textSecondary
                                font.pixelSize: theme.fontSizeXS; font.family: theme.fontFamily
                                wrapMode: Text.WordWrap
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 16

                                Text { text: "Устройство: " + alertDelegate.alertDevice; color: theme.textMuted; font.pixelSize: theme.fontSizeXS; font.family: theme.fontFamily }
                                Text { text: "Время: " + Utils.formatTime(alertDelegate.alertTime); color: theme.textMuted; font.pixelSize: theme.fontSizeXS; font.family: theme.fontFamily }

                                Item { Layout.fillWidth: true }

                                Row {
                                    spacing: 6
                                    visible: alertsPage.canManage && alertDelegate.alertStatus !== "closed"

                                    Rectangle {
                                        width: btn1text.implicitWidth + 20; height: 26
                                        radius: theme.radiusSmall
                                        visible: alertDelegate.alertStatus === "open"
                                        color: btn1mouse.containsMouse ? theme.bgTertiary : "transparent"
                                        border.color: theme.border; border.width: 1
                                        Behavior on color { ColorAnimation { duration: theme.animFast } }
                                        MouseArea {
                                            id: btn1mouse; anchors.fill: parent
                                            hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                                            onClicked: alertListModel.updateStatus(alertDelegate.alertId, "investigating")
                                        }
                                        Text { id: btn1text; anchors.centerIn: parent; text: "Взять в работу"; color: theme.warning; font.pixelSize: theme.fontSizeXS; font.family: theme.fontFamily }
                                    }

                                    Rectangle {
                                        width: btn2text.implicitWidth + 20; height: 26
                                        radius: theme.radiusSmall
                                        color: btn2mouse.containsMouse ? theme.bgTertiary : "transparent"
                                        border.color: theme.border; border.width: 1
                                        Behavior on color { ColorAnimation { duration: theme.animFast } }
                                        MouseArea {
                                            id: btn2mouse; anchors.fill: parent
                                            hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                                            onClicked: alertListModel.updateStatus(alertDelegate.alertId, "closed")
                                        }
                                        Text { id: btn2text; anchors.centerIn: parent; text: "Закрыть"; color: theme.success; font.pixelSize: theme.fontSizeXS; font.family: theme.fontFamily }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}