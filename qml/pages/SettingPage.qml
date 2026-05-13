import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../styles"

Item {
    id: settingsPage

    property var currentUserRole: "viewer"
    property bool canManage: currentUserRole === "admin"
    property var changePasswordDialog: null

    Theme { id: theme }

    Dialog {
        id: addRuleDialog
        modal: true
        standardButtons: Dialog.NoButton
        width: 560
        height: Math.min(760, Screen.height * 0.9)
        anchors.centerIn: Overlay.overlay

        Overlay.modal: Rectangle { color: Qt.rgba(0, 0, 0, 0.6) }

        enter: Transition {
            ParallelAnimation {
                NumberAnimation { property: "opacity"; from: 0; to: 1; duration: theme.animNormal; easing.type: Easing.OutCubic }
                NumberAnimation { property: "scale"; from: 0.96; to: 1; duration: theme.animNormal; easing.type: Easing.OutCubic }
            }
        }
        exit: Transition {
            ParallelAnimation {
                NumberAnimation { property: "opacity"; from: 1; to: 0; duration: theme.animFast; easing.type: Easing.InCubic }
                NumberAnimation { property: "scale"; from: 1; to: 0.96; duration: theme.animFast; easing.type: Easing.InCubic }
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
                spacing: 12

                Text {
                    text: "Новое правило корреляции"
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
                    color: closeRuleMouse.containsMouse ? theme.bgTertiary : "transparent"
                    border.color: closeRuleMouse.containsMouse ? theme.border : "transparent"
                    Behavior on color { ColorAnimation { duration: theme.animFast } }

                    MouseArea {
                        id: closeRuleMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: { clearRuleForm(); addRuleDialog.close() }
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

        contentItem: ScrollView {
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            ColumnLayout {
                width: addRuleDialog.width - 32
                spacing: 14
                anchors.margins: 16

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Text {
                        text: "НАЗВАНИЕ ПРАВИЛА"
                        color: theme.textMuted
                        font.pixelSize: theme.fontSizeXS
                        font.bold: true
                        font.family: theme.fontFamily
                        font.letterSpacing: 1.5
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 40
                        radius: theme.radiusMedium
                        color: ruleNameField.activeFocus ? theme.bgTertiary : theme.bgSecondary
                        border.color: ruleNameField.activeFocus ? theme.accent : theme.border
                        border.width: 1

                        Behavior on color { ColorAnimation { duration: theme.animNormal } }
                        Behavior on border.color { ColorAnimation { duration: theme.animNormal } }

                        TextField {
                            id: ruleNameField
                            anchors.fill: parent
                            anchors.margins: 1
                            leftPadding: 12
                            placeholderText: "Например: Брутфорс SSH"
                            color: theme.textPrimary
                            font.pixelSize: theme.fontSizeSM
                            font.family: theme.fontFamily
                            selectByMouse: true
                            placeholderTextColor: theme.textMuted
                            background: Rectangle { color: "transparent" }
                            onTextChanged: ruleErrorText.text = ""
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Text {
                        text: "ТИП ПРАВИЛА"
                        color: theme.textMuted
                        font.pixelSize: theme.fontSizeXS
                        font.bold: true
                        font.family: theme.fontFamily
                        font.letterSpacing: 1.5
                    }

                    ComboBox {
                        id: ruleTypeCombo
                        Layout.fillWidth: true
                        height: 40
                        model: ["threshold", "correlation"]

                        contentItem: Text {
                            leftPadding: 12
                            text: ruleTypeCombo.currentText === "threshold"
                                  ? "Порог (threshold)"
                                  : "Корреляция (correlation)"
                            color: theme.textPrimary
                            font.pixelSize: theme.fontSizeSM
                            font.family: theme.fontFamily
                            verticalAlignment: Text.AlignVCenter
                        }

                        background: Rectangle {
                            color: theme.bgSecondary
                            radius: theme.radiusMedium
                            border.color: ruleTypeCombo.popup.visible ? theme.accent : theme.border
                            border.width: 1
                        }

                        popup: Popup {
                            y: ruleTypeCombo.height + 4
                            width: ruleTypeCombo.width
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
                                model: ruleTypeCombo.popup.visible ? ruleTypeCombo.delegateModel : null
                                currentIndex: ruleTypeCombo.highlightedIndex
                            }
                        }

                        delegate: ItemDelegate {
                            width: ruleTypeCombo.width
                            height: 36
                            contentItem: Text {
                                leftPadding: 12
                                text: modelData === "threshold"
                                      ? "Порог (threshold)"
                                      : "Корреляция (correlation)"
                                color: highlighted ? theme.accent : theme.textPrimary
                                font.pixelSize: theme.fontSizeSM
                                font.family: theme.fontFamily
                                verticalAlignment: Text.AlignVCenter
                            }
                            highlighted: ruleTypeCombo.highlightedIndex === index
                            background: Rectangle {
                                color: highlighted ? Qt.rgba(0.345, 0.651, 1.0, 0.12) : "transparent"
                                radius: theme.radiusSmall
                            }
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: ruleTypeCombo.currentText === "threshold"
                              ? "Срабатывает когда N событий одного типа за X секунд"
                              : "Срабатывает когда событие A происходит после события B"
                        color: theme.textMuted
                        font.pixelSize: theme.fontSizeXS
                        font.family: theme.fontFamily
                        font.italic: true
                        wrapMode: Text.WordWrap
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    visible: ruleTypeCombo.currentText === "threshold"

                    Text {
                        text: "ТИП СОБЫТИЯ"
                        color: theme.textMuted
                        font.pixelSize: theme.fontSizeXS
                        font.bold: true
                        font.family: theme.fontFamily
                        font.letterSpacing: 1.5
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 40
                        radius: theme.radiusMedium
                        color: matchEventField.activeFocus ? theme.bgTertiary : theme.bgSecondary
                        border.color: matchEventField.activeFocus ? theme.accent : theme.border
                        border.width: 1

                        TextField {
                            id: matchEventField
                            anchors.fill: parent
                            anchors.margins: 1
                            leftPadding: 12
                            placeholderText: "Например: auth_failure"
                            color: theme.textPrimary
                            font.pixelSize: theme.fontSizeSM
                            font.family: theme.fontFamily
                            selectByMouse: true
                            placeholderTextColor: theme.textMuted
                            background: Rectangle { color: "transparent" }
                            onTextChanged: ruleErrorText.text = ""
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    visible: ruleTypeCombo.currentText === "correlation"

                    Text {
                        text: "ОСНОВНОЕ СОБЫТИЕ (триггер)"
                        color: theme.textMuted
                        font.pixelSize: theme.fontSizeXS
                        font.bold: true
                        font.family: theme.fontFamily
                        font.letterSpacing: 1.5
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 40
                        radius: theme.radiusMedium
                        color: matchEventField.activeFocus ? theme.bgTertiary : theme.bgSecondary
                        border.color: matchEventField.activeFocus ? theme.accent : theme.border
                        border.width: 1

                        TextField {
                            id: matchEventFieldCor
                            anchors.fill: parent
                            anchors.margins: 1
                            leftPadding: 12
                            placeholderText: "Например: auth_failure"
                            color: theme.textPrimary
                            font.pixelSize: theme.fontSizeSM
                            font.family: theme.fontFamily
                            selectByMouse: true
                            placeholderTextColor: theme.textMuted
                            background: Rectangle { color: "transparent" }
                            onTextChanged: ruleErrorText.text = ""
                        }
                    }

                    Text {
                        text: "ВТОРИЧНОЕ СОБЫТИЕ (предшествующее)"
                        color: theme.textMuted
                        font.pixelSize: theme.fontSizeXS
                        font.bold: true
                        font.family: theme.fontFamily
                        font.letterSpacing: 1.5
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 40
                        radius: theme.radiusMedium
                        color: secondaryEventField.activeFocus ? theme.bgTertiary : theme.bgSecondary
                        border.color: secondaryEventField.activeFocus ? theme.accent : theme.border
                        border.width: 1

                        TextField {
                            id: secondaryEventField
                            anchors.fill: parent
                            anchors.margins: 1
                            leftPadding: 12
                            placeholderText: "Например: auth_failure"
                            color: theme.textPrimary
                            font.pixelSize: theme.fontSizeSM
                            font.family: theme.fontFamily
                            selectByMouse: true
                            placeholderTextColor: theme.textMuted
                            background: Rectangle { color: "transparent" }
                            onTextChanged: ruleErrorText.text = ""
                        }
                    }
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 3
                    columnSpacing: 10
                    rowSpacing: 6

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Text {
                            text: "ПОРОГ"
                            color: theme.textMuted
                            font.pixelSize: theme.fontSizeXS
                            font.bold: true
                            font.family: theme.fontFamily
                            font.letterSpacing: 1.5
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 40
                            radius: theme.radiusMedium
                            color: thresholdField.activeFocus ? theme.bgTertiary : theme.bgSecondary
                            border.color: thresholdField.activeFocus ? theme.accent : theme.border
                            border.width: 1

                            TextField {
                                id: thresholdField
                                anchors.fill: parent
                                anchors.margins: 1
                                leftPadding: 12
                                placeholderText: "1"
                                text: "1"
                                color: theme.textPrimary
                                font.pixelSize: theme.fontSizeSM
                                font.family: theme.fontFamily
                                selectByMouse: true
                                placeholderTextColor: theme.textMuted
                                inputMethodHints: Qt.ImhDigitsOnly
                                validator: IntValidator { bottom: 1; top: 9999 }
                                background: Rectangle { color: "transparent" }
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Text {
                            text: "ОКНО (сек)"
                            color: theme.textMuted
                            font.pixelSize: theme.fontSizeXS
                            font.bold: true
                            font.family: theme.fontFamily
                            font.letterSpacing: 1.5
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 40
                            radius: theme.radiusMedium
                            color: windowField.activeFocus ? theme.bgTertiary : theme.bgSecondary
                            border.color: windowField.activeFocus ? theme.accent : theme.border
                            border.width: 1

                            TextField {
                                id: windowField
                                anchors.fill: parent
                                anchors.margins: 1
                                leftPadding: 12
                                placeholderText: "60"
                                text: "60"
                                color: theme.textPrimary
                                font.pixelSize: theme.fontSizeSM
                                font.family: theme.fontFamily
                                selectByMouse: true
                                placeholderTextColor: theme.textMuted
                                inputMethodHints: Qt.ImhDigitsOnly
                                validator: IntValidator { bottom: 1; top: 86400 }
                                background: Rectangle { color: "transparent" }
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Text {
                            text: "COOLDOWN (сек)"
                            color: theme.textMuted
                            font.pixelSize: theme.fontSizeXS
                            font.bold: true
                            font.family: theme.fontFamily
                            font.letterSpacing: 1.5
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 40
                            radius: theme.radiusMedium
                            color: cooldownField.activeFocus ? theme.bgTertiary : theme.bgSecondary
                            border.color: cooldownField.activeFocus ? theme.accent : theme.border
                            border.width: 1

                            TextField {
                                id: cooldownField
                                anchors.fill: parent
                                anchors.margins: 1
                                leftPadding: 12
                                placeholderText: "60"
                                text: "60"
                                color: theme.textPrimary
                                font.pixelSize: theme.fontSizeSM
                                font.family: theme.fontFamily
                                selectByMouse: true
                                placeholderTextColor: theme.textMuted
                                inputMethodHints: Qt.ImhDigitsOnly
                                validator: IntValidator { bottom: 1; top: 86400 }
                                background: Rectangle { color: "transparent" }
                            }
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    ColumnLayout {
                        Layout.preferredWidth: 150
                        spacing: 6

                        Text {
                            text: "УРОВЕНЬ УГРОЗЫ"
                            color: theme.textMuted
                            font.pixelSize: theme.fontSizeXS
                            font.bold: true
                            font.family: theme.fontFamily
                            font.letterSpacing: 1.5
                        }

                        ComboBox {
                            id: severityCombo
                            Layout.fillWidth: true
                            height: 40
                            model: ["low", "medium", "high", "critical"]
                            currentIndex: 2

                            contentItem: Text {
                                leftPadding: 12
                                text: severityCombo.currentText
                                color: {
                                    if (severityCombo.currentText === "critical") return theme.danger
                                    if (severityCombo.currentText === "high") return "#f97316"
                                    if (severityCombo.currentText === "medium") return theme.warning
                                    return theme.success
                                }
                                font.pixelSize: theme.fontSizeSM
                                font.bold: true
                                font.family: theme.fontFamily
                                verticalAlignment: Text.AlignVCenter
                            }

                            background: Rectangle {
                                color: theme.bgSecondary
                                radius: theme.radiusMedium
                                border.color: severityCombo.popup.visible ? theme.accent : theme.border
                                border.width: 1
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Text {
                            text: "ЗАГОЛОВОК АЛЕРТА"
                            color: theme.textMuted
                            font.pixelSize: theme.fontSizeXS
                            font.bold: true
                            font.family: theme.fontFamily
                            font.letterSpacing: 1.5
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 40
                            radius: theme.radiusMedium
                            color: alertTitleField.activeFocus ? theme.bgTertiary : theme.bgSecondary
                            border.color: alertTitleField.activeFocus ? theme.accent : theme.border
                            border.width: 1

                            TextField {
                                id: alertTitleField
                                anchors.fill: parent
                                anchors.margins: 1
                                leftPadding: 12
                                placeholderText: "Заголовок создаваемого алерта"
                                color: theme.textPrimary
                                font.pixelSize: theme.fontSizeSM
                                font.family: theme.fontFamily
                                selectByMouse: true
                                placeholderTextColor: theme.textMuted
                                background: Rectangle { color: "transparent" }
                                onTextChanged: ruleErrorText.text = ""
                            }
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Text {
                        text: "ОПИСАНИЕ АЛЕРТА"
                        color: theme.textMuted
                        font.pixelSize: theme.fontSizeXS
                        font.bold: true
                        font.family: theme.fontFamily
                        font.letterSpacing: 1.5
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 40
                        radius: theme.radiusMedium
                        color: alertDescField.activeFocus ? theme.bgTertiary : theme.bgSecondary
                        border.color: alertDescField.activeFocus ? theme.accent : theme.border
                        border.width: 1

                        TextField {
                            id: alertDescField
                            anchors.fill: parent
                            anchors.margins: 1
                            leftPadding: 12
                            placeholderText: "Описание алерта (необязательно)"
                            color: theme.textPrimary
                            font.pixelSize: theme.fontSizeSM
                            font.family: theme.fontFamily
                            selectByMouse: true
                            placeholderTextColor: theme.textMuted
                            background: Rectangle { color: "transparent" }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    visible: ruleErrorText.text.length > 0
                    implicitHeight: visible ? ruleErrorText.implicitHeight + 16 : 0
                    color: Qt.rgba(0.973, 0.318, 0.286, 0.08)
                    radius: theme.radiusMedium
                    border.color: Qt.rgba(0.973, 0.318, 0.286, 0.25)
                    border.width: 1

                    Text {
                        id: ruleErrorText
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

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Rectangle {
                        Layout.fillWidth: true
                        height: 40
                        radius: theme.radiusMedium
                        color: cancelRuleMouse.containsMouse ? theme.bgTertiary : "transparent"
                        border.color: theme.border
                        border.width: 1

                        MouseArea {
                            id: cancelRuleMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: { clearRuleForm(); addRuleDialog.close() }
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

                        property bool canAdd: {
                            if (ruleNameField.text.trim().length === 0) return false;
                            if (alertTitleField.text.trim().length === 0) return false;

                            if (ruleTypeCombo.currentText === "threshold") {
                                return matchEventField.text.trim().length > 0;
                            } else {
                                // correlation — проверяем оба поля
                                return matchEventFieldCor.text.trim().length > 0
                                    && secondaryEventField.text.trim().length > 0;
                            }
                        }

                        color: !canAdd ? theme.bgTertiary
                             : saveRuleMouse.pressed ? theme.accentDark
                             : saveRuleMouse.containsMouse ? theme.accentHover : theme.accent

                        MouseArea {
                            id: saveRuleMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: parent.canAdd ? Qt.PointingHandCursor : Qt.ArrowCursor
                            enabled: parent.canAdd
                            onClicked: saveRule()
                        }

                        Text {
                            anchors.centerIn: parent
                            text: "Добавить правило"
                            color: parent.canAdd ? theme.bgPrimary : theme.textMuted
                            font.pixelSize: theme.fontSizeSM
                            font.bold: true
                            font.family: theme.fontFamily
                        }
                    }
                }

                Item { Layout.preferredHeight: 4 }
            }
        }
    }

    Dialog {
        id: deleteRuleDialog
        modal: true
        standardButtons: Dialog.NoButton
        width: 360
        height: 200
        anchors.centerIn: Overlay.overlay
        property string ruleId: ""
        property string ruleName: ""

        Overlay.modal: Rectangle { color: Qt.rgba(0, 0, 0, 0.6) }

        background: Rectangle {
            color: theme.bgSecondary
            radius: theme.radiusLarge
            border.color: Qt.rgba(0.973, 0.318, 0.286, 0.3)
            border.width: 1
        }

        ColumnLayout {
            width: parent.width
            anchors.margins: 24
            spacing: 16

            Text {
                Layout.fillWidth: true
                text: "Удалить правило «" + deleteRuleDialog.ruleName + "»?"
                color: theme.textPrimary
                font.pixelSize: theme.fontSizeMD
                font.bold: true
                font.family: theme.fontFamily
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
            }

            Text {
                Layout.fillWidth: true
                text: "Это действие необратимо."
                color: theme.danger
                font.pixelSize: theme.fontSizeXS
                font.family: theme.fontFamily
                horizontalAlignment: Text.AlignHCenter
            }

            Item { Layout.fillHeight: true }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Rectangle {
                    Layout.fillWidth: true
                    height: 38
                    radius: theme.radiusMedium
                    color: cancelDelRuleMouse.containsMouse ? theme.bgTertiary : "transparent"
                    border.color: theme.border
                    border.width: 1
                    Behavior on color { ColorAnimation { duration: theme.animFast } }

                    MouseArea {
                        id: cancelDelRuleMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: deleteRuleDialog.close()
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
                    height: 38
                    radius: theme.radiusMedium
                    color: confirmDelRuleMouse.pressed ? Qt.darker(theme.danger, 1.2) : confirmDelRuleMouse.containsMouse ? Qt.lighter(theme.danger, 1.1) : theme.danger
                    Behavior on color { ColorAnimation { duration: theme.animFast } }

                    MouseArea {
                        id: confirmDelRuleMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            ruleListModel.removeRule(deleteRuleDialog.ruleId)
                            deleteRuleDialog.close()
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
    }

    Dialog {
        id: clearDataDialog
        modal: true
        standardButtons: Dialog.NoButton
        width: 360
        height: 220
        anchors.centerIn: Overlay.overlay
        property string dataType: ""

        Overlay.modal: Rectangle { color: Qt.rgba(0, 0, 0, 0.6) }

        background: Rectangle {
            color: theme.bgSecondary
            radius: theme.radiusLarge
            border.color: Qt.rgba(0.973, 0.318, 0.286, 0.3)
            border.width: 1
        }

        ColumnLayout {
            width: parent.width
            anchors.margins: 24
            spacing: 12

            Text {
                Layout.fillWidth: true
                text: clearDataDialog.dataType === "events" ? "Очистить все события?" : "Очистить все алерты?"
                color: theme.textPrimary
                font.pixelSize: theme.fontSizeMD
                font.bold: true
                font.family: theme.fontFamily
                horizontalAlignment: Text.AlignHCenter
            }

            Text {
                Layout.fillWidth: true
                text: "Все записи будут удалены из базы данных без возможности восстановления."
                color: theme.textMuted
                font.pixelSize: theme.fontSizeXS
                font.family: theme.fontFamily
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
            }

            Text {
                Layout.fillWidth: true
                text: "Это действие необратимо!"
                color: theme.danger
                font.pixelSize: theme.fontSizeXS
                font.bold: true
                font.family: theme.fontFamily
                horizontalAlignment: Text.AlignHCenter
            }

            Item { Layout.fillHeight: true }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Rectangle {
                    Layout.fillWidth: true
                    height: 38
                    radius: theme.radiusMedium
                    color: cancelClearMouse.containsMouse ? theme.bgTertiary : "transparent"
                    border.color: theme.border
                    border.width: 1
                    Behavior on color { ColorAnimation { duration: theme.animFast } }

                    MouseArea {
                        id: cancelClearMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: clearDataDialog.close()
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
                    height: 38
                    radius: theme.radiusMedium
                    color: confirmClearMouse.pressed ? Qt.darker(theme.danger, 1.2) : confirmClearMouse.containsMouse ? Qt.lighter(theme.danger, 1.1) : theme.danger
                    Behavior on color { ColorAnimation { duration: theme.animFast } }

                    MouseArea {
                        id: confirmClearMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (clearDataDialog.dataType === "events")
                                dashboardStatsModel.clearEvents()
                            else
                                dashboardStatsModel.clearAlerts()
                            clearDataDialog.close()
                        }
                    }

                    Text {
                        anchors.centerIn: parent
                        text: "Очистить"
                        color: "#0d1117"
                        font.pixelSize: theme.fontSizeSM
                        font.bold: true
                        font.family: theme.fontFamily
                    }
                }
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 48

            Text {
                text: "Настройки"
                color: theme.textPrimary
                font.pixelSize: theme.fontSizeXL
                font.bold: true
                font.family: theme.fontFamily
            }

            Item { Layout.fillWidth: true }

            Rectangle {
                visible: !settingsPage.canManage
                width: rolesLabel.implicitWidth + 20
                height: 28
                radius: theme.radiusFull
                color: Qt.rgba(0.345, 0.651, 1.0, 0.12)
                border.color: Qt.rgba(0.345, 0.651, 1.0, 0.3)
                border.width: 1

                Text {
                    id: rolesLabel
                    anchors.centerIn: parent
                    text: "Только просмотр"
                    color: theme.accent
                    font.pixelSize: theme.fontSizeXS
                    font.bold: true
                    font.family: theme.fontFamily
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: theme.border }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            ColumnLayout {
                width: settingsPage.width - 48
                spacing: 16

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 16

                    Rectangle {
                        Layout.preferredWidth: (settingsPage.width - 48 - 16) / 2
                        Layout.preferredHeight: 240
                        Layout.fillWidth: true
                        color: theme.bgSecondary
                        radius: theme.radiusMedium
                        border.color: wsService && wsService.isRunning ? Qt.rgba(0.247, 0.725, 0.314, 0.3) : theme.border
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 16
                            spacing: 10

                            Text {
                                text: "WebSocket сервер"
                                color: theme.textPrimary
                                font.pixelSize: theme.fontSizeSM
                                font.bold: true
                                font.family: theme.fontFamily
                            }

                            Rectangle { Layout.fillWidth: true; height: 1; color: theme.border }

                            RowLayout {
                                Layout.fillWidth: true
                                Text { text: "Статус"; color: theme.textMuted; font.pixelSize: theme.fontSizeSM; font.family: theme.fontFamily }
                                Item { Layout.fillWidth: true }
                                Rectangle {
                                    width: wsStatusLabel.implicitWidth + 32
                                    height: 22
                                    radius: theme.radiusFull
                                    color: wsService && wsService.isRunning ? Qt.rgba(0.247, 0.725, 0.314, 0.15) : Qt.rgba(0.973, 0.318, 0.286, 0.15)
                                    Text {
                                        id: wsStatusLabel
                                        anchors.centerIn: parent
                                        text: wsService && wsService.isRunning ? "Работает" : "Остановлен"
                                        color: wsService && wsService.isRunning ? theme.success : theme.danger
                                        font.pixelSize: theme.fontSizeXS
                                        font.bold: true
                                        font.family: theme.fontFamily
                                    }
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Text { text: "Порт"; color: theme.textMuted; font.pixelSize: theme.fontSizeSM; font.family: theme.fontFamily }
                                Item { Layout.fillWidth: true }
                                Text { text: typeof wsPort !== "undefined" ? wsPort : ""; color: theme.textPrimary; font.pixelSize: theme.fontSizeSM; font.bold: true; font.family: theme.fontFamily }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Text { text: "Адрес"; color: theme.textMuted; font.pixelSize: theme.fontSizeSM; font.family: theme.fontFamily }
                                Item { Layout.fillWidth: true }
                                Text { text: wsPort > 0 ? wsPort.toString() : "8080"; color: theme.accent; font.pixelSize: theme.fontSizeSM; font.family: theme.fontFamily }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Text { text: "Клиентов подключено"; color: theme.textMuted; font.pixelSize: theme.fontSizeSM; font.family: theme.fontFamily }
                                Item { Layout.fillWidth: true }
                                Rectangle {
                                    width: clientCntText.implicitWidth + 16
                                    height: 20
                                    radius: theme.radiusFull
                                    color: theme.bgTertiary
                                    border.color: theme.border
                                    border.width: 1
                                    Text {
                                        id: clientCntText
                                        anchors.centerIn: parent
                                        text: wsService ? wsService.clientCount : "0"
                                        color: theme.textPrimary
                                        font.pixelSize: theme.fontSizeXS
                                        font.bold: true
                                        font.family: theme.fontFamily
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.preferredWidth: (settingsPage.width - 48 - 16) / 2
                        Layout.preferredHeight: 240
                        Layout.fillWidth: true
                        color: theme.bgSecondary
                        radius: theme.radiusMedium
                        border.color: theme.border
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 16
                            spacing: 10

                            Text {
                                text: "Смена пароля"
                                color: theme.textPrimary
                                font.pixelSize: theme.fontSizeSM
                                font.bold: true
                                font.family: theme.fontFamily
                            }

                            Rectangle { Layout.fillWidth: true; height: 1; color: theme.border }

                            RowLayout {
                                Layout.fillWidth: true
                                Text { text: "Хеширование"; color: theme.textMuted; font.pixelSize: theme.fontSizeSM; font.family: theme.fontFamily }
                                Item { Layout.fillWidth: true }
                                Rectangle {
                                    width: pbkdfLabel.implicitWidth + 16
                                    height: 22
                                    radius: theme.radiusFull
                                    color: Qt.rgba(0.247, 0.725, 0.314, 0.15)
                                    border.color: Qt.rgba(0.247, 0.725, 0.314, 0.3)
                                    border.width: 1
                                    Text {
                                        id: pbkdfLabel
                                        anchors.centerIn: parent
                                        text: "PBKDF2 · 100 000 итераций"
                                        color: theme.success
                                        font.pixelSize: theme.fontSizeXS
                                        font.bold: true
                                        font.family: theme.fontFamily
                                    }
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Text { text: "Соль"; color: theme.textMuted; font.pixelSize: theme.fontSizeSM; font.family: theme.fontFamily }
                                Item { Layout.fillWidth: true }
                                Text { text: "256 бит · CSPRNG"; color: theme.textPrimary; font.pixelSize: theme.fontSizeSM; font.family: theme.fontFamily }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Text { text: "Смена пароля при входе"; color: theme.textMuted; font.pixelSize: theme.fontSizeSM; font.family: theme.fontFamily }
                                Item { Layout.fillWidth: true }
                                Rectangle {
                                    width: forcedLabel.implicitWidth + 16
                                    height: 22
                                    radius: theme.radiusFull
                                    color: Qt.rgba(0.247, 0.725, 0.314, 0.15)
                                    border.color: Qt.rgba(0.247, 0.725, 0.314, 0.3)
                                    border.width: 1
                                    Text {
                                        id: forcedLabel
                                        anchors.centerIn: parent
                                        text: "Включено"
                                        color: theme.success
                                        font.pixelSize: theme.fontSizeXS
                                        font.bold: true
                                        font.family: theme.fontFamily
                                    }
                                }
                            }

                            Rectangle { Layout.fillWidth: true; height: 1; color: theme.border }

                            Rectangle {
                                Layout.fillWidth: true
                                height: 36
                                radius: theme.radiusMedium
                                color: changePwMouse.pressed ? theme.accentDark : changePwMouse.containsMouse ? theme.accentHover : theme.accent

                                MouseArea {
                                    id: changePwMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        if (settingsPage.changePasswordDialog) {
                                            settingsPage.changePasswordDialog.isForced = false
                                            settingsPage.changePasswordDialog.open()
                                        }
                                    }
                                }

                                Text {
                                    anchors.centerIn: parent
                                    text: "Сменить пароль"
                                    color: theme.bgPrimary
                                    font.pixelSize: theme.fontSizeSM
                                    font.bold: true
                                    font.family: theme.fontFamily
                                }
                            }
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 16

                    Rectangle {
                        Layout.preferredWidth: (settingsPage.width - 48 - 16) / 2
                        Layout.preferredHeight: 240
                        Layout.fillWidth: true
                        color: theme.bgSecondary
                        radius: theme.radiusMedium
                        border.color: theme.border
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 16
                            spacing: 10

                            Text {
                                text: "База данных"
                                color: theme.textPrimary
                                font.pixelSize: theme.fontSizeSM
                                font.bold: true
                                font.family: theme.fontFamily
                            }

                            Rectangle { Layout.fillWidth: true; height: 1; color: theme.border }

                            RowLayout { Layout.fillWidth: true
                                Text { text: "Тип"; color: theme.textMuted; font.pixelSize: theme.fontSizeSM; font.family: theme.fontFamily }
                                Item { Layout.fillWidth: true }
                                Text { text: "SQLite"; color: theme.textPrimary; font.pixelSize: theme.fontSizeSM; font.family: theme.fontFamily }
                            }
                            RowLayout { Layout.fillWidth: true
                                Text { text: "Файл"; color: theme.textMuted; font.pixelSize: theme.fontSizeSM; font.family: theme.fontFamily }
                                Item { Layout.fillWidth: true }
                                Text { text: "siem_agent.db"; color: theme.textPrimary; font.pixelSize: theme.fontSizeSM; font.family: theme.fontFamily }
                            }
                            RowLayout { Layout.fillWidth: true
                                Text { text: "Расположение"; color: theme.textMuted; font.pixelSize: theme.fontSizeSM; font.family: theme.fontFamily }
                                Item { Layout.fillWidth: true }
                                Text { text: "~/.local/share/SIEMAgent"; color: theme.textPrimary; font.pixelSize: theme.fontSizeSM; font.family: theme.fontFamily }
                            }
                            RowLayout { Layout.fillWidth: true
                                Text { text: "Событий"; color: theme.textMuted; font.pixelSize: theme.fontSizeSM; font.family: theme.fontFamily }
                                Item { Layout.fillWidth: true }
                                Text { text: typeof dashboardStatsModel !== "undefined" && dashboardStatsModel ? dashboardStatsModel.totalEvents : "0"; color: theme.textPrimary; font.pixelSize: theme.fontSizeSM; font.bold: true; font.family: theme.fontFamily }
                            }
                            RowLayout { Layout.fillWidth: true
                                Text { text: "Алертов"; color: theme.textMuted; font.pixelSize: theme.fontSizeSM; font.family: theme.fontFamily }
                                Item { Layout.fillWidth: true }
                                Text { text: typeof dashboardStatsModel !== "undefined" && dashboardStatsModel ? dashboardStatsModel.totalAlerts : "0"; color: theme.textPrimary; font.pixelSize: theme.fontSizeSM; font.bold: true; font.family: theme.fontFamily }
                            }

                            Rectangle { Layout.fillWidth: true; height: 1; color: theme.border; visible: settingsPage.canManage }

                            RowLayout {
                                Layout.fillWidth: true
                                visible: settingsPage.canManage
                                spacing: 8

                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 34
                                    radius: theme.radiusMedium
                                    color: clearEvMouse.pressed ? Qt.darker(theme.danger, 1.2) : clearEvMouse.containsMouse ? Qt.lighter(theme.danger, 1.1) : Qt.rgba(0.973, 0.318, 0.286, 0.15)
                                    border.color: Qt.rgba(0.973, 0.318, 0.286, 0.3)
                                    border.width: 1

                                    MouseArea {
                                        id: clearEvMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: { clearDataDialog.dataType = "events"; clearDataDialog.open() }
                                    }

                                    Text {
                                        anchors.centerIn: parent
                                        text: "Очистить события"
                                        color: theme.danger
                                        font.pixelSize: theme.fontSizeXS
                                        font.bold: true
                                        font.family: theme.fontFamily
                                    }
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 34
                                    radius: theme.radiusMedium
                                    color: clearAlMouse.pressed ? Qt.darker(theme.danger, 1.2) : clearAlMouse.containsMouse ? Qt.lighter(theme.danger, 1.1) : Qt.rgba(0.973, 0.318, 0.286, 0.15)
                                    border.color: Qt.rgba(0.973, 0.318, 0.286, 0.3)
                                    border.width: 1

                                    MouseArea {
                                        id: clearAlMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: { clearDataDialog.dataType = "alerts"; clearDataDialog.open() }
                                    }

                                    Text {
                                        anchors.centerIn: parent
                                        text: "Очистить алерты"
                                        color: theme.danger
                                        font.pixelSize: theme.fontSizeXS
                                        font.bold: true
                                        font.family: theme.fontFamily
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.preferredWidth: (settingsPage.width - 48 - 16) / 2
                        Layout.preferredHeight: 240
                        Layout.fillWidth: true
                        color: theme.bgSecondary
                        radius: theme.radiusMedium
                        border.color: theme.border
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 16
                            spacing: 10

                            Text {
                                text: "О системе"
                                color: theme.textPrimary
                                font.pixelSize: theme.fontSizeSM
                                font.bold: true
                                font.family: theme.fontFamily
                            }

                            Rectangle { Layout.fillWidth: true; height: 1; color: theme.border }

                            RowLayout { Layout.fillWidth: true
                                Text { text: "Название"; color: theme.textMuted; font.pixelSize: theme.fontSizeSM; font.family: theme.fontFamily }
                                Item { Layout.fillWidth: true }
                                Text { text: "SIEM Agent"; color: theme.textPrimary; font.pixelSize: theme.fontSizeSM; font.family: theme.fontFamily }
                            }
                            RowLayout { Layout.fillWidth: true
                                Text { text: "Версия"; color: theme.textMuted; font.pixelSize: theme.fontSizeSM; font.family: theme.fontFamily }
                                Item { Layout.fillWidth: true }
                                Text { text: "1.0.0"; color: theme.textPrimary; font.pixelSize: theme.fontSizeSM; font.family: theme.fontFamily }
                            }
                            RowLayout { Layout.fillWidth: true
                                Text { text: "Платформа"; color: theme.textMuted; font.pixelSize: theme.fontSizeSM; font.family: theme.fontFamily }
                                Item { Layout.fillWidth: true }
                                Text { text: "Qt 6 / QML"; color: theme.textPrimary; font.pixelSize: theme.fontSizeSM; font.family: theme.fontFamily }
                            }
                            RowLayout { Layout.fillWidth: true
                                Text { text: "Назначение"; color: theme.textMuted; font.pixelSize: theme.fontSizeSM; font.family: theme.fontFamily }
                                Item { Layout.fillWidth: true }
                                Text { text: "Мониторинг безопасности"; color: theme.textPrimary; font.pixelSize: theme.fontSizeSM; font.family: theme.fontFamily }
                            }
                            RowLayout { Layout.fillWidth: true
                                Text { text: "Тестирование"; color: theme.textMuted; font.pixelSize: theme.fontSizeSM; font.family: theme.fontFamily }
                                Item { Layout.fillWidth: true }
                                Rectangle {
                                    height: 22
                                    width: simLabel.implicitWidth + 16
                                    radius: theme.radiusFull
                                    color: theme.bgTertiary
                                    border.color: theme.border
                                    border.width: 1
                                    Text {
                                        id: simLabel
                                        anchors.centerIn: parent
                                        text: "python3 simulator.py"
                                        color: theme.accent
                                        font.pixelSize: theme.fontSizeXS
                                        font.family: "monospace"
                                    }
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 320
                    color: theme.bgSecondary
                    radius: theme.radiusMedium
                    border.color: theme.border
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12

                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: "Правила корреляции"
                                color: theme.textPrimary
                                font.pixelSize: theme.fontSizeSM
                                font.bold: true
                                font.family: theme.fontFamily
                            }
                            Item { Layout.fillWidth: true }
                            Rectangle {
                                width: ruleCountBadge.implicitWidth + 16
                                height: 22
                                radius: theme.radiusFull
                                color: theme.bgTertiary
                                border.color: theme.border
                                border.width: 1
                                Text {
                                    id: ruleCountBadge
                                    anchors.centerIn: parent
                                    text: typeof ruleListModel !== "undefined" && ruleListModel ? ruleListModel.rowCount() + " правил" : "0 правил"
                                    color: theme.textSecondary
                                    font.pixelSize: theme.fontSizeXS
                                    font.bold: true
                                    font.family: theme.fontFamily
                                }
                            }
                            Item { width: 8 }
                            Rectangle {
                                visible: settingsPage.canManage
                                width: addRuleLabel.implicitWidth + 24
                                height: 30
                                radius: theme.radiusMedium
                                color: addRuleMouse.pressed ? theme.accentDark : addRuleMouse.containsMouse ? theme.accentHover : theme.accent
                                MouseArea {
                                    id: addRuleMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: addRuleDialog.open()
                                }
                                Text {
                                    id: addRuleLabel
                                    anchors.centerIn: parent
                                    text: "+ Добавить правило"
                                    color: theme.bgPrimary
                                    font.pixelSize: theme.fontSizeXS
                                    font.bold: true
                                    font.family: theme.fontFamily
                                }
                            }
                        }

                        Rectangle { Layout.fillWidth: true; height: 1; color: theme.border }

                        ListView {
                            id: rulesList
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            model: typeof ruleListModel !== "undefined" ? ruleListModel : null
                            spacing: 4
                            boundsBehavior: Flickable.StopAtBounds
                            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded; minimumSize: 0.05 }

                            delegate: Rectangle {
                                width: rulesList.width
                                height: 44
                                radius: theme.radiusSmall
                                color: ruleDelegateMouse.containsMouse ? theme.bgTertiary : "transparent"
                                border.color: ruleDelegateMouse.containsMouse ? theme.border : "transparent"
                                border.width: 1
                                Behavior on color { ColorAnimation { duration: theme.animFast } }

                                MouseArea { id: ruleDelegateMouse; anchors.fill: parent; hoverEnabled: true }

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 12
                                    anchors.rightMargin: 12
                                    spacing: 8

                                    Rectangle {
                                        Layout.preferredWidth: 6
                                        Layout.preferredHeight: 6
                                        radius: 3
                                        color: model.isEnabled ? theme.success : theme.textMuted
                                        Layout.alignment: Qt.AlignVCenter
                                    }

                                    Text {
                                        Layout.preferredWidth: 172
                                        text: model.name !== undefined ? model.name : ""
                                        color: model.isEnabled ? theme.textPrimary : theme.textMuted
                                        font.pixelSize: theme.fontSizeSM
                                        font.family: theme.fontFamily
                                        elide: Text.ElideRight
                                    }

                                    Rectangle {
                                        Layout.preferredWidth: 88
                                        Layout.preferredHeight: 22
                                        radius: theme.radiusFull
                                        color: model.ruleType === "threshold" ? Qt.rgba(0.345, 0.651, 1.0, 0.12) : Qt.rgba(0.973, 0.647, 0.0, 0.12)
                                        Text {
                                            anchors.centerIn: parent
                                            text: model.ruleType === "threshold" ? "threshold" : "correlation"
                                            color: model.ruleType === "threshold" ? theme.accent : theme.warning
                                            font.pixelSize: theme.fontSizeXS
                                            font.family: theme.fontFamily
                                        }
                                    }

                                    Text {
                                        Layout.preferredWidth: 128
                                        text: model.matchEventType !== undefined ? model.matchEventType : ""
                                        color: theme.textSecondary
                                        font.pixelSize: theme.fontSizeXS
                                        font.family: "monospace"
                                        elide: Text.ElideRight
                                    }

                                    Text {
                                        Layout.preferredWidth: 58
                                        text: model.threshold !== undefined ? model.threshold : ""
                                        color: theme.textSecondary
                                        font.pixelSize: theme.fontSizeSM
                                        font.family: theme.fontFamily
                                        horizontalAlignment: Text.AlignHCenter
                                    }

                                    Text {
                                        Layout.preferredWidth: 68
                                        text: model.windowSeconds !== undefined ? model.windowSeconds : ""
                                        color: theme.textSecondary
                                        font.pixelSize: theme.fontSizeSM
                                        font.family: theme.fontFamily
                                        horizontalAlignment: Text.AlignHCenter
                                    }

                                    Rectangle {
                                        Layout.preferredWidth: 68
                                        Layout.preferredHeight: 22
                                        radius: theme.radiusFull
                                        color: {
                                            var s = model.alertSeverity !== undefined ? model.alertSeverity : ""
                                            if (s === "critical") return Qt.rgba(0.973, 0.318, 0.286, 0.15)
                                            if (s === "high") return Qt.rgba(0.976, 0.451, 0.086, 0.15)
                                            if (s === "medium") return Qt.rgba(0.984, 0.741, 0.012, 0.15)
                                            return Qt.rgba(0.247, 0.725, 0.314, 0.15)
                                        }

                                        Text {
                                            anchors.centerIn: parent
                                            text: model.alertSeverity !== undefined ? model.alertSeverity : ""
                                            color: {
                                                var s = model.alertSeverity !== undefined ? model.alertSeverity : ""
                                                if (s === "critical") return theme.danger
                                                if (s === "high") return "#f97316"
                                                if (s === "medium") return theme.warning
                                                return theme.success
                                            }
                                            font.pixelSize: theme.fontSizeXS
                                            font.bold: true
                                            font.family: theme.fontFamily
                                        }
                                    }

                                    Item { Layout.fillWidth: true }

                                    Rectangle {
                                        Layout.preferredWidth: 36
                                        Layout.preferredHeight: 20
                                        radius: 10
                                        color: model.isEnabled ? Qt.rgba(0.247, 0.725, 0.314, 0.3) : theme.bgTertiary
                                        border.color: model.isEnabled ? theme.success : theme.border
                                        border.width: 1

                                        MouseArea {
                                            anchors.fill: parent
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: ruleListModel.toggleRule(model.ruleId, !model.isEnabled)
                                        }

                                        Rectangle {
                                            width: 14
                                            height: 14
                                            radius: 7
                                            color: model.isEnabled ? theme.success : theme.textMuted
                                            anchors.verticalCenter: parent.verticalCenter
                                            anchors.left: parent.left
                                            anchors.leftMargin: model.isEnabled ? 19 : 3
                                        }
                                    }

                                    Rectangle {
                                        visible: settingsPage.canManage
                                        Layout.preferredWidth: 28
                                        Layout.preferredHeight: 28
                                        radius: theme.radiusSmall
                                        color: delRuleMouse.containsMouse ? Qt.rgba(0.973, 0.318, 0.286, 0.15) : "transparent"

                                        MouseArea {
                                            id: delRuleMouse
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: {
                                                deleteRuleDialog.ruleId = model.ruleId
                                                deleteRuleDialog.ruleName = model.name
                                                deleteRuleDialog.open()
                                            }
                                        }

                                        Text {
                                            anchors.centerIn: parent
                                            text: "×"
                                            color: delRuleMouse.containsMouse ? theme.danger : theme.textMuted
                                            font.pixelSize: 16
                                            font.family: theme.fontFamily
                                        }
                                    }
                                }
                            }

                            Text {
                                anchors.centerIn: parent
                                visible: rulesList.count === 0
                                text: "Правил нет. Нажмите «+ Добавить правило»"
                                color: theme.textMuted
                                font.pixelSize: theme.fontSizeSM
                                font.family: theme.fontFamily
                            }
                        }
                    }
                }


                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 120
                    color: theme.bgSecondary
                    radius: theme.radiusMedium
                    border.color: theme.border
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 10

                        Text {
                            text: "Экспорт отчетов"
                            color: theme.textPrimary
                            font.pixelSize: theme.fontSizeSM
                            font.bold: true
                            font.family: theme.fontFamily
                        }

                        Rectangle { Layout.fillWidth: true; height: 1; color: theme.border }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Rectangle {
                                Layout.fillWidth: true; height: 34; radius: theme.radiusMedium
                                color: expEvMouse.pressed ? theme.accentDark : expEvMouse.containsMouse ? theme.accentHover : theme.accent
                                Behavior on color { ColorAnimation { duration: theme.animFast } }
                                MouseArea {
                                    id: expEvMouse; anchors.fill: parent; hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: reportService.exportEventsCsv(reportService.defaultExportPath("events"))
                                }
                                Text { anchors.centerIn: parent; text: "События (CSV)"; color: theme.bgPrimary
                                       font.pixelSize: theme.fontSizeXS; font.bold: true; font.family: theme.fontFamily }
                            }

                            Rectangle {
                                Layout.fillWidth: true; height: 34; radius: theme.radiusMedium
                                color: expAlMouse.pressed ? theme.accentDark : expAlMouse.containsMouse ? theme.accentHover : theme.accent
                                Behavior on color { ColorAnimation { duration: theme.animFast } }
                                MouseArea {
                                    id: expAlMouse; anchors.fill: parent; hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: reportService.exportAlertsCsv(reportService.defaultExportPath("alerts"))
                                }
                                Text { anchors.centerIn: parent; text: "Алерты (CSV)"; color: theme.bgPrimary
                                       font.pixelSize: theme.fontSizeXS; font.bold: true; font.family: theme.fontFamily }
                            }

                            Rectangle {
                                Layout.fillWidth: true; height: 34; radius: theme.radiusMedium
                                color: expRpMouse.pressed ? theme.accentDark : expRpMouse.containsMouse ? theme.accentHover : theme.accent
                                Behavior on color { ColorAnimation { duration: theme.animFast } }
                                MouseArea {
                                    id: expRpMouse; anchors.fill: parent; hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: reportService.exportReportJson(reportService.defaultExportPath("report"))
                                }
                                Text { anchors.centerIn: parent; text: "Отчет (JSON)"; color: theme.bgPrimary
                                       font.pixelSize: theme.fontSizeXS; font.bold: true; font.family: theme.fontFamily }
                            }
                        }
                    }
                }

                Item { height: 8 }
            }
        }
    }

    function saveRule() {
        var name      = ruleNameField.text.trim()
        var ruleType  = ruleTypeCombo.currentText
        // Было: var matchEvt = matchEventField.text.trim() — всегда брало threshold-поле
        // Стало: берём нужное поле в зависимости от типа
        var matchEvt  = ruleType === "threshold"
                    ? matchEventField.text.trim()
                    : matchEventFieldCor.text.trim()
        var secEvt    = secondaryEventField.text.trim()
        var threshold = parseInt(thresholdField.text) || 1
        var window    = parseInt(windowField.text) || 60
        var cooldown  = parseInt(cooldownField.text) || 60
        var severity  = severityCombo.currentText
        var title     = alertTitleField.text.trim()
        var desc      = alertDescField.text.trim()

        if (!name.length)     { ruleErrorText.text = "Введите название"; return }
        if (!matchEvt.length) { ruleErrorText.text = "Введите основное событие"; return }
        if (ruleType === "correlation" && !secEvt.length) {
            ruleErrorText.text = "Введите вторичное событие"
            return
        }
        if (!title.length)    { ruleErrorText.text = "Введите заголовок алерта"; return }

        var ok = ruleListModel.addRule(name, ruleType, matchEvt, secEvt,
                                    threshold, window, cooldown,
                                    severity, title, desc)
        if (ok) {
            clearRuleForm()
            addRuleDialog.close()
        } else {
            ruleErrorText.text = "Ошибка сохранения"
        }
    }

    function clearRuleForm() {
        ruleNameField.text = ""
        matchEventField.text = ""
        secondaryEventField.text = ""
        thresholdField.text = "1"
        windowField.text = "60"
        cooldownField.text = "60"
        alertTitleField.text = ""
        alertDescField.text = ""
        ruleTypeCombo.currentIndex = 0
        severityCombo.currentIndex = 2
        ruleErrorText.text = ""
    }

    // ── Сигналы от ReportService
    Connections {
        target: reportService
        function onExportSuccess(filePath) {
            exportToast.message = "Экспортировано: " + filePath
            exportToast.visible = true
            exportToastTimer.restart()
        }
        function onExportFailed(reason) {
            exportToast.message = "Ошибка: " + reason
            exportToast.visible = true
            exportToastTimer.restart()
        }
    }

    Rectangle {
        id: exportToast
        property string message: ""
        visible: false
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin: 20
        width: toastText.implicitWidth + 32
        height: 38
        radius: theme.radiusFull
        color: theme.bgSecondary
        border.color: theme.border
        border.width: 1
        z: 999

        Text {
            id: toastText
            anchors.centerIn: parent
            text: exportToast.message
            color: theme.textPrimary
            font.pixelSize: theme.fontSizeSM
            font.family: theme.fontFamily
        }

        Timer {
            id: exportToastTimer
            interval: 3000
            onTriggered: exportToast.visible = false
        }
    }
}