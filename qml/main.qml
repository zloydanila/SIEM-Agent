import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import "./styles"
import "./components"

ApplicationWindow {
    id: mainWindow
    visible: true
    width: 1280
    height: 800
    title: "SIEM Agent"
    color: "#0d1117"

    Theme { id: theme }

    property bool isAuthenticated: false
    property var  currentUser: null

    Loader {
        id: loginLoader
        anchors.fill: parent
        source: Qt.resolvedUrl("pages/LoginPage.qml")
        visible: !mainWindow.isAuthenticated
        active: !mainWindow.isAuthenticated

        Connections {
            target: loginLoader.item
            ignoreUnknownSignals: true
            function onLoginAttempt(username, password) {
                authManager.login(username, password)
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0
        visible: mainWindow.isAuthenticated

        Slidebar {
            id: sidebar
            Layout.fillHeight: true

            onPageChanged: function(index) {
                pageLoader.currentIndex = index
            }

            onLogoutRequested: {
                authManager.logout()
            }
        }

        StackLayout {
            id: pageLoader
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: 0

            Loader {
                id: dashLoader
                source: mainWindow.isAuthenticated ? Qt.resolvedUrl("pages/DashboardPage.qml") : ""
                active: mainWindow.isAuthenticated

                onLoaded: {
                    item.currentUserName     = mainWindow.currentUser?.username ?? ""
                    item.currentUserRole     = mainWindow.currentUser?.role ?? ""
                    item.editUserDialog      = mainWindow.editUserDialog
                    item.confirmDeleteDialog = mainWindow.confirmDeleteDialog
                }

                Connections {
                    target: dashLoader.item
                    ignoreUnknownSignals: true
                    function onLogoutRequested() {
                        authManager.logout()
                    }
                    function onCreateUserRequested() {
                        if (mainWindow.registerDialog)
                            mainWindow.registerDialog.open()
                    }
                }
            }

            Loader {
                id: eventsLoader
                source: mainWindow.isAuthenticated ? Qt.resolvedUrl("pages/EventsPage.qml") : ""
                active: mainWindow.isAuthenticated
            }

            Loader {
                id: alertsLoader
                source: mainWindow.isAuthenticated ? Qt.resolvedUrl("pages/AlertsPage.qml") : ""
                active: mainWindow.isAuthenticated
                onLoaded: {
                    item.currentUserRole = mainWindow.currentUser?.role ?? "viewer"
                }
            }

            Loader {
                id: settingsLoader
                source: mainWindow.isAuthenticated ? Qt.resolvedUrl("pages/SettingPage.qml") : ""
                active: mainWindow.isAuthenticated

                onLoaded: {
                    item.changePasswordDialog = mainWindow.changePasswordDialog
                    item.currentUserRole = mainWindow.currentUser?.role ?? "viewer"
                }
            }
        }
    }

    Loader {
        id: registerDialogLoader
        source: Qt.resolvedUrl("pages/RegisterUserDialog.qml")
        active: true
        onLoaded: { item.authManager = authManager }
    }

    Loader {
        id: editUserDialogLoader
        source: Qt.resolvedUrl("pages/EditUserDialog.qml")
        active: true
        onLoaded: { item.authManager = authManager }
    }

    Loader {
        id: confirmDeleteDialogLoader
        source: Qt.resolvedUrl("pages/ConfirmDeleteDialog.qml")
        active: true
        onLoaded: { item.authManager = authManager }
    }

    Loader {
        id: changePasswordDialogLoader
        source: Qt.resolvedUrl("pages/ChangePasswordDialog.qml")
        active: true
        onLoaded: { item.authManager = authManager }
    }

    property var registerDialog: registerDialogLoader.item
    property var editUserDialog: editUserDialogLoader.item
    property var confirmDeleteDialog: confirmDeleteDialogLoader.item
    property var changePasswordDialog: changePasswordDialogLoader.item

    Connections {
        target: authManager

        function onLoginSuccess(username, role, fullName, email) {
            mainWindow.isAuthenticated = true
            mainWindow.currentUser = {
                username: username,
                role: role,
                fullName: fullName,
                email: email
            }
            sidebar.currentUser = mainWindow.currentUser

            Qt.callLater(function() {
                if (dashLoader.item) {
                    dashLoader.item.currentUserName = username
                    dashLoader.item.currentUserRole = role
                    dashLoader.item.editUserDialog = mainWindow.editUserDialog
                    dashLoader.item.confirmDeleteDialog = mainWindow.confirmDeleteDialog
                }
                if (alertsLoader.item) {
                    alertsLoader.item.currentUserRole = role
                }
                if (settingsLoader.item) {
                    settingsLoader.item.currentUserRole = role
                    settingsLoader.item.changePasswordDialog = mainWindow.changePasswordDialog
                }
            })
        }

        function onLoginFailed(reason) {
            if (loginLoader.item && loginLoader.item.showError)
                loginLoader.item.showError(reason)
        }

        function onPasswordChangeRequired() {
            if (mainWindow.changePasswordDialog) {
                mainWindow.changePasswordDialog.isForced = true
                mainWindow.changePasswordDialog.open()
            }
        }

        function onPasswordChanged() {
            if (mainWindow.changePasswordDialog)
                mainWindow.changePasswordDialog.close()
        }

        function onLoggedOut() {
            mainWindow.isAuthenticated = false
            mainWindow.currentUser = null
            sidebar.currentUser = null
            sidebar.currentIndex = 0
            pageLoader.currentIndex = 0
        }

        function onRegistrationFailed(reason) {
            if (registerDialog) registerDialog.showError(reason)
        }

        function onUserRegistered(username) {
            if (registerDialog) {
                registerDialog.clearForm()
                registerDialog.close()
            }
            if (dashLoader.item && dashLoader.item.refreshUsers)
                dashLoader.item.refreshUsers()
        }

        function onUserUpdated(userId) {
            if (editUserDialog) {
                editUserDialog.clearForm()
                editUserDialog.close()
            }
            if (dashLoader.item && dashLoader.item.refreshUsers)
                dashLoader.item.refreshUsers()
        }

        function onUpdateFailed(reason) {
            if (editUserDialog) editUserDialog.showError(reason)
        }

        function onUserDeleted(userId) {
            if (confirmDeleteDialog) {
                confirmDeleteDialog.clearForm()
                confirmDeleteDialog.close()
            }
            if (dashLoader.item && dashLoader.item.refreshUsers)
                dashLoader.item.refreshUsers()
        }

        function onDeletionFailed(reason) {
            if (confirmDeleteDialog) confirmDeleteDialog.showError(reason)
        }

        function onErrorOccured(message) {
            if (mainWindow.changePasswordDialog)
                mainWindow.changePasswordDialog.showError(message)
        }
    }
}