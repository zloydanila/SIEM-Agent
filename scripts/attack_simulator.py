import asyncio
import websockets
import ssl
import json
import random
import uuid
import hmac
import hashlib
import os
import argparse
from datetime import datetime, timezone

WS_HOST = "localhost"
WS_PORT = 8080
SEND_INTERVAL = 2.0
SHARED_SECRET = os.environ.get("SIEM_WS_SECRET", "")
if not SHARED_SECRET:
    print("[WARNING] SIEM_WS_SECRET не задан — сообщения пойдут без подписи")

DEVICES = [
    {"name": "Насос-01", "location": "Насосная станция"},
    {"name": "Котел-02", "location": "Котельная"},
    {"name": "Датчик температуры", "location": "Цех 1"},
    {"name": "Датчик давления", "location": "Цех 2"},
    {"name": "Контроллер линии А", "location": "Производственная линия А"},
    {"name": "Контроллер линии Б", "location": "Производственная линия Б"},
    {"name": "Камера видеонадзора", "location": "Периметр / Вход"},
    {"name": "Сервер управления", "location": "Серверная"},
]

EVENTS = {
    "low": [
        {"eventType": "sensor_poll", "action": "Плановый опрос датчика", "rawLog": "Датчик опрошен успешно. Показания в норме."},
        {"eventType": "heartbeat", "action": "Проверка связи", "rawLog": "Устройство активно, связь в норме."},
        {"eventType": "config_backup", "action": "Резервное копирование", "rawLog": "Конфигурация устройства успешно сохранена."},
    ],
    "medium": [
        {"eventType": "auth_failure", "action": "Ошибка входа", "rawLog": "Неверный пароль при попытке входа в систему управления."},
        {"eventType": "config_change", "action": "Изменение настроек", "rawLog": "Оператор изменил рабочий параметр устройства."},
        {"eventType": "network_scan", "action": "Сканирование сети", "rawLog": "Обнаружено сканирование портов с неизвестного адреса."},
    ],
    "high": [
        {"eventType": "brute_force", "action": "Подбор пароля", "rawLog": "Зафиксировано 10 неудачных попыток входа подряд — возможная атака."},
        {"eventType": "process_anomaly", "action": "Превышение допустимых значений", "rawLog": "Показание датчика вышло за допустимый предел — требуется проверка."},
        {"eventType": "unauthorized_access", "action": "Несанкционированный доступ", "rawLog": "Попытка доступа к устройству с неизвестного IP-адреса."},
    ],
    "critical": [
        {"eventType": "emergency_stop", "action": "Аварийная остановка", "rawLog": "КРИТИЧНО: Получена команда аварийной остановки из неизвестного источника."},
        {"eventType": "safety_bypass", "action": "Отключение защиты", "rawLog": "КРИТИЧНО: Система защитных блокировок отключена без разрешения."},
        {"eventType": "malware_detected", "action": "Обнаружена угроза", "rawLog": "КРИТИЧНО: На устройстве обнаружено вредоносное программное обеспечение."},
    ],
}

SEVERITY_WEIGHTS = {"low": 60, "medium": 25, "high": 12, "critical": 3}


def generate_event(severity=None, event_type=None, device=None, raw_log=None, action=None):
    if severity is None:
        severity = random.choices(list(SEVERITY_WEIGHTS.keys()), weights=list(SEVERITY_WEIGHTS.values()), k=1)[0]
    if device is None:
        device = random.choice(DEVICES)
    if event_type is None:
        template = random.choice(EVENTS[severity])
    else:
        template = {"eventType": event_type, "action": action or event_type, "rawLog": raw_log or event_type}

    return {
        "id": str(uuid.uuid4()),
        "deviceName": device["name"],
        "location": device["location"],
        "eventType": template["eventType"],
        "action": template["action"],
        "severity": severity,
        "timestamp": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%S"),
        "rawLog": template["rawLog"],
    }


def sign_event(event: dict, secret: str) -> dict:
    if not secret:
        return event
    nonce = str(uuid.uuid4())
    payload = event["deviceName"] + event["eventType"] + nonce + secret
    signature = hmac.new(secret.encode(), payload.encode(), digestmod=hashlib.sha256).hexdigest()
    event["nonce"] = nonce
    event["signature"] = signature
    return event


async def send_event(ws, event, sent):
    await ws.send(json.dumps(event, ensure_ascii=False))
    sent += 1
    print(f"[{sent:04d}] [{event['severity'].upper():8s}] {event['deviceName']} — {event['action']}")
    return sent


async def attack_sequence(ws, sent):
    device = next(d for d in DEVICES if d["name"] == "Сервер управления")
    for _ in range(3):
        e = generate_event(severity="medium", event_type="auth_failure", device=device,
                           action="Ошибка входа", raw_log="Тест атаки: auth_failure")
        e = sign_event(e, SHARED_SECRET)
        sent = await send_event(ws, e, sent)
        await asyncio.sleep(0.05)  

    e = generate_event(severity="low", event_type="config_backup", device=device,
                       action="Резервное копирование", raw_log="Тест атаки: config_backup")
    e = sign_event(e, SHARED_SECRET)
    sent = await send_event(ws, e, sent)
    return sent


async def run_simulator(mode):
    uri = f"wss://{WS_HOST}:{WS_PORT}"
    print(f"Подключение к {uri} ...")
    ssl_ctx = ssl.create_default_context()
    ssl_ctx.check_hostname = False
    ssl_ctx.verify_mode = ssl.CERT_NONE
    sent = 0
    try:
        async with websockets.connect(uri, ssl=ssl_ctx) as ws:
            print("Подключено.")
            if mode == "attack":
                print("Запускаю тест атаки: 3x auth_failure -> 1x config_backup\n")
                sent = await attack_sequence(ws, sent)
                return
            print(f"Отправка событий каждые {SEND_INTERVAL}с")
            while True:
                event = generate_event()
                event = sign_event(event, SHARED_SECRET)
                sent = await send_event(ws, event, sent)
                await asyncio.sleep(SEND_INTERVAL)
    except ConnectionRefusedError:
        print(f"Ошибка: не удалось подключиться к {uri}")
        print("Убедись, что SIEM Agent запущен и порт 8080 открыт.")
    except KeyboardInterrupt:
        print(f"Остановлено вручную. Отправлено событий: {sent}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--mode", choices=["normal", "attack"], default="normal")
    args = parser.parse_args()
    asyncio.run(run_simulator(args.mode))
