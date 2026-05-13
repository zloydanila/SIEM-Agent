import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../styles"

Dialog {
    id: editUserDialog
    modal: true
    standardButtons: Dialog.NoButton
    width: 460
    height: 600
    anchors.centerIn: Overlay.overlay

    property var authManager: null
    property var userToEdit: null
    property bool isActiveValue: true

    Theme { id: theme }

    background: Rectangle {
        color: theme.bgSecondary
        radius: theme.radiusLarge
        border.color: theme.border
        border.width: 1
    }

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

    header: Rectangle {
        width: parent.width
        height: 56
        color: "transparent"

        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 1
            color: theme.border
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 20

            Text {
                text: "Редактировать пользователя"
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
                color: closeMouse.containsMouse ? theme.bgTertiary : "transparent"
                border.color: closeMouse.containsMouse ? theme.border : "transparent"
                Behavior on color { ColorAnimation { duration: theme.animFast } }

                MouseArea {
                    id: closeMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        clearForm()
                        editUserDialog.close()
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
                    placeholderText: "Введите имя пользователя"
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

        // Статус
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Text {
                text: "СТАТУС"
                color: theme.textMuted
                font.pixelSize: theme.fontSizeXS
                font.family: theme.fontFamily
                font.letterSpacing: 1.5
                font.bold: true
            }

            Item { Layout.fillWidth: true }

            Text {
                text: isActiveToggle.checked ? "Активен" : "Отключён"
                color: isActiveToggle.checked ? theme.success : theme.textMuted
                font.pixelSize: theme.fontSizeSM
                font.family: theme.fontFamily
                font.bold: true
                Behavior on color { ColorAnimation { duration: theme.animNormal } }
            }

            Rectangle {
                width: 44; height: 24; radius: 12
                color: isActiveToggle.checked ? Qt.rgba(0.247, 0.725, 0.314, 0.3) : theme.bgTertiary
                border.color: isActiveToggle.checked ? theme.success : theme.border
                border.width: 1
                Behavior on color       { ColorAnimation { duration: theme.animNormal } }
                Behavior on border.color { ColorAnimation { duration: theme.animNormal } }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: isActiveToggle.checked = !isActiveToggle.checked
                }

                Rectangle {
                    width: 18; height: 18; radius: 9
                    color: isActiveToggle.checked ? theme.success : theme.textMuted
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: isActiveToggle.checked ? 23 : 3
                    Behavior on anchors.leftMargin { NumberAnimation { duration: theme.animNormal; easing.type: Easing.OutCubic } }
                    Behavior on color { ColorAnimation { duration: theme.animNormal } }
                }
            }

            // Невидимый CheckBox для хранения состояния тоггла
            CheckBox { id: isActiveToggle; visible: false; checked: true }
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
                color: cancelMouse.pressed       ? theme.bgTertiary
                     : cancelMouse.containsMouse ? theme.bgTertiary : "transparent"
                border.color: theme.border
                border.width: 1
                Behavior on color { ColorAnimation { duration: theme.animFast } }

                MouseArea {
                    id: cancelMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        clearForm()
                        editUserDialog.close()
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

                property bool saveEnabled: usernameField.text.trim().length > 0 &&
                                           fullNameField.text.trim().length > 0

                color: !saveEnabled           ? theme.bgTertiary
                     : saveMouse.pressed      ? theme.accentDark
                     : saveMouse.containsMouse ? theme.accentHover : theme.accent
                Behavior on color { ColorAnimation { duration: theme.animFast } }

                MouseArea {
                    id: saveMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: parent.saveEnabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                    enabled: parent.saveEnabled
                    onClicked: validateAndSave()
                }

                Text {
                    anchors.centerIn: parent
                    text: "Сохранить"
                    color: parent.saveEnabled ? theme.bgPrimary : theme.textMuted
                    font.pixelSize: theme.fontSizeSM
                    font.bold: true
                    font.family: theme.fontFamily
                }
            }
        }
    }

    function validateAndSave() {
        var username = usernameField.text.trim()
        var fullName = fullNameField.text.trim()
        var email    = emailField.text.trim()

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
            var emailRegex = /^[^\s@]+@[^\s@]+$/
            if (!emailRegex.test(email)) {
                errorText.text = "Неверный формат электронной почты"
                return false
            }
        }
        if (!editUserDialog.authManager) {
            errorText.text = "Ошибка: authManager не инициализирован"
            return false
        }

        errorText.text = ""
        editUserDialog.authManager.updateUser(
            editUserDialog.userToEdit.id,
            username,
            fullName,
            roleCombo.currentText,
            email,
            isActiveToggle.checked
        )
        return true
    }

    function loadUserData(user) {
        usernameField.text     = user.username
        fullNameField.text     = user.fullName
        emailField.text        = user.email
        roleCombo.currentIndex = roleCombo.model.indexOf(user.role)
        isActiveToggle.checked = user.isActive
        errorText.text         = ""
    }

    function clearForm() {
        usernameField.text     = ""
        fullNameField.text     = ""
        emailField.text        = ""
        roleCombo.currentIndex = 0
        isActiveToggle.checked = true
        errorText.text         = ""
    }

    function showError(message) {
        errorText.text = message
    }

    onOpened: {
        if (userToEdit) loadUserData(userToEdit)
    }
}