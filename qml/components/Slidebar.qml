import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../styles"

Rectangle {
    id: sidebar
    width: 220
    color: theme.bgSecondary
    border.color: theme.border
    border.width: 1

    Theme { id: theme }

    property int currentIndex: 0
    signal pageChanged(int index)
    signal logoutRequested()
    property var currentUser: null

    property var menuItems: [
        { icon: "⊞", label: "Дашборд"   },
        { icon: "📋", label: "События"   },
        { icon: "🔔", label: "Алерты"    },
        { icon: "⚙️",  label: "Настройки" },
    ]

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ── Логотип ────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 64
            color: "transparent"

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: theme.border
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 10

                Rectangle {
                    width: 32
                    height: 32
                    radius: theme.radiusSmall
                    color: theme.accent

                    Text {
                        anchors.centerIn: parent
                        text: "S"
                        color: theme.bgPrimary
                        font.pixelSize: 16
                        font.bold: true
                        font.family: theme.fontFamily
                    }
                }

                Column {
                    spacing: 2

                    Text {
                        text: "SIEM Agent"
                        color: theme.textPrimary
                        font.pixelSize: theme.fontSizeSM
                        font.bold: true
                        font.family: theme.fontFamily
                    }

                    Text {
                        text: "Industrial Security"
                        color: theme.textMuted
                        font.pixelSize: theme.fontSizeXS
                        font.family: theme.fontFamily
                    }
                }
            }
        }

        // ── Пункты меню ────────────────────────────────────────
        Item { Layout.preferredHeight: 8 }

        Repeater {
            model: sidebar.menuItems

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 44
                radius: theme.radiusSmall
                color: {
                    if (sidebar.currentIndex === index)
                        return Qt.rgba(
                            Qt.color(theme.accent).r,
                            Qt.color(theme.accent).g,
                            Qt.color(theme.accent).b,
                            0.12
                        )
                    if (menuMouse.containsMouse)
                        return theme.bgTertiary
                    return "transparent"
                }
                Behavior on color { ColorAnimation { duration: theme.animFast } }

                Rectangle {
                    width: 3
                    height: 24
                    radius: 2
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    color: theme.accent
                    visible: sidebar.currentIndex === index
                }

                MouseArea {
                    id: menuMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        sidebar.currentIndex = index
                        sidebar.pageChanged(index)
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 12
                    spacing: 12

                    Text {
                        text: modelData.icon
                        font.pixelSize: 16
                        font.family: theme.fontFamily
                    }

                    Text {
                        text: modelData.label
                        color: sidebar.currentIndex === index ? theme.accent : theme.textSecondary
                        font.pixelSize: theme.fontSizeSM
                        font.family: theme.fontFamily
                        font.bold: sidebar.currentIndex === index
                        Layout.fillWidth: true
                        Behavior on color { ColorAnimation { duration: theme.animFast } }
                    }
                }
            }
        }

        Item { Layout.fillHeight: true }

        // ── Разделитель ────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: theme.border
        }

        // ── Пользователь + выход ───────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 64
            color: "transparent"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 12
                spacing: 10

                Rectangle {
                    width: 32
                    height: 32
                    radius: theme.radiusFull
                    color: theme.bgTertiary
                    border.color: theme.border
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        text: (sidebar.currentUser?.fullName ?? "U").charAt(0).toUpperCase()
                        color: theme.textPrimary
                        font.pixelSize: theme.fontSizeSM
                        font.bold: true
                        font.family: theme.fontFamily
                    }
                }

                Column {
                    spacing: 2
                    Layout.fillWidth: true

                    Text {
                        text: sidebar.currentUser?.fullName ?? "Пользователь"
                        color: theme.textPrimary
                        font.pixelSize: theme.fontSizeXS
                        font.bold: true
                        font.family: theme.fontFamily
                        elide: Text.ElideRight
                        width: parent.width
                    }

                    Text {
                        text: sidebar.currentUser?.role ?? ""
                        color: theme.textMuted
                        font.pixelSize: theme.fontSizeXS
                        font.family: theme.fontFamily
                    }
                }

                Rectangle {
                    width: 28
                    height: 28
                    radius: theme.radiusSmall
                    color: logoutMouse.containsMouse ? theme.bgTertiary : "transparent"
                    border.color: logoutMouse.containsMouse ? theme.border : "transparent"
                    Behavior on color { ColorAnimation { duration: theme.animFast } }

                    MouseArea {
                        id: logoutMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: sidebar.logoutRequested()
                    }

                    Text {
                        anchors.centerIn: parent
                        text: "→"
                        color: theme.textMuted
                        font.pixelSize: 16
                        font.family: theme.fontFamily
                    }
                }
            }
        }
    }
}