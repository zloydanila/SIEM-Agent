#!/bin/bash
set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${GREEN}"
echo "SIEM Agent - Installer"


if [ ! -f /etc/os-release ]; then
    echo -e "${RED}Неподдерживаемая ОС${NC}"
    exit 1
fi


echo -e "${YELLOW}[1/4] Установка зависимостей...${NC}"
sudo apt-get update -q
sudo apt-get install -y \
    cmake \
    ninja-build \
    qt6-base-dev \
    qt6-websockets-dev \
    qt6-declarative-dev \
    libqt6sql6-sqlite \
    openssl \
    python3 \
    python3-pip

echo -e "${GREEN}✓ Зависимости установлены${NC}"

echo -e "${YELLOW}  Установка Python-зависимостей...${NC}"
if apt-cache show python3-websockets &>/dev/null; then
    sudo apt-get install -y python3-websockets
    echo -e "${GREEN}  ✓ python3-websockets установлен через apt${NC}"
else
    python3 -m venv .venv --system-site-packages
    .venv/bin/pip install --quiet websockets
    echo -e "${GREEN}  ✓ websockets установлен в .venv${NC}"
fi

echo -e "${YELLOW}[2/4] Генерация SSL сертификатов...${NC}"
mkdir -p certs
if [ ! -f certs/server.crt ]; then
    openssl req -x509 -newkey rsa:4096 -sha256 -days 3650 \
        -nodes \
        -keyout certs/server.key \
        -out certs/server.crt \
        -subj "/CN=siem-agent/O=NSTU/C=RU" \
        -addext "subjectAltName=IP:127.0.0.1,IP:0.0.0.0"
    echo -e "${GREEN}Сертификаты созданы в ./certs/${NC}"
else
    echo -e "${YELLOW} Сертификаты уже существуют, пропускаем${NC}"
fi

# Сборка
echo -e "${YELLOW}[3/4] Сборка проекта...${NC}"
mkdir -p build
cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -j$(nproc)
cd ..
echo -e "${GREEN} Сборка завершена${NC}"

# Конфиг
echo -e "${YELLOW}[4/4] Создание конфигурации...${NC}"
mkdir -p data/config
if [ ! -f data/config/app_config.json ]; then
    cat > data/config/app_config.json << EOF
{
    "cert_path": "certs/server.crt",
    "key_path":  "certs/server.key"
}
EOF
    echo -e "${GREEN} Конфиг создан${NC}"
else
    echo -e "${YELLOW}Конфиг уже существует, пропускаем${NC}"
fi

echo ""

echo "Установка завершена успешно"

echo ""
echo "Запуск агента:"
echo -e "  ${YELLOW}./build/SIEMAgent${NC}"
echo ""
echo "Запуск симулятора (в отдельном терминале):"
if [ -d ".venv" ]; then
    echo -e "  ${YELLOW}.venv/bin/python3 scripts/simulator.py${NC}"
else
    echo -e "  ${YELLOW}python3 scripts/simulator.py${NC}"
fi
echo ""
echo "Запуск симулятора в Docker:"
echo -e "  ${YELLOW}docker compose up simulator${NC}"
echo ""