import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../styles"

Rectangle {
    id: loginPage
    anchors.fill: parent
    color: theme.bgPrimary

    signal loginAttempt(string username, string password)

    property bool isLoading: false

    Theme { id: theme }

    Connections {
        target: authManager

        function onLoginFailed(reason) {
            errorMessage.text = reason
            isLoading = false
            formColumn.shake()
        }

        function onLoginSuccess() {
            isLoading = false
            usernameInput.text = ""
            passwordInput.text = ""
            errorMessage.text  = ""
        }
    }

    Canvas {
        anchors.fill: parent
        opacity: 0.04

        onPaint: {
            var ctx = getContext("2d")
            ctx.strokeStyle = theme.textPrimary
            ctx.lineWidth = 1

            for (var x = 0; x < width; x += 40) {
                ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, height); ctx.stroke()
            }
            for (var y = 0; y < height; y += 40) {
                ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(width, y); ctx.stroke()
            }
        }
    }

    Column {
        id: formColumn
        anchors.centerIn: parent
        spacing: 12
        width: 360

        property real shakeOffset: 0
        anchors.horizontalCenterOffset: shakeOffset

        function shake() { shakeAnimation.start() }

        SequentialAnimation {
            id: shakeAnimation
            NumberAnimation { target: formColumn; property: "shakeOffset"; to: -10; duration: 50 }
            NumberAnimation { target: formColumn; property: "shakeOffset"; to:  10; duration: 50 }
            NumberAnimation { target: formColumn; property: "shakeOffset"; to:  -8; duration: 50 }
            NumberAnimation { target: formColumn; property: "shakeOffset"; to:   8; duration: 50 }
            NumberAnimation { target: formColumn; property: "shakeOffset"; to:   0; duration: 50 }
        }

        opacity: 0
        Component.onCompleted: appearAnimation.start()

        ParallelAnimation {
            id: appearAnimation
            NumberAnimation {
                target: formColumn; property: "opacity"
                from: 0; to: 1; duration: theme.animSlow; easing.type: Easing.OutCubic
            }
            NumberAnimation {
                target: formColumn; property: "anchors.verticalCenterOffset"
                from: 20; to: 0; duration: theme.animSlow; easing.type: Easing.OutCubic
            }
        }

        Rectangle {
            width: 64; height: 64
            radius: theme.radiusMedium
            color: theme.bgTertiary
            border.color: theme.border; border.width: 1
            anchors.horizontalCenter: parent.horizontalCenter

            Text {
                anchors.centerIn: parent
                text: "SIEM"
                color: theme.accent
                font.pixelSize: theme.fontSizeMD
                font.bold: true; font.family: theme.fontFamily; font.letterSpacing: 2
            }
        }

        Item { height: 8; width: 1 }

        Text {
            text: "Вход в систему"
            color: theme.textPrimary
            font.pixelSize: theme.fontSizeXXL
            font.bold: true; font.family: theme.fontFamily
            horizontalAlignment: Text.AlignHCenter
            width: parent.width
        }

        Text {
            text: "Система мониторинга промышленной безопасности"
            color: theme.textSecondary
            font.pixelSize: theme.fontSizeSM; font.family: theme.fontFamily
            horizontalAlignment: Text.AlignHCenter
            width: parent.width; font.letterSpacing: 0.5
        }

        Item { height: 12; width: 1 }

        // Имя пользователя
        Column {
            width: parent.width
            spacing: 6

            Text {
                text: "ИМЯ ПОЛЬЗОВАТЕЛЯ"
                color: theme.textMuted
                font.pixelSize: theme.fontSizeXS; font.family: theme.fontFamily
                font.letterSpacing: 1.5; font.bold: true
            }

            Rectangle {
                width: parent.width; height: 44
                color: usernameInput.activeFocus ? theme.bgTertiary : theme.bgSecondary
                radius: theme.radiusMedium
                border.color: usernameInput.activeFocus ? theme.accent : theme.border
                border.width: 1
                Behavior on color       { ColorAnimation { duration: theme.animNormal } }
                Behavior on border.color { ColorAnimation { duration: theme.animNormal } }

                TextField {
                    id: usernameInput
                    anchors.fill: parent; anchors.margins: 1
                    leftPadding: 14; rightPadding: 14
                    placeholderText: "Введите имя пользователя"
                    color: theme.textPrimary
                    font.pixelSize: theme.fontSizeMD; font.family: theme.fontFamily
                    enabled: !isLoading
                    selectByMouse: true
                    placeholderTextColor: theme.textMuted
                    background: Rectangle { color: "transparent" }
                    Keys.onReturnPressed: passwordInput.forceActiveFocus()
                    onTextChanged: errorMessage.text = ""
                }
            }
        }

        // Пароль
        Column {
            width: parent.width
            spacing: 6

            Text {
                text: "ПАРОЛЬ"
                color: theme.textMuted
                font.pixelSize: theme.fontSizeXS; font.family: theme.fontFamily
                font.letterSpacing: 1.5; font.bold: true
            }

            Rectangle {
                width: parent.width; height: 44
                color: passwordInput.activeFocus ? theme.bgTertiary : theme.bgSecondary
                radius: theme.radiusMedium
                border.color: passwordInput.activeFocus ? theme.accent : theme.border
                border.width: 1
                Behavior on color       { ColorAnimation { duration: theme.animNormal } }
                Behavior on border.color { ColorAnimation { duration: theme.animNormal } }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14; anchors.rightMargin: 8
                    spacing: 0

                    TextField {
                        id: passwordInput
                        Layout.fillWidth: true; Layout.fillHeight: true
                        placeholderText: "Введите пароль"
                        color: theme.textPrimary
                        font.pixelSize: theme.fontSizeMD; font.family: theme.fontFamily
                        echoMode: showPass.checked ? TextInput.Normal : TextInput.Password
                        enabled: !isLoading
                        selectByMouse: true
                        inputMethodHints: Qt.ImhSensitiveData | Qt.ImhNoPredictiveText
                        placeholderTextColor: theme.textMuted
                        background: Rectangle { color: "transparent" }
                        Keys.onReturnPressed: loginButton.doLogin()
                        onTextChanged: errorMessage.text = ""
                    }

                    CheckBox {
                        id: showPass
                        checked: false
                        indicator: Rectangle {
                            width: 32; height: 32
                            radius: theme.radiusSmall
                            color: showPass.hovered ? theme.bgTertiary : "transparent"
                            Behavior on color { ColorAnimation { duration: theme.animFast } }
                            Text {
                                anchors.centerIn: parent
                                text: showPass.checked ? "СКРЫТЬ" : "ПОКАЗ"
                                color: theme.textSecondary
                                font.pixelSize: 9; font.bold: true; font.letterSpacing: 0.5
                            }
                        }
                    }
                }
            }
        }

        // Ошибка
        Rectangle {
            width: parent.width
            height: errorMessage.text.length > 0 ? errorMessage.implicitHeight + 16 : 0
            color: Qt.rgba(0.973, 0.318, 0.286, 0.1)
            radius: theme.radiusMedium
            border.color: Qt.rgba(0.973, 0.318, 0.286, 0.3); border.width: 1
            clip: true
            Behavior on height { NumberAnimation { duration: theme.animNormal; easing.type: Easing.OutCubic } }

            Text {
                id: errorMessage
                anchors.centerIn: parent; text: ""
                color: theme.danger
                font.pixelSize: theme.fontSizeSM; font.family: theme.fontFamily
                width: parent.width - 20
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
            }
        }

        Item { height: 4; width: 1 }

        // Кнопка входа
        Rectangle {
            id: loginButton
            width: parent.width; height: 44
            radius: theme.radiusMedium
            color: {
                if (!enabled)                    return theme.bgTertiary
                if (loginMouseArea.pressed)      return theme.accentDark
                if (loginMouseArea.containsMouse) return theme.accentHover
                return theme.accent
            }
            enabled: !isLoading
            Behavior on color { ColorAnimation { duration: theme.animFast } }

            property alias pressed: loginMouseArea.pressed

            function doLogin() {
                if (usernameInput.text.trim().length === 0) {
                    errorMessage.text = "Введите имя пользователя"
                    usernameInput.forceActiveFocus()
                    return
                }
                if (passwordInput.text.length === 0) {
                    errorMessage.text = "Введите пароль"
                    passwordInput.forceActiveFocus()
                    return
                }
                errorMessage.text = ""
                isLoading = true
                loginAttempt(usernameInput.text.trim(), passwordInput.text)
            }

            MouseArea {
                id: loginMouseArea
                anchors.fill: parent
                hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                onClicked: loginButton.doLogin()
            }

            RowLayout {
                anchors.centerIn: parent
                spacing: 10

                BusyIndicator {
                    Layout.preferredWidth: 20; Layout.preferredHeight: 20
                    running: isLoading; visible: isLoading
                }

                Text {
                    text: isLoading ? "Выполняется вход..." : "Войти"
                    color: loginButton.enabled ? theme.bgPrimary : theme.textMuted
                    font.pixelSize: theme.fontSizeMD; font.family: theme.fontFamily
                    font.bold: true; font.letterSpacing: 0.5
                }
            }
        }

        Item { height: 4; width: 1 }
    }

    Text {
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin: 20
        text: "SIEM Agent v1.0.0"
        color: theme.textMuted
        font.pixelSize: theme.fontSizeXS; font.family: theme.fontFamily; font.letterSpacing: 1
    }
}