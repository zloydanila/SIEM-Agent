import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../styles"
import Utils 1.0

Rectangle {
    id: dashboardPage
    anchors.fill: parent
    color: theme.bgPrimary

    property string currentUserName: ""
    property string currentUserRole: ""
    property var editUserDialog:      null
    property var confirmDeleteDialog: null
    property var authManager:         null

    property bool canManage: currentUserRole === "admin" || currentUserRole === "operator"

    signal logoutRequested()
    signal createUserRequested()

    Theme { id: theme }

    property bool wsConnected: false

    Connections {
        target: wsService
        function onIsRunningChanged()   { dashboardPage.wsConnected = wsService.isRunning }
        function onClientCountChanged() { dashboardPage.wsConnected = wsService.isRunning }
    }

    Component.onCompleted: {
        if (wsService) dashboardPage.wsConnected = wsService.isRunning
        refreshUsers()
        appearAnimation.start()
    }

    function refreshUsers() { userListModel.refresh() }

    opacity: 0
    ParallelAnimation {
        id: appearAnimation
        NumberAnimation {
            target: dashboardPage; property: "opacity"
            from: 0; to: 1; duration: theme.animSlow
            easing.type: Easing.OutCubic
        }
    }

    function maxVal(arr) {
        var m = 1
        for (var i = 0; i < arr.length; i++) if (arr[i] > m) m = arr[i]
        return m
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ── Header ──────────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 64
            color: theme.bgSecondary

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width; height: 1
                color: theme.border
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 24
                anchors.rightMargin: 24
                spacing: 16

                Rectangle {
                    width: 36; height: 36
                    radius: theme.radiusSmall
                    color: theme.bgTertiary
                    border.color: theme.border; border.width: 1
                    Text {
                        anchors.centerIn: parent
                        text: "S"; color: theme.accent
                        font.pixelSize: theme.fontSizeLG
                        font.bold: true; font.family: theme.fontFamily
                    }
                }

                Column {
                    spacing: 2
                    Text {
                        text: "SIEM Dashboard"
                        color: theme.textPrimary
                        font.pixelSize: theme.fontSizeLG
                        font.bold: true; font.family: theme.fontFamily
                    }
                    Text {
                        text: currentUserName + "  |  " + currentUserRole.toUpperCase()
                        color: theme.textMuted
                        font.pixelSize: theme.fontSizeXS
                        font.family: theme.fontFamily; font.letterSpacing: 0.5
                    }
                }

                Row {
                    spacing: 6
                    Rectangle {
                        width: 8; height: 8; radius: 4
                        anchors.verticalCenter: parent.verticalCenter
                        color: wsConnected ? theme.success : theme.danger
                        SequentialAnimation on opacity {
                            running: !wsConnected
                            loops: Animation.Infinite
                            NumberAnimation { to: 0.3; duration: 600 }
                            NumberAnimation { to: 1.0; duration: 600 }
                        }
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: wsConnected ? "Online" : "Offline"
                        color: wsConnected ? theme.success : theme.danger
                        font.pixelSize: theme.fontSizeXS
                        font.family: theme.fontFamily; font.bold: true
                    }
                }

                Item { Layout.fillWidth: true }

                Rectangle {
                    visible: !canManage
                    width: roleLabel.implicitWidth + 20
                    height: 28
                    radius: theme.radiusFull
                    color: Qt.rgba(0.345, 0.651, 1.0, 0.12)
                    border.color: Qt.rgba(0.345, 0.651, 1.0, 0.3)
                    border.width: 1
                    Text {
                        id: roleLabel
                        anchors.centerIn: parent
                        text: "Только просмотр"
                        color: theme.roleViewer
                        font.pixelSize: theme.fontSizeXS
                        font.bold: true; font.family: theme.fontFamily
                    }
                }

                Item { width: 8; visible: !canManage }

                RowLayout {
                    spacing: 8

                    Rectangle {
                        width: 110; height: 34
                        radius: theme.radiusMedium
                        color: addMouseArea.pressed       ? theme.successDark
                             : addMouseArea.containsMouse ? Qt.lighter(theme.success, 1.1)
                             : theme.success
                        visible: currentUserRole === "admin"
                        Behavior on color { ColorAnimation { duration: theme.animFast } }
                        MouseArea {
                            id: addMouseArea; anchors.fill: parent
                            hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                            onClicked: createUserRequested()
                        }
                        Text {
                            anchors.centerIn: parent; text: "+ Add User"
                            color: "#0d1117"
                            font.pixelSize: theme.fontSizeSM
                            font.bold: true; font.family: theme.fontFamily
                        }
                    }

                    Rectangle {
                        width: 80; height: 34
                        radius: theme.radiusMedium
                        color: refMouseArea.containsMouse ? theme.bgTertiary : theme.bgSecondary
                        border.color: theme.border; border.width: 1
                        Behavior on color { ColorAnimation { duration: theme.animFast } }
                        MouseArea {
                            id: refMouseArea; anchors.fill: parent
                            hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                            onClicked: refreshUsers()
                        }
                        Text {
                            anchors.centerIn: parent; text: "Refresh"
                            color: theme.textSecondary
                            font.pixelSize: theme.fontSizeSM; font.family: theme.fontFamily
                        }
                    }

                    Rectangle {
                        width: 80; height: 34
                        radius: theme.radiusMedium
                        color: logMouseArea.containsMouse
                               ? Qt.rgba(0.973, 0.318, 0.286, 0.15) : "transparent"
                        border.color: logMouseArea.containsMouse ? theme.danger : theme.border
                        border.width: 1
                        Behavior on color        { ColorAnimation { duration: theme.animFast } }
                        Behavior on border.color { ColorAnimation { duration: theme.animFast } }
                        MouseArea {
                            id: logMouseArea; anchors.fill: parent
                            hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                            onClicked: logoutRequested()
                        }
                        Text {
                            anchors.centerIn: parent; text: "Logout"
                            color: logMouseArea.containsMouse ? theme.danger : theme.textSecondary
                            font.pixelSize: theme.fontSizeSM; font.family: theme.fontFamily
                            Behavior on color { ColorAnimation { duration: theme.animFast } }
                        }
                    }
                }
            }
        }

        // ── Scroll area ─────────────────────────────────────────────────────
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            ColumnLayout {
                width: parent.width
                spacing: 0

                // ── KPI cards ────────────────────────────────────────────────
                RowLayout {
                    Layout.fillWidth: true
                    Layout.margins: 20
                    spacing: 12

                    Repeater {
                        model: [
                            { label: "Всего событий",    val: dashboardStatsModel ? dashboardStatsModel.totalEvents   : 0, color: theme.accent,  icon: "≡"  },
                            { label: "Открытых алертов", val: dashboardStatsModel ? dashboardStatsModel.openAlerts    : 0, color: theme.danger,  icon: "!"  },
                            { label: "Критических",      val: dashboardStatsModel ? dashboardStatsModel.criticalCount : 0, color: theme.danger,  icon: "!!" },
                            { label: "Высоких",          val: dashboardStatsModel ? dashboardStatsModel.highCount     : 0, color: theme.warning, icon: "^"  },
                            { label: "Средних",          val: dashboardStatsModel ? dashboardStatsModel.mediumCount   : 0, color: theme.accent,  icon: "~"  },
                            { label: "Низких",           val: dashboardStatsModel ? dashboardStatsModel.lowCount      : 0, color: theme.success, icon: "v"  },
                        ]

                        Rectangle {
                            Layout.fillWidth: true
                            height: 90
                            radius: theme.radiusMedium
                            color: theme.bgSecondary
                            border.color: theme.border; border.width: 1
                            
                            Rectangle {
                                width: parent.width; height: 3; radius: 2
                                color: modelData.color
                                anchors.top: parent.top
                            }

                            Column {
                                anchors {
                                    left: parent.left; right: parent.right
                                    top: parent.top; margins: 16
                                }
                                spacing: 6
                                Text {
                                    text: modelData.label
                                    color: theme.textMuted
                                    font.pixelSize: theme.fontSizeXS
                                    font.family: theme.fontFamily; font.letterSpacing: 0.5
                                }
                                Text {
                                    text: modelData.val
                                    color: modelData.color
                                    font.pixelSize: 28
                                    font.bold: true; font.family: theme.fontFamily
                                }
                            }
                        }
                    }
                }

                // ── Charts row ───────────────────────────────────────────────
                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 20; Layout.rightMargin: 20; Layout.bottomMargin: 12
                    spacing: 12

                    // Activity bar chart
                    Rectangle {
                        Layout.fillWidth: true
                        height: 220
                        radius: theme.radiusMedium
                        color: theme.bgSecondary
                        border.color: theme.border; border.width: 1

                        Column {
                            anchors.fill: parent
                            anchors.margins: 16
                            spacing: 12

                            Text {
                                text: "Активность событий (последние 7 часов)"
                                color: theme.textPrimary
                                font.pixelSize: theme.fontSizeSM
                                font.bold: true; font.family: theme.fontFamily
                            }

                            Item {
                                width: parent.width
                                height: parent.height - 50

                                Row {
                                    anchors.fill: parent
                                    spacing: 6

                                    Repeater {
                                        model: dashboardStatsModel ? dashboardStatsModel.activityData.length : 0

                                        Column {
                                            width: (parent.width - 6 * (dashboardStatsModel.activityData.length - 1))
                                                   / dashboardStatsModel.activityData.length
                                            height: parent.height
                                            spacing: 4

                                            Item {
                                                width: parent.width
                                                height: parent.height - 20

                                                Rectangle {
                                                    width: parent.width
                                                    anchors.bottom: parent.bottom
                                                    radius: 3
                                                    color: theme.accent

                                                    property real ratio: {
                                                        var data = dashboardStatsModel.activityData
                                                        var max = 1
                                                        for (var i = 0; i < data.length; i++)
                                                            if (data[i] > max) max = data[i]
                                                        return max > 0 ? data[index] / max : 0
                                                    }
                                                    height: parent.height * ratio

                                                    Rectangle {
                                                        anchors.bottom: parent.top
                                                        anchors.bottomMargin: 4
                                                        anchors.horizontalCenter: parent.horizontalCenter
                                                        width: tipText.implicitWidth + 10
                                                        height: 20; radius: 4
                                                        color: theme.bgTertiary
                                                        border.color: theme.border; border.width: 1
                                                        visible: barMouse.containsMouse
                                                        Text {
                                                            id: tipText
                                                            anchors.centerIn: parent
                                                            text: dashboardStatsModel.activityData[index]
                                                            color: theme.textPrimary
                                                            font.pixelSize: theme.fontSizeXS
                                                            font.family: theme.fontFamily
                                                        }
                                                    }

                                                    MouseArea {
                                                        id: barMouse
                                                        anchors.fill: parent
                                                        hoverEnabled: true
                                                    }

                                                    Behavior on opacity { NumberAnimation { duration: 150 } }
                                                    opacity: barMouse.containsMouse ? 0.7 : 1.0
                                                }
                                            }

                                            Text {
                                                width: parent.width
                                                horizontalAlignment: Text.AlignHCenter
                                                text: dashboardStatsModel.activityLabels[index] ?? ""
                                                color: theme.textMuted
                                                font.pixelSize: 9; font.family: theme.fontFamily
                                                elide: Text.ElideRight
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Severity donut chart
                    Rectangle {
                        width: 260; height: 220
                        radius: theme.radiusMedium
                        color: theme.bgSecondary
                        border.color: theme.border; border.width: 1

                        Column {
                            anchors.fill: parent
                            anchors.margins: 16
                            spacing: 10

                            Text {
                                text: "По уровню угрозы"
                                color: theme.textPrimary
                                font.pixelSize: theme.fontSizeSM
                                font.bold: true; font.family: theme.fontFamily
                            }

                            Row {
                                width: parent.width
                                spacing: 12

                                Canvas {
                                    id: donutCanvas
                                    width: 110; height: 110

                                    Connections {
                                        target: dashboardStatsModel
                                        function onStatsChanged() { donutCanvas.requestPaint() }
                                    }

                                    onPaint: {
                                        var ctx = getContext("2d")
                                        ctx.clearRect(0, 0, width, height)
                                        var cx = width / 2, cy = height / 2
                                        var r = 46, ir = 28
                                        var total = dashboardStatsModel ? (
                                            dashboardStatsModel.criticalCount +
                                            dashboardStatsModel.highCount +
                                            dashboardStatsModel.mediumCount +
                                            dashboardStatsModel.lowCount) : 0
                                        
                                        if (total === 0) {
                                            ctx.beginPath()
                                            ctx.arc(cx, cy, r,  0, Math.PI * 2)
                                            ctx.arc(cx, cy, ir, 0, Math.PI * 2, true)
                                            ctx.fillStyle = "#2a2a2a"
                                            ctx.fill()
                                            return
                                        }

                                        var angle = -Math.PI / 2
                                        var vals   = [dashboardStatsModel.criticalCount,
                                                      dashboardStatsModel.highCount,
                                                      dashboardStatsModel.mediumCount,
                                                      dashboardStatsModel.lowCount]
                                        var colors = [theme.danger, theme.warning,
                                                      theme.accent, theme.success]

                                        for (var i = 0; i < vals.length; i++) {
                                            if (vals[i] === 0) continue
                                            var sweep = (vals[i] / total) * Math.PI * 2
                                            ctx.beginPath()
                                            ctx.moveTo(cx, cy)
                                            ctx.arc(cx, cy, r,  angle, angle + sweep)
                                            ctx.arc(cx, cy, ir, angle + sweep, angle, true)
                                            ctx.closePath()
                                            ctx.fillStyle = colors[i]
                                            ctx.fill()
                                            angle += sweep
                                        }

                                        ctx.fillStyle = theme.bgSecondary
                                        ctx.beginPath()
                                        ctx.arc(cx, cy, ir - 2, 0, Math.PI * 2)
                                        ctx.fill()

                                        ctx.fillStyle = theme.textPrimary
                                        ctx.font = "bold 14px sans-serif"
                                        ctx.textAlign = "center"
                                        ctx.textBaseline = "middle"
                                        ctx.fillText(total, cx, cy)
                                    }
                                }

                                Column {
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: 8

                                    Repeater {
                                        model: [
                                            { label: "Критично", value: dashboardStatsModel ? dashboardStatsModel.criticalCount : 0, color: theme.danger  },
                                            { label: "Высокий",  value: dashboardStatsModel ? dashboardStatsModel.highCount     : 0, color: theme.warning },
                                            { label: "Средний",  value: dashboardStatsModel ? dashboardStatsModel.mediumCount   : 0, color: theme.accent  },
                                            { label: "Низкий",   value: dashboardStatsModel ? dashboardStatsModel.lowCount      : 0, color: theme.success },
                                        ]

                                        Row {
                                            spacing: 6
                                            Rectangle {
                                                width: 8; height: 8; radius: 2
                                                color: modelData.color
                                                anchors.verticalCenter: parent.verticalCenter
                                            }
                                            Text {
                                                text: modelData.label + ": " + modelData.value
                                                color: theme.textSecondary
                                                font.pixelSize: theme.fontSizeXS
                                                font.family: theme.fontFamily
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // ── Top devices ──────────────────────────────────────────────
                Rectangle {
                    Layout.fillWidth: true
                    Layout.leftMargin: 20; Layout.rightMargin: 20; Layout.bottomMargin: 12
                    Layout.preferredHeight: topDevicesCol.implicitHeight + 32
                    Layout.fillHeight: false
                    radius: theme.radiusMedium
                    color: theme.bgSecondary
                    border.color: theme.border; border.width: 1

                    Column {
                        id: topDevicesCol
                        anchors { left: parent.left; right: parent.right; top: parent.top; margins: 16 }
                        spacing: 10

                        Text {
                            text: "Топ устройств по событиям"
                            color: theme.textPrimary
                            font.pixelSize: theme.fontSizeSM
                            font.bold: true; font.family: theme.fontFamily
                        }

                        Text {
                            visible: !dashboardStatsModel || dashboardStatsModel.topDevices.length === 0
                            text: "Данные появятся после получения событий"
                            color: theme.textMuted
                            font.pixelSize: theme.fontSizeXS; font.family: theme.fontFamily
                        }

                        Repeater {
                            model: dashboardStatsModel
                                   ? Math.min(dashboardStatsModel.topDevices.length, 5) : 0

                            Row {
                                width: topDevicesCol.width
                                spacing: 10
                                
                                Text {
                                    width: 180
                                    text: dashboardStatsModel.topDevices[index]?.name ?? "—"
                                    color: theme.textSecondary
                                    font.pixelSize: theme.fontSizeXS; font.family: theme.fontFamily
                                    elide: Text.ElideRight
                                    anchors.verticalCenter: parent.verticalCenter
                                }

                                Rectangle {
                                    width: topDevicesCol.width - 260
                                    height: 8; radius: 4
                                    color: theme.bgTertiary
                                    anchors.verticalCenter: parent.verticalCenter

                                    Rectangle {
                                        height: parent.height; radius: 4
                                        color: theme.accent
                                        property real maxCount: dashboardStatsModel.topDevices[0]?.count ?? 1
                                        width: 0
                                        Component.onCompleted: {
                                            barW.to = (dashboardStatsModel.topDevices[index]?.count ?? 0)
                                                      / maxCount * parent.width
                                            barW.start()
                                        }
                                        NumberAnimation {
                                            id: barW; property: "width"
                                            duration: 700; easing.type: Easing.OutCubic
                                        }
                                    }
                                }

                                Text {
                                    width: 50
                                    text: dashboardStatsModel.topDevices[index]?.count ?? 0
                                    color: theme.textMuted
                                    font.pixelSize: theme.fontSizeXS; font.family: theme.fontFamily
                                    horizontalAlignment: Text.AlignRight
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }
                        }
                    }
                }

                // ── Users table ──────────────────────────────────────────────
                Rectangle {
                    Layout.fillWidth: true
                    Layout.leftMargin: 20; Layout.rightMargin: 20; Layout.bottomMargin: 20
                    height: 52 + 36 + Math.min(usersView.count, 6) * 56 + 2
                    radius: theme.radiusLarge
                    color: theme.bgSecondary
                    border.color: theme.border; border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0

                        Rectangle {
                            Layout.fillWidth: true
                            height: 52; color: "transparent"
                            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: theme.border }
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 20; anchors.rightMargin: 20
                                Text {
                                    text: "Пользователи"
                                    color: theme.textPrimary
                                    font.pixelSize: theme.fontSizeMD
                                    font.bold: true; font.family: theme.fontFamily
                                }
                                Rectangle {
                                    width: badge.implicitWidth + 16; height: 22; radius: 11
                                    color: theme.bgTertiary
                                    border.color: theme.border; border.width: 1
                                    Text {
                                        id: badge; anchors.centerIn: parent
                                        text: usersView.count
                                        color: theme.textSecondary
                                        font.pixelSize: theme.fontSizeXS
                                        font.family: theme.fontFamily; font.bold: true
                                    }
                                }
                                Item { Layout.fillWidth: true }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true; height: 36
                            color: theme.bgTertiary
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 16; anchors.rightMargin: 16
                                spacing: 0
                                Repeater {
                                    model: [
                                        { text: "USERNAME",  w: 160, fill: false },
                                        { text: "FULL NAME", w: 0,   fill: true  },
                                        { text: "EMAIL",     w: 200, fill: false },
                                        { text: "ROLE",      w: 110, fill: false },
                                        { text: "STATUS",    w: 90,  fill: false },
                                        { text: "ACTIONS",   w: 130, fill: false }
                                    ]
                                    delegate: Text {
                                        text: modelData.text
                                        color: theme.textMuted
                                        font.pixelSize: theme.fontSizeXS
                                        font.family: theme.fontFamily
                                        font.letterSpacing: 1; font.bold: true
                                        Layout.preferredWidth: modelData.fill ? -1 : modelData.w
                                        Layout.fillWidth: modelData.fill
                                    }
                                }
                            }
                        }

                        ListView {
                            id: usersView
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true; model: userListModel; spacing: 0
                            boundsBehavior: Flickable.StopAtBounds
                            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                            delegate: Rectangle {
                                width: usersView.width; height: 56
                                color: rowMouse.containsMouse ? theme.bgTertiary : "transparent"
                                Behavior on color { ColorAnimation { duration: theme.animFast } }

                                Rectangle {
                                    anchors.bottom: parent.bottom
                                    anchors.left: parent.left; anchors.right: parent.right
                                    anchors.leftMargin: 16; anchors.rightMargin: 16
                                    height: 1; color: theme.border; opacity: 0.5
                                }
                                
                                MouseArea { id: rowMouse; anchors.fill: parent; hoverEnabled: true }

                                Row {
                                    anchors.fill: parent
                                    anchors.leftMargin: 16; anchors.rightMargin: 16
                                    spacing: 0

                                    Row {
                                        width: 160; height: parent.height; spacing: 10
                                        Rectangle {
                                            width: 28; height: 28; radius: theme.radiusSmall
                                            anchors.verticalCenter: parent.verticalCenter
                                            color: role === "admin"    ? Qt.rgba(0.973,0.318,0.286,0.15)
                                                 : role === "operator" ? Qt.rgba(0.824,0.600,0.133,0.15)
                                                 : Qt.rgba(0.345,0.651,1.0,0.15)
                                            Text {
                                                anchors.centerIn: parent
                                                text: username.charAt(0).toUpperCase()
                                                color: role === "admin"    ? theme.roleAdmin
                                                     : role === "operator" ? theme.roleOperator
                                                     : theme.roleViewer
                                                font.pixelSize: theme.fontSizeSM
                                                font.bold: true; font.family: theme.fontFamily
                                            }
                                        }
                                        Text {
                                            text: username; color: theme.textPrimary
                                            font.pixelSize: theme.fontSizeSM
                                            font.family: theme.fontFamily
                                            elide: Text.ElideRight
                                            width: parent.width - 38
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                    }

                                    Text {
                                        text: fullName || "—"
                                        color: theme.textSecondary
                                        font.pixelSize: theme.fontSizeSM
                                        font.family: theme.fontFamily
                                        elide: Text.ElideRight
                                        width: parent.width - (160 + 200 + 110 + 90 + 130 + 32)
                                        height: parent.height
                                        verticalAlignment: Text.AlignVCenter
                                    }

                                    Text {
                                        width: 200; text: email || "—"
                                        color: theme.textMuted
                                        font.pixelSize: theme.fontSizeXS
                                        font.family: theme.fontFamily
                                        elide: Text.ElideRight
                                        height: parent.height
                                        verticalAlignment: Text.AlignVCenter
                                    }

                                    Rectangle {
                                        width: 110; height: 24
                                        radius: theme.radiusSmall
                                        anchors.verticalCenter: parent.verticalCenter
                                        color: role === "admin"    ? Qt.rgba(0.973,0.318,0.286,0.12)
                                             : role === "operator" ? Qt.rgba(0.824,0.600,0.133,0.12)
                                             : Qt.rgba(0.345,0.651,1.0,0.12)
                                        border.color: role === "admin"    ? Qt.rgba(0.973,0.318,0.286,0.3)
                                                    : role === "operator" ? Qt.rgba(0.824,0.600,0.133,0.3)
                                                    : Qt.rgba(0.345,0.651,1.0,0.3)
                                        border.width: 1
                                        Text {
                                            anchors.centerIn: parent
                                            text: role === "admin" ? "Admin"
                                                : role === "operator" ? "Operator" : "Viewer"
                                            color: role === "admin"    ? theme.roleAdmin
                                                 : role === "operator" ? theme.roleOperator
                                                 : theme.roleViewer
                                            font.pixelSize: theme.fontSizeXS; font.bold: true
                                            font.family: theme.fontFamily; font.letterSpacing: 0.5
                                        }
                                    }
                                    
                                    Rectangle {
                                        width: 90; height: 24
                                        radius: theme.radiusSmall
                                        anchors.verticalCenter: parent.verticalCenter
                                        color: isActive ? Qt.rgba(0.247,0.725,0.314,0.12)
                                                        : Qt.rgba(0.541,0.580,0.608,0.12)
                                        border.color: isActive ? Qt.rgba(0.247,0.725,0.314,0.3)
                                                               : Qt.rgba(0.541,0.580,0.608,0.3)
                                        border.width: 1
                                        Text {
                                            anchors.centerIn: parent
                                            text: isActive ? "Active" : "Inactive"
                                            color: isActive ? theme.success : theme.textMuted
                                            font.pixelSize: theme.fontSizeXS; font.bold: true
                                            font.family: theme.fontFamily
                                        }
                                    }

                                    Row {
                                        width: 130; height: parent.height
                                        spacing: 6
                                        anchors.verticalCenter: parent.verticalCenter

                                        Rectangle {
                                            width: 58; height: 28
                                            radius: theme.radiusSmall
                                            visible: currentUserRole === "admin"
                                            anchors.verticalCenter: parent.verticalCenter
                                            color: editMouse.containsMouse ? theme.bgTertiary : "transparent"
                                            border.color: editMouse.containsMouse ? theme.border : "transparent"
                                            border.width: 1
                                            Behavior on color { ColorAnimation { duration: theme.animFast } }
                                            MouseArea {
                                                id: editMouse; anchors.fill: parent
                                                hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                                                onClicked: {
                                                    if (editUserDialog) {
                                                        editUserDialog.userToEdit = {
                                                            id: model.id,
                                                            username: model.username,
                                                            fullName: model.fullName,
                                                            email: model.email,
                                                            role: model.role,
                                                            isActive: model.isActive
                                                        }
                                                        editUserDialog.open()
                                                    }
                                                }
                                            }
                                            Text { anchors.centerIn: parent; text: "Edit"; color: theme.textSecondary; font.pixelSize: theme.fontSizeXS; font.family: theme.fontFamily }
                                        }

                                        Rectangle {
                                            width: 58; height: 28
                                            radius: theme.radiusSmall
                                            visible: currentUserRole === "admin"
                                            anchors.verticalCenter: parent.verticalCenter
                                            color: delMouse.containsMouse ? Qt.rgba(0.973,0.318,0.286,0.15) : "transparent"
                                            border.color: delMouse.containsMouse ? theme.danger : "transparent"
                                            border.width: 1
                                            Behavior on color { ColorAnimation { duration: theme.animFast } }
                                            MouseArea {
                                                id: delMouse; anchors.fill: parent
                                                hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                                                onClicked: {
                                                    if (confirmDeleteDialog) {
                                                        confirmDeleteDialog.userToDelete = {
                                                            id: model.id,
                                                            username: model.username
                                                        }
                                                        confirmDeleteDialog.open()
                                                    }
                                                }
                                            }
                                            Text {
                                                anchors.centerIn: parent; text: "Delete"
                                                color: delMouse.containsMouse ? theme.danger : theme.textSecondary
                                                font.pixelSize: theme.fontSizeXS; font.family: theme.fontFamily
                                                Behavior on color { ColorAnimation { duration: theme.animFast } }
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
    }
}