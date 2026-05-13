#include <QtTest>
#include "services/DatabaseService.h"
#include "managers/AuthManager.h"

class tst_AuthManager : public QObject {
    Q_OBJECT

private:
    DatabaseService *m_db     = nullptr;
    AuthManager     *m_auth   = nullptr;

private slots:
    void initTestCase() {
        m_db = new DatabaseService("test_auth_connection");
        QVERIFY(m_db->openWithPath(":memory:"));
        QVERIFY(m_db->initSchema());
        QVERIFY(m_db->createDefaultAdmin());
        m_auth = new AuthManager(m_db);
    }

    void cleanupTestCase() {
        delete m_auth;
        delete m_db;
    }

    void test_loginSuccess() {
        QSignalSpy spy(m_auth, &AuthManager::loginSuccess);

        User admin;
        admin.id       = "test-admin-id";
        admin.username = "testadmin";
        admin.role     = "admin";
        admin.fullName = "Test Admin";
        admin.email    = "admin@test.com";
        admin.isActive = true;
        admin.mustChangePassword = false;
        admin.createdAt = QDateTime::currentDateTime();
        admin.setPassword("secret123", m_db->generateSalt());
        QVERIFY(m_db->createUser(admin));

        m_auth->login("testadmin", "secret123");

        QCOMPARE(spy.count(), 1);
        QVERIFY(m_auth->isAuthenticated());
        QCOMPARE(m_auth->getCurrentUser()->username, QString("testadmin"));

        m_auth->logout();
    }

    void test_loginWrongPassword() {
        QSignalSpy spy(m_auth, &AuthManager::loginFailed);

        m_auth->login("testadmin", "wrongpassword");

        QCOMPARE(spy.count(), 1);
        QVERIFY(!m_auth->isAuthenticated());
    }

    void test_loginUnknownUser() {
        QSignalSpy spy(m_auth, &AuthManager::loginFailed);

        m_auth->login("ghost", "anypassword");

        QCOMPARE(spy.count(), 1);
    }

    void test_loginInactiveUser() {
        User inactive;
        inactive.id       = "inactive-id";
        inactive.username = "inactive";
        inactive.role     = "viewer";
        inactive.fullName = "Inactive User";
        inactive.email    = "inactive@test.com";
        inactive.isActive = false;
        inactive.mustChangePassword = false;
        inactive.createdAt = QDateTime::currentDateTime();
        inactive.setPassword("pass123", m_db->generateSalt());
        QVERIFY(m_db->createUser(inactive));

        QSignalSpy spy(m_auth, &AuthManager::loginFailed);
        m_auth->login("inactive", "pass123");

        QCOMPARE(spy.count(), 1);
    }

    void test_logout() {
        m_auth->login("testadmin", "secret123");
        QVERIFY(m_auth->isAuthenticated());

        QSignalSpy spy(m_auth, &AuthManager::loggedOut);
        m_auth->logout();

        QCOMPARE(spy.count(), 1);
        QVERIFY(!m_auth->isAuthenticated());
        QVERIFY(m_auth->getCurrentUser() == nullptr);
    }

    void test_changePassword() {
        m_auth->login("testadmin", "secret123");
        QVERIFY(m_auth->isAuthenticated());

        QSignalSpy spy(m_auth, &AuthManager::passwordChanged);
        bool result = m_auth->changePassword("secret123", "newpass456");

        QVERIFY(result);
        QCOMPARE(spy.count(), 1);

        m_auth->logout();

        QSignalSpy failSpy(m_auth, &AuthManager::loginFailed);
        m_auth->login("testadmin", "secret123");
        QCOMPARE(failSpy.count(), 1);

        QSignalSpy okSpy(m_auth, &AuthManager::loginSuccess);
        m_auth->login("testadmin", "newpass456");
        QCOMPARE(okSpy.count(), 1);

        m_auth->logout();
    }


    void test_registerWithoutAdmin() {

        User viewer;
        viewer.id       = "viewer-id";
        viewer.username = "viewer";
        viewer.role     = "viewer";
        viewer.fullName = "Viewer";
        viewer.email    = "viewer@test.com";
        viewer.isActive = true;
        viewer.mustChangePassword = false;
        viewer.createdAt = QDateTime::currentDateTime();
        viewer.setPassword("viewpass", m_db->generateSalt());
        QVERIFY(m_db->createUser(viewer));

        m_auth->login("viewer", "viewpass");

        QSignalSpy spy(m_auth, &AuthManager::registrationFailed);
        m_auth->registerUser("newuser", "pass123", "viewer", "New", "new@test.com");

        QCOMPARE(spy.count(), 1);
        m_auth->logout();
    }

    void test_cannotDeleteSelf() {
        m_auth->login("testadmin", "newpass456");

        QSignalSpy spy(m_auth, &AuthManager::deletionFailed);
        m_auth->deleteUser(m_auth->getCurrentUser()->id);

        QCOMPARE(spy.count(), 1);
        m_auth->logout();
    }
};

QTEST_MAIN(tst_AuthManager)
#include "tst_AuthManager.moc"