#include <QtTest>
#include "services/DatabaseService.h"
#include "engine/CorrelationEngine.h"
#include "engine/Rule.h"
#include "models/Event.h"

class tst_CorrelationEngine : public QObject {
    Q_OBJECT

private:
    DatabaseService  *m_db     = nullptr;
    CorrelationEngine *m_engine = nullptr;

    Event makeEvent(const QString &device, const QString &type,
                    const QString &severity = "high") {
        Event e;
        e.id         = QUuid::createUuid().toString(QUuid::WithoutBraces);
        e.deviceName = device;
        e.eventType  = type;
        e.action     = "test_action";
        e.severity   = severity;
        e.timestamp  = QDateTime::currentDateTimeUtc();
        e.rawLog     = "test log";
        e.location   = "127.0.0.1";
        return e;
    }

    Rule makeThresholdRule(const QString &eventType, int threshold,
                           int window = 60, int cooldown = 5) {
        Rule r;
        r.id               = QUuid::createUuid().toString(QUuid::WithoutBraces);
        r.name             = "Test threshold rule";
        r.ruleType         = "threshold";
        r.matchEventType   = eventType;
        r.threshold        = threshold;
        r.windowSeconds    = window;
        r.cooldownSeconds  = cooldown;
        r.alertTitle       = "Test Alert";
        r.alertDescription = "Test description";
        r.alertSeverity    = "high";
        r.isEnabled        = true;
        return r;
    }

private slots:
    void initTestCase() {
        m_db = new DatabaseService("test_corr_connection");
        QVERIFY(m_db->openWithPath(":memory:"));
        QVERIFY(m_db->initSchema());
        m_engine = new CorrelationEngine(m_db);
    }

    void init() {
        m_db->clearCorrelationHistory();
        delete m_engine;
        m_engine = new CorrelationEngine(m_db);
    }

    void cleanupTestCase() {
        delete m_engine;
        delete m_db;
    }

    void test_thresholdFires() {
        Rule rule = makeThresholdRule("failed_login", 3);
        QVERIFY(m_db->createRule(rule));
        m_engine->reloadRules();

        QSignalSpy spy(m_engine, &CorrelationEngine::alertCreated);

        // 2 события — тихо
        m_engine->analyze(makeEvent("server01", "failed_login"));
        m_engine->analyze(makeEvent("server01", "failed_login"));
        QCOMPARE(spy.count(), 0);

        m_engine->analyze(makeEvent("server01", "failed_login"));
        QCOMPARE(spy.count(), 1);

        QVERIFY(m_db->deleteRule(rule.id));
        m_engine->reloadRules();
    }

    void test_thresholdPerDevice() {
        Rule rule = makeThresholdRule("unique_per_device_abc", 3);
        QVERIFY(m_db->createRule(rule));
        m_engine->reloadRules();

        QSignalSpy spy(m_engine, &CorrelationEngine::alertCreated);

        m_engine->analyze(makeEvent("server01", "unique_per_device_abc"));
        m_engine->analyze(makeEvent("server02", "unique_per_device_abc"));
        m_engine->analyze(makeEvent("server01", "unique_per_device_abc"));

        QCOMPARE(spy.count(), 0);

        QVERIFY(m_db->deleteRule(rule.id));
        m_engine->reloadRules();
    }

    void test_disabledRuleDoesNotFire() {
        Rule rule = makeThresholdRule("port_scan", 2);
        rule.isEnabled = false;
        QVERIFY(m_db->createRule(rule));
        m_engine->reloadRules();

        QSignalSpy spy(m_engine, &CorrelationEngine::alertCreated);

        m_engine->analyze(makeEvent("server01", "port_scan"));
        m_engine->analyze(makeEvent("server01", "port_scan"));
        m_engine->analyze(makeEvent("server01", "port_scan"));

        QCOMPARE(spy.count(), 0);

        QVERIFY(m_db->deleteRule(rule.id));
        m_engine->reloadRules();
    }

    void test_cooldownPreventsDoubleAlert() {
        Rule rule = makeThresholdRule("unique_test_event_xyz", 2, 60, 3600);
        QVERIFY(m_db->createRule(rule));
        m_engine->reloadRules();

        QSignalSpy spy(m_engine, &CorrelationEngine::alertCreated);

        m_engine->analyze(makeEvent("server01", "unique_test_event_xyz"));
        m_engine->analyze(makeEvent("server01", "unique_test_event_xyz")); 
        m_engine->analyze(makeEvent("server01", "unique_test_event_xyz")); 
        m_engine->analyze(makeEvent("server01", "unique_test_event_xyz")); 

        QCOMPARE(spy.count(), 1);

        QVERIFY(m_db->deleteRule(rule.id));
        m_engine->reloadRules();
    }

    void test_globalCleanupDoesNotCrash() {
        m_engine->analyze(makeEvent("server01", "some_event"));
        m_engine->globalCleanup();
        QVERIFY(true);
    }
};

QTEST_MAIN(tst_CorrelationEngine)
#include "tst_CorrelationEngine.moc"