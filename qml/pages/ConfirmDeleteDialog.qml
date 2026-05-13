import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../styles"

Dialog {
    id: confirmDeleteDialog
    modal: true
    standardButtons: Dialog.NoButton
    width: 400
    height: 280
    anchors.centerIn: Overlay.overlay

    property var authManager: null
    property var userToDelete: null

    Theme { id: theme }

    Overlay.modal: Rectangle {
        color: Qt.rgba(0, 0, 0, 0.6)
    }

    enter: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: theme.animNormal; easing.type: Easing.OutCubic }
            NumberAnimation { property: "scale"; from: 0.95; to: 1; duration: theme.animNormal; easing.type: Easing.OutCubic }
        }
    }

    exit: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; from: 1; to: 0; duration: theme.animFast; easing.type: Easing.InCubic }
            NumberAnimation { property: "scale"; from: 1; to: 0.95; duration: theme.animFast; easing.type: Easing.InCubic }
        }
    }

    background: Rectangle {
        color: theme.bgSecondary
        radius: theme.radiusLarge
        border.color: Qt.rgba(0.973, 0.318, 0.286, 0.3)
        border.width: 1
    }

    header: Rectangle {
        width: parent.width
        height: 56
        color: "transparent"

        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 1
            color: theme.border
        }

        Text {
            anchors.centerIn: parent
            text: "Подтверждение удаления"
            color: theme.danger
            font.pixelSize: theme.fontSizeLG
            font.bold: true
            font.family: theme.fontFamily
        }
    }

    ColumnLayout {
        width: parent.width
        spacing: 14
        anchors.margins: 24

        // Предупреждение
        Rectangle {
            Layout.fillWidth: true
            height: warningCol.implicitHeight + 24
            color: Qt.rgba(0.973, 0.318, 0.286, 0.08)
            radius: theme.radiusMedium
            border.color: Qt.rgba(0.973, 0.318, 0.286, 0.2)
            border.width: 1

            Column {
                id: warningCol
                anchors.centerIn: parent
                width: parent.width - 32
                spacing: 6

                Text {
                    width: parent.width
                    text: "Вы собираетесь удалить пользователя:"
                    color: theme.textSecondary
                    font.pixelSize: theme.fontSizeSM
                    font.family: theme.fontFamily
                    horizontalAlignment: Text.AlignHCenter
                }

                Text {
                    width: parent.width
                    text: userToDelete ? userToDelete.username : ""
                    color: theme.textPrimary
                    font.pixelSize: theme.fontSizeLG
                    font.bold: true
                    font.family: theme.fontFamily
                    horizontalAlignment: Text.AlignHCenter
                }

                Text {
                    width: parent.width
                    text: "Это действие необратимо."
                    color: theme.danger
                    font.pixelSize: theme.fontSizeXS
                    font.family: theme.fontFamily
                    font.letterSpacing: 0.3
                    horizontalAlignment: Text.AlignHCenter
                    opacity: 0.8
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: errorText.text.length > 0 ? errorText.implicitHeight + 20 : 0
            visible: errorText.text.length > 0
            color: Qt.rgba(0.973, 0.318, 0.286, 0.08)
            radius: theme.radiusMedium
            border.color: Qt.rgba(0.973, 0.318, 0.286, 0.25)
            border.width: 1
            clip: true
            Behavior on Layout.preferredHeight { NumberAnimation { duration: theme.animNormal; easing.type: Easing.OutCubic } }

            Text {
                id: errorText
                anchors.centerIn: parent
                text: ""
                color: theme.danger
                font.pixelSize: theme.fontSizeSM
                font.family: theme.fontFamily
                width: parent.width - 20
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
            }
        }

        Item { Layout.fillHeight: true }

        // Кнопки
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Rectangle {
                Layout.fillWidth: true
                height: 40
                radius: theme.radiusMedium
                color: cancelDelMouse.pressed       ? theme.bgTertiary
                     : cancelDelMouse.containsMouse ? theme.bgTertiary : "transparent"
                border.color: theme.border
                border.width: 1
                Behavior on color { ColorAnimation { duration: theme.animFast } }

                MouseArea {
                    id: cancelDelMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        clearForm()
                        confirmDeleteDialog.close()
                    }
                }

                Text {
                    anchors.centerIn: parent
                    text: "Отмена"
                    color: theme.textSecondary
                    font.pixelSize: theme.fontSizeSM
                    font.family: theme.fontFamily
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 40
                radius: theme.radiusMedium
                color: confirmDelMouse.pressed       ? theme.dangerDark
                     : confirmDelMouse.containsMouse ? Qt.lighter(theme.danger, 1.1) : theme.danger
                Behavior on color { ColorAnimation { duration: theme.animFast } }

                MouseArea {
                    id: confirmDelMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (!confirmDeleteDialog.authManager) {
                            errorText.text = "Ошибка: authManager не инициализирован"
                            return
                        }
                        if (!userToDelete) {
                            errorText.text = "Ошибка: пользователь не выбран"
                            return
                        }
                        errorText.text = ""
                        confirmDeleteDialog.authManager.deleteUser(userToDelete.id)
                    }
                }

                Text {
                    anchors.centerIn: parent
                    text: "Удалить"
                    color: "#0d1117"
                    font.pixelSize: theme.fontSizeSM
                    font.bold: true
                    font.family: theme.fontFamily
                }
            }
        }
    }

    function clearForm() {
        errorText.text = ""
        userToDelete = null
    }

    function showError(message) {
        errorText.text = message
    }

    onOpened: {
        errorText.text = ""
    }
}