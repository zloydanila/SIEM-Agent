# SIEM Agent

Система мониторинга безопасности (SIEM) с графическим интерфейсом,
разработанная на C++/Qt6 в рамках РГР по ТИМП.

## Возможности

- 📡 Приём событий по WebSocket (WS/WSS)
- 🔐 HMAC-аутентификация агентов
- 🚨 Движок корреляции событий с настраиваемыми правилами
- 📊 Дашборд с real-time статистикой
- 👤 Управление пользователями с ролями (admin/operator/viewer)
- 🛡️ Rate limiting и защита от replay-атак

## Требования

| Компонент | Версия |
|---|---|
| Qt | 6.4+ |
| CMake | 3.16+ |
| GCC | 11+ |
| OpenSSL | 3.0+ |
| Python | 3.9+ (для симулятора) |
| Docker | 20.10+ (опционально) |

## Быстрый старт

### Автоматическая установка
```bash
chmod +x install.sh
./install.sh
```

### Ручная сборка

**1. Установка зависимостей (Ubuntu/Debian):**
```bash
sudo apt-get install -y cmake ninja-build \
    qt6-base-dev qt6-websockets-dev \
    qt6-declarative-dev libqt6sql6-sqlite openssl
```

**2. Генерация SSL сертификатов:**
```bash
mkdir -p certs
openssl req -x509 -newkey rsa:4096 -sha256 -days 3650 \
    -nodes \
    -keyout certs/server.key \
    -out certs/server.crt \
    -subj "/CN=siem-agent/O=NSTU/C=RU"
```

**3. Сборка:**
```bash
mkdir build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -j$(nproc)
cd ..
```

**4. Запуск:**
```bash
./build/SIEMAgent
```

При первом запуске в консоли отобразится пароль администратора.

## Симулятор событий

### Напрямую:
```bash
pip3 install websockets
python3 scripts/simulator.py
```

### Через Docker:
```bash
# Агент запущен на хосте, симулятор в контейнере
docker compose up simulator

# С кастомными параметрами:
EVENTS_PER_SECOND=5 SIEM_PORT=8080 docker compose up simulator
```

### Переменные окружения симулятора:

| Переменная | По умолчанию | Описание |
|---|---|---|
| `SIEM_HOST` | `host.docker.internal` | Адрес агента |
| `SIEM_PORT` | `8080` | Порт WebSocket |
| `SIEM_WS_SECRET` | _(пусто)_ | HMAC-ключ |
| `EVENTS_PER_SECOND` | `2` | Интенсивность |

## Безопасность

Для включения HMAC-аутентификации задайте секрет:
```bash
export SIEM_WS_SECRET="your-secret-key"
./build/SIEMAgent
```

Тот же ключ укажите в симуляторе:
```bash
SIEM_WS_SECRET="your-secret-key" python3 scripts/simulator.py
```

## Тесты

```bash
cd build
ctest --output-on-failure
```
