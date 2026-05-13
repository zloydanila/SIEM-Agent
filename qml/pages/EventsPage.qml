import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../styles"
import Utils 1.0

Item {
    id: eventsPage

    Theme { id: theme }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        RowLayout {
            Layout.fillWidth: true

            Column {
                spacing: 4
                Text {
                    text: "События"
                    color: theme.textPrimary
                    font.pixelSize: theme.fontSizeXL
                    font.bold: true
                    font.family: theme.fontFamily
                }
                Row {
                    spacing: 6
                    Text {
                        text: "Загружено: " + eventsList.count
                        color: theme.textMuted
                        font.pixelSize: theme.fontSizeSM
                        font.family: theme.fontFamily
                    }
                    Text {
                        visible: eventListModel && eventListModel.hasMore
                        text: "из " + (eventListModel ? eventListModel.totalCount : 0)
                        color: theme.textMuted
                        font.pixelSize: theme.fontSizeSM
                        font.family: theme.fontFamily
                    }
                }
            }

            Item { Layout.fillWidth: true }

            Rectangle {
                width: 110; height: 36
                radius: theme.radiusMedium
                color: refreshMouse.pressed       ? theme.accentDark
                     : refreshMouse.containsMouse ? Qt.lighter(theme.accent, 1.1)
                     : theme.accent
                Behavior on color { ColorAnimation { duration: theme.animFast } }

                MouseArea {
                    id: refreshMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: eventListModel.refresh()
                }
                Text {
                    anchors.centerIn: parent
                    text: "Обновить"
                    color: theme.bgPrimary
                    font.pixelSize: theme.fontSizeSM
                    font.bold: true
                    font.family: theme.fontFamily
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
                visible: eventsList.count === 0

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "Событий пока нет"
                    color: theme.textPrimary
                    font.pixelSize: theme.fontSizeLG
                    font.bold: true
                    font.family: theme.fontFamily
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "Запустите simulator.py для генерации событий"
                    color: theme.textMuted
                    font.pixelSize: theme.fontSizeSM
                    font.family: theme.fontFamily
                }
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: 8

                ListView {
                    id: eventsList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 8
                    model: eventListModel
                    boundsBehavior: Flickable.StopAtBounds
                    flickDeceleration: 3000

                    ScrollBar.vertical: ScrollBar {
                        policy: ScrollBar.AsNeeded
                        minimumSize: 0.05
                    }

                    delegate: Rectangle {
                        width: eventsList.width
                        height: cardContent.implicitHeight + 24
                        radius: theme.radiusMedium
                        color: cardMouse.containsMouse ? theme.bgTertiary : theme.bgSecondary
                        border.color: theme.border
                        border.width: 1
                        Behavior on color { ColorAnimation { duration: theme.animFast } }

                        MouseArea { id: cardMouse; anchors.fill: parent; hoverEnabled: true }

                        RowLayout {
                            id: cardContent
                            anchors { left: parent.left; right: parent.right; top: parent.top; margins: 16 }
                            spacing: 14

                            Rectangle {
                                width: 4
                                height: cardContent.implicitHeight + 8
                                radius: 2
                                color: Utils.severityColor(model.severity)
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 6

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 10

                                    Text {
                                        text: model.deviceName ?? ""
                                        color: theme.textPrimary
                                        font.pixelSize: theme.fontSizeSM
                                        font.bold: true
                                        font.family: theme.fontFamily
                                    }
                                    Text { text: "—"; color: theme.textMuted; font.pixelSize: theme.fontSizeSM; font.family: theme.fontFamily }
                                    Text {
                                        text: model.action ?? ""
                                        color: theme.textPrimary
                                        font.pixelSize: theme.fontSizeSM
                                        font.family: theme.fontFamily
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                    }

                                    Rectangle {
                                        width: sevBadge.implicitWidth + 16; height: 22
                                        radius: theme.radiusFull
                                        color: Qt.rgba(
                                            Qt.color(Utils.severityColor(model.severity)).r,
                                            Qt.color(Utils.severityColor(model.severity)).g,
                                            Qt.color(Utils.severityColor(model.severity)).b, 0.15)
                                        Text {
                                            id: sevBadge
                                            anchors.centerIn: parent
                                            text: Utils.severityLabel(model.severity ?? "")
                                            color: Utils.severityColor(model.severity)
                                            font.pixelSize: theme.fontSizeXS
                                            font.bold: true
                                            font.family: theme.fontFamily
                                            font.letterSpacing: 0.8
                                        }
                                    }
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 16
                                    Text { text: "Место: " + (model.location ?? ""); color: theme.textMuted; font.pixelSize: theme.fontSizeXS; font.family: theme.fontFamily }
                                    Text { text: "Время: " + Utils.formatTime(model.timestamp ?? ""); color: theme.textMuted; font.pixelSize: theme.fontSizeXS; font.family: theme.fontFamily }
                                    Item { Layout.fillWidth: true }
                                }

                                Text {
                                    Layout.fillWidth: true
                                    text: model.rawLog ?? ""
                                    color: theme.textMuted
                                    font.pixelSize: theme.fontSizeXS
                                    font.family: theme.fontFamily
                                    font.italic: true
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                    height: eventListModel && eventListModel.hasMore ? 48 : 0
                    visible: eventListModel && eventListModel.hasMore
                    clip: true

                    Behavior on height { NumberAnimation { duration: theme.animNormal; easing.type: Easing.OutCubic } }

                    Rectangle {
                        anchors.centerIn: parent
                        width: loadMoreText.implicitWidth + 32; height: 34
                        radius: theme.radiusMedium
                        color: loadMouse.containsMouse ? theme.bgTertiary : theme.bgSecondary
                        border.color: theme.border; border.width: 1
                        Behavior on color { ColorAnimation { duration: theme.animFast } }

                        MouseArea {
                            id: loadMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: eventListModel.loadMore()
                        }
                        Text {
                            id: loadMoreText
                            anchors.centerIn: parent
                            text: "Загрузить ещё 100"
                            color: theme.textSecondary
                            font.pixelSize: theme.fontSizeSM
                            font.family: theme.fontFamily
                        }
                    }
                }
            }
        }
    }
}