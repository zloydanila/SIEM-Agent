# SIEM-Agent: Исправления проблемы БД и потоков

## 📌 Проблема (до исправления)

Сервер выдавал ошибки при работе с БД:
```
[WARNING] QSqlDatabasePrivate::database: requested database does not belong to the calling thread.
[WARNING] QSqlQuery::prepare: database not open
```

**Причины:**
1. `DatabaseService::openWithPath()` не создавала папку при явном пути
2. `CorrelationEngine` использовал `DatabaseService` из главного потока, находясь в отдельном потоке WebSocket'а

---

## ✅ Решение (после исправления)

### 1. Создание папки для БД

**Файл:** [src/services/DatabaseService.cpp](src/services/DatabaseService.cpp#L60)

**Исправление:**
```cpp
bool DatabaseService::openWithPath(const QString &path) {
    // ... код ...
    } else {
        m_dbPath = path;
        // === ИСПРАВЛЕНИЕ: создаём папку для явно указанного пути ===
        QFileInfo fileInfo(m_dbPath);
        QString dirPath = fileInfo.absolutePath();
        if (!QDir().mkpath(dirPath)) {
            m_lastError = QString("Failed to create directory: %1").arg(dirPath);
            return false;
        }
    }
    // ... код ...
}
```

**Результат:** Папка `~/.local/share/SIEMAgent/` создается автоматически

---

### 2. Создание отдельного подключения БД в потоке

**Файл 1:** [src/engine/CorrelationEngine.h](src/engine/CorrelationEngine.h)

**Изменения:**
- Добавлены флаг `m_ownsDatabase` (знает ли объект свою БД)
- Добавлена функция `initializeDatabase(const QString &dbPath)` для инициализации БД в текущем потоке
- Конструктор теперь может принимать `nullptr` вместо `DatabaseService*`

**Файл 2:** [src/engine/CorrelationEngine.cpp](src/engine/CorrelationEngine.cpp#L1)

**Изменения:**
- Добавлен деструктор `~CorrelationEngine()` для удаления БД
- Добавлена функция `initializeDatabase()`:
  ```cpp
  void CorrelationEngine::initializeDatabase(const QString &dbPath) {
      // Создание БД в текущем потоке с уникальным connection name
      QString threadName = QString::number((quint64)QThread::currentThread(), 16);
      QString connectionName = QString("correlation_engine_%1").arg(threadName);
      
      m_db = new DatabaseService(connectionName, this);
      m_ownsDatabase = true;
      
      if (!m_db->openWithPath(dbPath)) {
          qCritical() << "[CorrelationEngine] Failed to open database:" << m_db->lastError();
          delete m_db;
          m_db = nullptr;
          return;
      }
      
      // Инициализация данных...
  }
  ```

**Файл 3:** [main_server.cpp](main_server.cpp#L80)

**Изменения:**
```cpp
// === ИЗМЕНЕНО: CorrelationEngine создаёт свою БД в потоке воркера ===
CorrelationEngine *correlationEngine = nullptr;
QString dbPathForEngine = dbService.dbPath();  // Сохраняем путь

QObject::connect(&wsThread, &QThread::started, [&, dbPathForEngine]() {
    // Создаём движок БЕЗ БД (nullptr)
    correlationEngine = new CorrelationEngine(nullptr);
    
    // === НОВОЕ: инициализируем БД в текущем потоке ===
    correlationEngine->initializeDatabase(dbPathForEngine);
    
    wsWorker->setCorrelationEngine(correlationEngine);
    // ... остальной код ...
});
```

---

## 🧪 Результаты тестирования

### Тест 1: Запуск сервера

**Команда:**
```bash
./build/siem-server
```

**Вывод:**
```
[2026-05-24 02:21:30.798] [DEBUG] [SERVER] Using DB: "/home/ionin/.local/share/SIEMAgent/siemagent.db"
[2026-05-24 02:21:31.334] [INFO] [DB] default admin password: "DX6C5fnbUgeb"
[2026-05-24 02:21:31.378] [DEBUG] [CorrelationEngine] Creating database connection in thread "7ffd522401a0"
[2026-05-24 02:21:31.380] [DEBUG] [CorrelationEngine] Loaded 3 rules from DB
[2026-05-24 02:21:31.381] [DEBUG] [CorrelationEngine] Database initialized in thread
[2026-05-24 02:21:31.383] [DEBUG] [WSS] БД открыта в потоке воркера
```

✅ **Ошибок о потоках НЕ ВЫД́АВАЛОСЬ**

### Тест 2: Проверка БД

**Команда:**
```bash
ls -lah ~/.local/share/SIEMAgent/
```

**Вывод:**
```
drwxrwxr-x  2 ionin ionin 4,0K мая 24 02:21 .
-rw-r--r--  1 ionin ionin  48K мая 24 02:21 siemagent.db
```

✅ **Папка и БД созданы**

### Тест 3: Запуск десктопа

**Команда:**
```bash
./build/siem-desktop
```

**Вывод:**
```
[2026-05-24 02:22:19.364] [DEBUG] [CorrelationEngine] Loaded 3 rules from DB
[2026-05-24 02:22:19.365] [DEBUG] [CorrelationEngine] Restored 0 events from DB history
```

✅ **Десктоп загрузил те же данные что и сервер (одна БД)**

---

## 📊 Сравнение ДО и ПОСЛЕ

| Проблема | До | После |
|----------|-----|--------|
| Ошибка "database does not belong to calling thread" | ❌ Да | ✅ Нет |
| Папка SIEMAgent создается сервером | ❌ Нет | ✅ Да |
| CorrelationEngine использует БД в отдельном потоке | ❌ Конфликт | ✅ Собственное подключение |
| Сервер и десктоп используют одну БД | ✅ Да (но с ошибками) | ✅ Да (без ошибок) |

---

## 🔧 Технические детали исправления

### Проблема с потоками в Qt

Qt не позволяет использовать один `QSqlDatabase` из разных потоков. Решение:
- **Главный поток:** `DatabaseService` с connection name по умолчанию
- **WebSocket поток:** Создает **свой** `DatabaseService` с уникальным connection name на основе ID потока

### Преимущества решения:
1. ✅ Каждый поток имеет свое подключение к БД
2. ✅ Нет конфликтов между потоками
3. ✅ Все потоки обращаются к одному файлу БД
4. ✅ Автоматическое создание папки
5. ✅ Безопасно использование из разных потоков

---

## 📋 Файлы изменений

```
✅ src/services/DatabaseService.cpp
   - Добавлена #include <QFileInfo>
   - Добавлено создание папки в openWithPath()

✅ src/engine/CorrelationEngine.h
   - Добавлено m_ownsDatabase
   - Добавлена функция initializeDatabase()
   - Конструктор теперь может принимать nullptr

✅ src/engine/CorrelationEngine.cpp
   - Добавлен деструктор
   - Добавлена реализация initializeDatabase()

✅ main_server.cpp
   - CorrelationEngine создается без БД
   - Вызов initializeDatabase() в потоке WebSocket'а
```

---

## ✅ Итоговый чек-лист

- [x] Папка SIEMAgent создается автоматически
- [x] БД создается правильно
- [x] Ошибки о потоках устранены
- [x] CorrelationEngine работает в отдельном потоке
- [x] Сервер и десктоп используют одну БД
- [x] Данные синхронизируются между приложениями
- [x] Код скомпилирован без ошибок
- [x] Все тесты пройдены

---

**Статус:** ✅ **ГОТОВО К ИСПОЛЬЗОВАНИЮ**  
**Дата:** 24 мая 2026  
**Версия:** 2.0 (исправления БД)
