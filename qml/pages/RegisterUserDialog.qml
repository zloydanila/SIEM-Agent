import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../styles"

Dialog {
    id: registerDialog
    modal: true
    standardButtons: Dialog.NoButton
    width: 460
    height: 640
    anchors.centerIn: Overlay.overlay

    property var authManager: null

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
        border.color: theme.border
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

            Text {
                text: "Создать пользователя"
                color: theme.textPrimary
                font.pixelSize: theme.fontSizeLG
                font.bold: true
                font.family: theme.fontFamily
            }

            Item { Layout.fillWidth: true }

            Rectangle {
                width: 28
                height: 28
                radius: theme.radiusSmall
                color: closeRegMouse.containsMouse ? theme.bgTertiary : "transparent"
                border.color: closeRegMouse.containsMouse ? theme.border : "transparent"
                Behavior on color { ColorAnimation { duration: theme.animFast } }

                MouseArea {
                    id: closeRegMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        clearForm()
                        registerDialog.close()
                    }
                }

                Text {
                    anchors.centerIn: parent
                    text: "×"
                    color: theme.textMuted
                    font.pixelSize: 18
                    font.family: theme.fontFamily
                }
            }
        }
    }

    ColumnLayout {
        width: parent.width
        spacing: 14
        anchors.margins: 20

        // Имя пользователя
        Column {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: "ИМЯ ПОЛЬЗОВАТЕЛЯ"
                color: theme.textMuted
                font.pixelSize: theme.fontSizeXS
                font.family: theme.fontFamily
                font.letterSpacing: 1.5
                font.bold: true
            }

            Rectangle {
                width: parent.width
                height: 40
                color: usernameField.activeFocus ? theme.bgTertiary : theme.bgSecondary
                radius: theme.radiusMedium
                border.color: usernameField.activeFocus ? theme.accent : theme.border
                border.width: 1
                Behavior on color { ColorAnimation { duration: theme.animNormal } }
                Behavior on border.color { ColorAnimation { duration: theme.animNormal } }

                TextField {
                    id: usernameField
                    anchors.fill: parent
                    anchors.margins: 1
                    leftPadding: 12
                    placeholderText: "От 3 до 20 символов"
                    color: theme.textPrimary
                    font.pixelSize: theme.fontSizeSM
                    font.family: theme.fontFamily
                    selectByMouse: true
                    placeholderTextColor: theme.textMuted
                    background: Rectangle { color: "transparent" }
                    onTextChanged: errorText.text = ""
                }
            }
        }

        // Полное имя
        Column {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: "ПОЛНОЕ ИМЯ"
                color: theme.textMuted
                font.pixelSize: theme.fontSizeXS
                font.family: theme.fontFamily
                font.letterSpacing: 1.5
                font.bold: true
            }

            Rectangle {
                width: parent.width
                height: 40
                color: fullNameField.activeFocus ? theme.bgTertiary : theme.bgSecondary
                radius: theme.radiusMedium
                border.color: fullNameField.activeFocus ? theme.accent : theme.border
                border.width: 1
                Behavior on color { ColorAnimation { duration: theme.animNormal } }
                Behavior on border.color { ColorAnimation { duration: theme.animNormal } }

                TextField {
                    id: fullNameField
                    anchors.fill: parent
                    anchors.margins: 1
                    leftPadding: 12
                    placeholderText: "Иванов Иван Иванович"
                    color: theme.textPrimary
                    font.pixelSize: theme.fontSizeSM
                    font.family: theme.fontFamily
                    selectByMouse: true
                    placeholderTextColor: theme.textMuted
                    background: Rectangle { color: "transparent" }
                    onTextChanged: errorText.text = ""
                }
            }
        }

        // Email
        Column {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: "ЭЛЕКТРОННАЯ ПОЧТА"
                color: theme.textMuted
                font.pixelSize: theme.fontSizeXS
                font.family: theme.fontFamily
                font.letterSpacing: 1.5
                font.bold: true
            }

            Rectangle {
                width: parent.width
                height: 40
                color: emailField.activeFocus ? theme.bgTertiary : theme.bgSecondary
                radius: theme.radiusMedium
                border.color: emailField.activeFocus ? theme.accent : theme.border
                border.width: 1
                Behavior on color { ColorAnimation { duration: theme.animNormal } }
                Behavior on border.color { ColorAnimation { duration: theme.animNormal } }

                TextField {
                    id: emailField
                    anchors.fill: parent
                    anchors.margins: 1
                    leftPadding: 12
                    placeholderText: "user@example.com"
                    color: theme.textPrimary
                    font.pixelSize: theme.fontSizeSM
                    font.family: theme.fontFamily
                    selectByMouse: true
                    placeholderTextColor: theme.textMuted
                    background: Rectangle { color: "transparent" }
                    onTextChanged: errorText.text = ""
                }
            }
        }

        // Пароль
        Column {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: "ПАРОЛЬ"
                color: theme.textMuted
                font.pixelSize: theme.fontSizeXS
                font.family: theme.fontFamily
                font.letterSpacing: 1.5
                font.bold: true
            }

            Rectangle {
                width: parent.width
                height: 40
                color: passwordField.activeFocus ? theme.bgTertiary : theme.bgSecondary
                radius: theme.radiusMedium
                border.color: passwordField.activeFocus ? theme.accent : theme.border
                border.width: 1
                Behavior on color { ColorAnimation { duration: theme.animNormal } }
                Behavior on border.color { ColorAnimation { duration: theme.animNormal } }

                TextField {
                    id: passwordField
                    anchors.fill: parent
                    anchors.margins: 1
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

            // Индикатор надёжности пароля
            Rectangle {
                width: parent.width
                height: 3
                radius: 2
                color: theme.bgTertiary
                visible: passwordField.text.length > 0

                Rectangle {
                    height: parent.height
                    radius: parent.radius
                    color: {
                        if (passwordField.text.length < 6) return theme.danger
                        if (passwordField.text.length < 10) return theme.warning
                        return theme.success
                    }
                    width: {
                        if (passwordField.text.length < 6) return parent.width * 0.3
                        if (passwordField.text.length < 10) return parent.width * 0.65
                        return parent.width
                    }
                    Behavior on width { NumberAnimation { duration: theme.animNormal } }
                    Behavior on color { ColorAnimation { duration: theme.animNormal } }
                }
            }
        }

        // Подтверждение пароля
        Column {
            Layout.fillWidth: true
            spacing: 6

            RowLayout {
                Text {
                    text: "ПОДТВЕРЖДЕНИЕ ПАРОЛЯ"
                    color: theme.textMuted
                    font.pixelSize: theme.fontSizeXS
                    font.family: theme.fontFamily
                    font.letterSpacing: 1.5
                    font.bold: true
                }

                Text {
                    id: matchStatus
                    text: ""
                    font.pixelSize: theme.fontSizeXS
                    font.family: theme.fontFamily
                    font.italic: true
                }
            }

            Rectangle {
                width: parent.width
                height: 40
                color: confirmPasswordField.activeFocus ? theme.bgTertiary : theme.bgSecondary
                radius: theme.radiusMedium
                border.color: {
                    if (confirmPasswordField.text.length === 0)
                        return confirmPasswordField.activeFocus ? theme.accent : theme.border
                    return passwordField.text === confirmPasswordField.text ? theme.success : theme.danger
                }
                border.width: 1
                Behavior on border.color { ColorAnimation { duration: theme.animNormal } }

                TextField {
                    id: confirmPasswordField
                    anchors.fill: parent
                    anchors.margins: 1
                    leftPadding: 12
                    placeholderText: "Повторите пароль"
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
                        if (text.length === 0 || passwordField.text.length === 0) {
                            matchStatus.text = ""
                        } else if (passwordField.text === text) {
                            matchStatus.text = "  ✓ Совпадает"
                            matchStatus.color = theme.success
                        } else {
                            matchStatus.text = "  ✗ Не совпадает"
                            matchStatus.color = theme.danger
                        }
                    }
                }
            }
        }

        // Роль
        Column {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: "РОЛЬ"
                color: theme.textMuted
                font.pixelSize: theme.fontSizeXS
                font.family: theme.fontFamily
                font.letterSpacing: 1.5
                font.bold: true
            }

            ComboBox {
                id: roleCombo
                width: parent.width
                height: 40
                model: ["viewer", "operator", "admin"]
                currentIndex: 0

                contentItem: Text {
                    leftPadding: 12
                    text: {
                        if (roleCombo.currentText === "admin")    return "Администратор"
                        if (roleCombo.currentText === "operator") return "Оператор"
                        return "Наблюдатель"
                    }
                    color: theme.textPrimary
                    font.pixelSize: theme.fontSizeSM
                    font.family: theme.fontFamily
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    color: theme.bgSecondary
                    radius: theme.radiusMedium
                    border.color: roleCombo.popup.visible ? theme.accent : theme.border
                    border.width: 1
                    Behavior on border.color { ColorAnimation { duration: theme.animNormal } }
                }

                popup: Popup {
                    y: roleCombo.height + 4
                    width: roleCombo.width
                    padding: 4

                    background: Rectangle {
                        color: theme.bgPrimary
                        radius: theme.radiusMedium
                        border.color: theme.border
                        border.width: 1
                    }

                    contentItem: ListView {
                        clip: true
                        implicitHeight: contentHeight
                        model: roleCombo.popup.visible ? roleCombo.delegateModel : null
                        currentIndex: roleCombo.highlightedIndex
                        ScrollIndicator.vertical: ScrollIndicator {}
                    }
                }

                delegate: ItemDelegate {
                    width: roleCombo.width
                    height: 36

                    contentItem: Text {
                        leftPadding: 12
                        text: {
                            if (modelData === "admin")    return "Администратор"
                            if (modelData === "operator") return "Оператор"
                            return "Наблюдатель"
                        }
                        color: highlighted ? theme.accent : theme.textPrimary
                        font.pixelSize: theme.fontSizeSM
                        font.family: theme.fontFamily
                        verticalAlignment: Text.AlignVCenter
                        Behavior on color { ColorAnimation { duration: theme.animFast } }
                    }

                    highlighted: roleCombo.highlightedIndex === index

                    background: Rectangle {
                        color: highlighted ? Qt.rgba(0.345, 0.651, 1.0, 0.12) : "transparent"
                        radius: theme.radiusSmall
                        Behavior on color { ColorAnimation { duration: theme.animFast } }
                    }
                }
            }
        }

        // Блок ошибки
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
                color: cancelRegMouse.pressed       ? theme.bgTertiary
                     : cancelRegMouse.containsMouse ? theme.bgTertiary : "transparent"
                border.color: theme.border
                border.width: 1
                Behavior on color { ColorAnimation { duration: theme.animFast } }

                MouseArea {
                    id: cancelRegMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        clearForm()
                        registerDialog.close()
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

                property bool canRegister: passwordField.text === confirmPasswordField.text &&
                                           confirmPasswordField.text.length > 0 &&
                                           usernameField.text.trim().length >= 3 &&
                                           fullNameField.text.trim().length > 0

                color: !canRegister        ? theme.bgTertiary
                     : regMouse.pressed    ? theme.accentDark
                     : regMouse.containsMouse ? theme.accentHover : theme.accent

                Behavior on color { ColorAnimation { duration: theme.animFast } }

                MouseArea {
                    id: regMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: parent.canRegister ? Qt.PointingHandCursor : Qt.ArrowCursor
                    enabled: parent.canRegister
                    onClicked: validateAndCreate()
                }

                Text {
                    anchors.centerIn: parent
                    text: "Создать"
                    color: parent.canRegister ? theme.bgPrimary : theme.textMuted
                    font.pixelSize: theme.fontSizeSM
                    font.bold: true
                    font.family: theme.fontFamily
                }
            }
        }
    }

    function validateAndCreate() {
        var username = usernameField.text.trim()
        var fullName = fullNameField.text.trim()
        var email    = emailField.text.trim()
        var password = passwordField.text
        var confirmPassword = confirmPasswordField.text

        if (!username.length) {
            errorText.text = "Введите имя пользователя"
            return false
        }
        if (username.length < 3 || username.length > 20) {
            errorText.text = "Имя пользователя: от 3 до 20 символов"
            return false
        }
        var usernameRegex = /^[a-zA-Z0-9_]+$/
        if (!usernameRegex.test(username)) {
            errorText.text = "Только латинские буквы, цифры и подчёркивание"
            return false
        }
        if (!fullName.length) {
            errorText.text = "Введите полное имя"
            return false
        }
        if (email.length) {
            var emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/
            if (!emailRegex.test(email)) {
                errorText.text = "Неверный формат электронной почты"
                return false
            }
        }
        if (!password.length) {
            errorText.text = "Введите пароль"
            return false
        }
        if (password.length < 6) {
            errorText.text = "Пароль должен быть не менее 6 символов"
            return false
        }
        if (password !== confirmPassword) {
            errorText.text = "Пароли не совпадают"
            return false
        }
        if (!registerDialog.authManager) {
            errorText.text = "Ошибка: authManager не инициализирован"
            return false
        }

        errorText.text = ""
        registerDialog.authManager.registerUser(
            username, password,
            roleCombo.currentText,
            fullName, email
        )
        return true
    }

    function clearForm() {
        usernameField.text        = ""
        fullNameField.text        = ""
        emailField.text           = ""
        passwordField.text        = ""
        confirmPasswordField.text = ""
        roleCombo.currentIndex    = 0
        errorText.text            = ""
        matchStatus.text          = ""
    }

    function showError(message) {
        errorText.text = message
    }

    onOpened: {
        clearForm()
        usernameField.forceActiveFocus()
    }
}