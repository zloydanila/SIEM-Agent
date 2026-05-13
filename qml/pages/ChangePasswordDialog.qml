import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../styles"

Dialog {
    id: changePasswordDialog
    modal: true
    standardButtons: Dialog.NoButton
    closePolicy: Popup.NoAutoClose   // нельзя закрыть кликом мимо
    width: 440
    height: 480
    anchors.centerIn: Overlay.overlay

    property var authManager: null
    property bool isForced: false   // true = обязательная смена, нельзя отменить

    Theme { id: theme }

    Overlay.modal: Rectangle {
        color: Qt.rgba(0, 0, 0, 0.7)
    }

    enter: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: theme.animNormal; easing.type: Easing.OutCubic }
            NumberAnimation { property: "scale"; from: 0.95; to: 1; duration: theme.animNormal; easing.type: Easing.OutCubic }
        }
    }

    background: Rectangle {
        color: theme.bgSecondary
        radius: theme.radiusLarge
        border.color: theme.warning
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

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 20

            Column {
                spacing: 2
                Text {
                    text: isForced ? "Требуется смена пароля" : "Смена пароля"
                    color: isForced ? theme.warning : theme.textPrimary
                    font.pixelSize: theme.fontSizeLG
                    font.bold: true
                    font.family: theme.fontFamily
                }
                Text {
                    visible: isForced
                    text: "Вы должны сменить пароль перед продолжением"
                    color: theme.textMuted
                    font.pixelSize: theme.fontSizeXS
                    font.family: theme.fontFamily
                }
            }
        }
    }

    ColumnLayout {
        width: parent.width
        spacing: 14
        anchors.margins: 20

        // Предупреждение для принудительной смены
        Rectangle {
            Layout.fillWidth: true
            height: warnText.implicitHeight + 20
            visible: isForced
            color: Qt.rgba(1.0, 0.647, 0.0, 0.08)
            radius: theme.radiusMedium
            border.color: Qt.rgba(1.0, 0.647, 0.0, 0.25)
            border.width: 1

            Text {
                id: warnText
                anchors.centerIn: parent
                width: parent.width - 24
                text: "Это стандартный пароль по умолчанию. Установите надёжный пароль для защиты системы."
                color: theme.warning
                font.pixelSize: theme.fontSizeXS
                font.family: theme.fontFamily
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
            }
        }

        // Текущий пароль
        Column {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: "ТЕКУЩИЙ ПАРОЛЬ"
                color: theme.textMuted
                font.pixelSize: theme.fontSizeXS
                font.family: theme.fontFamily
                font.letterSpacing: 1.5
                font.bold: true
            }

            Rectangle {
                width: parent.width
                height: 40
                color: currentPassField.activeFocus ? theme.bgTertiary : theme.bgSecondary
                radius: theme.radiusMedium
                border.color: currentPassField.activeFocus ? theme.accent : theme.border
                border.width: 1
                Behavior on color       { ColorAnimation { duration: theme.animNormal } }
                Behavior on border.color { ColorAnimation { duration: theme.animNormal } }

                TextField {
                    id: currentPassField
                    anchors.fill: parent; anchors.margins: 1
                    leftPadding: 12
                    placeholderText: "Введите текущий пароль"
                    color: theme.textPrimary
                    font.pixelSize: theme.fontSizeSM
                    font.family: theme.fontFamily
                    echoMode: TextInput.Password
                    selectByMouse: true
                    inputMethodHints: Qt.ImhSensitiveData | Qt.ImhNoPredictiveText
                    placeholderTextColor: theme.textMuted
                    background: Rectangle { color: "transparent" }
                    onTextChanged: errorText.text = ""
                }
            }
        }

        // Новый пароль
        Column {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: "НОВЫЙ ПАРОЛЬ"
                color: theme.textMuted
                font.pixelSize: theme.fontSizeXS
                font.family: theme.fontFamily
                font.letterSpacing: 1.5
                font.bold: true
            }

            Rectangle {
                width: parent.width
                height: 40
                color: newPassField.activeFocus ? theme.bgTertiary : theme.bgSecondary
                radius: theme.radiusMedium
                border.color: newPassField.activeFocus ? theme.accent : theme.border
                border.width: 1
                Behavior on color       { ColorAnimation { duration: theme.animNormal } }
                Behavior on border.color { ColorAnimation { duration: theme.animNormal } }

                TextField {
                    id: newPassField
                    anchors.fill: parent; anchors.margins: 1
                    leftPadding: 12
                    placeholderText: "Минимум 6 символов"
                    color: theme.textPrimary
                    font.pixelSize: theme.fontSizeSM
                    font.family: theme.fontFamily
                    echoMode: TextInput.Password
                    selectByMouse: true
                    inputMethodHints: Qt.ImhSensitiveData | Qt.ImhNoPredictiveText
                    placeholderTextColor: theme.textMuted
                    background: Rectangle { color: "transparent" }
                    onTextChanged: errorText.text = ""
                }
            }

            // Индикатор надёжности
            Rectangle {
                width: parent.width
                height: 3
                radius: 2
                color: theme.bgTertiary
                visible: newPassField.text.length > 0

                Rectangle {
                    height: parent.height; radius: parent.radius
                    color: {
                        if (newPassField.text.length < 6)  return theme.danger
                        if (newPassField.text.length < 10) return theme.warning
                        return theme.success
                    }
                    width: {
                        if (newPassField.text.length < 6)  return parent.width * 0.3
                        if (newPassField.text.length < 10) return parent.width * 0.65
                        return parent.width
                    }
                    Behavior on width { NumberAnimation { duration: theme.animNormal } }
                    Behavior on color { ColorAnimation { duration: theme.animNormal } }
                }
            }
        }

        // Подтверждение нового пароля
        Column {
            Layout.fillWidth: true
            spacing: 6

            RowLayout {
                Text {
                    text: "ПОДТВЕРЖДЕНИЕ"
                    color: theme.textMuted
                    font.pixelSize: theme.fontSizeXS
                    font.family: theme.fontFamily
                    font.letterSpacing: 1.5
                    font.bold: true
                }
                Text {
                    id: matchLabel
                    text: ""
                    font.pixelSize: theme.fontSizeXS
                    font.family: theme.fontFamily
                    font.italic: true
                }
            }

            Rectangle {
                width: parent.width
                height: 40
                color: confirmPassField.activeFocus ? theme.bgTertiary : theme.bgSecondary
                radius: theme.radiusMedium
                border.color: {
                    if (confirmPassField.text.length === 0)
                        return confirmPassField.activeFocus ? theme.accent : theme.border
                    return newPassField.text === confirmPassField.text ? theme.success : theme.danger
                }
                border.width: 1
                Behavior on border.color { ColorAnimation { duration: theme.animNormal } }

                TextField {
                    id: confirmPassField
                    anchors.fill: parent; anchors.margins: 1
                    leftPadding: 12
                    placeholderText: "Повторите новый пароль"
                    color: theme.textPrimary
                    font.pixelSize: theme.fontSizeSM
                    font.family: theme.fontFamily
                    echoMode: TextInput.Password
                    selectByMouse: true
                    inputMethodHints: Qt.ImhSensitiveData | Qt.ImhNoPredictiveText
                    placeholderTextColor: theme.textMuted
                    background: Rectangle { color: "transparent" }
                    onTextChanged: {
                        errorText.text = ""
                        if (text.length === 0 || newPassField.text.length === 0) {
                            matchLabel.text = ""
                        } else if (newPassField.text === text) {
                            matchLabel.text = "  ✓ Совпадает"
                            matchLabel.color = theme.success
                        } else {
                            matchLabel.text = "  ✗ Не совпадает"
                            matchLabel.color = theme.danger
                        }
                    }
                }
            }
        }

        // Ошибка
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
                visible: !isForced
                color: skipMouse.pressed       ? theme.bgTertiary
                     : skipMouse.containsMouse ? theme.bgTertiary : "transparent"
                border.color: theme.border
                border.width: 1
                Behavior on color { ColorAnimation { duration: theme.animFast } }

                MouseArea {
                    id: skipMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        clearForm()
                        changePasswordDialog.close()
                    }
                }

                Text {
                    anchors.centerIn: parent
                    text: "Пропустить"
                    color: theme.textSecondary
                    font.pixelSize: theme.fontSizeSM
                    font.family: theme.fontFamily
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 40
                radius: theme.radiusMedium

                property bool canSave: currentPassField.text.length > 0 &&
                                       newPassField.text.length >= 6 &&
                                       newPassField.text === confirmPassField.text

                color: !canSave             ? theme.bgTertiary
                     : savePwMouse.pressed  ? theme.accentDark
                     : savePwMouse.containsMouse ? theme.accentHover : theme.accent
                Behavior on color { ColorAnimation { duration: theme.animFast } }

                MouseArea {
                    id: savePwMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: parent.canSave ? Qt.PointingHandCursor : Qt.ArrowCursor
                    enabled: parent.canSave
                    onClicked: doChangePassword()
                }

                Text {
                    anchors.centerIn: parent
                    text: "Сменить пароль"
                    color: parent.canSave ? theme.bgPrimary : theme.textMuted
                    font.pixelSize: theme.fontSizeSM
                    font.bold: true
                    font.family: theme.fontFamily
                }
            }
        }
    }

    Connections {
        target: authManager

        function onPasswordChanged() {
            clearForm()
            changePasswordDialog.close()
        }

        function onErrorOccured(message) {
            errorText.text = message
        }
    }

    function doChangePassword() {
        var current = currentPassField.text
        var newPass = newPassField.text
        var confirm = confirmPassField.text

        if (!current.length) {
            errorText.text = "Введите текущий пароль"
            return
        }
        if (newPass.length < 6) {
            errorText.text = "Новый пароль должен быть не менее 6 символов"
            return
        }
        if (newPass !== confirm) {
            errorText.text = "Пароли не совпадают"
            return
        }
        if (newPass === current) {
            errorText.text = "Новый пароль должен отличаться от текущего"
            return
        }

        errorText.text = ""
        changePasswordDialog.authManager.changePassword(current, newPass)
    }

    function clearForm() {
        currentPassField.text  = ""
        newPassField.text      = ""
        confirmPassField.text  = ""
        errorText.text         = ""
        matchLabel.text        = ""
    }

    onOpened: {
        clearForm()
        currentPassField.forceActiveFocus()
    }
}