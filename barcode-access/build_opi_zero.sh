#!/bin/bash
set -e

# Nombre de la carpeta de salida
OUTPUT_DIR="Orange Pi Zero"
IMAGE_NAME="barcode-access-opi-zero"
# ARMv7 es la arquitectura para Orange Pi Zero (Allwinner H2+)
PLATFORM="linux/arm/v7"

echo "=== Compilación Cruzada para Orange Pi Zero (ARMv7) ==="
echo "Carpeta de destino: $OUTPUT_DIR"

# Crear carpeta de salida
mkdir -p "$OUTPUT_DIR"

# Intentar habilitar emulación QEMU
echo "[1/4] Verificando emulación QEMU..."
if docker run --rm --privileged multiarch/qemu-user-static --reset -p yes > /dev/null 2>&1; then
    echo "      Emulación QEMU configurada."
else
    echo "      (Nota: Si falla el siguiente paso, asegúrate de tener qemu-user-static instalado)"
fi

# Construir la imagen Docker
echo "[2/4] Construyendo imagen Docker para $PLATFORM..."
# Reutilizamos Dockerfile.rpi porque los pasos de instalación son idénticos para Debian
docker build --platform $PLATFORM -f Dockerfile.rpi -t $IMAGE_NAME .

# Crear un contenedor temporal
echo "[3/4] Extrayendo binarios compilados..."
CONTAINER_ID=$(docker create --platform $PLATFORM $IMAGE_NAME)

# Copiar archivos
docker cp $CONTAINER_ID:/app/build/server/access-server "$OUTPUT_DIR/access-server"
docker cp $CONTAINER_ID:/app/build/client/access-client "$OUTPUT_DIR/access-client"
docker cp $CONTAINER_ID:/app/config.ini.example "$OUTPUT_DIR/config.ini"
docker cp $CONTAINER_ID:/app/client.ini.example "$OUTPUT_DIR/client.ini"

# Limpiar contenedor
docker rm -v $CONTAINER_ID > /dev/null

echo "[4/4] ¡Éxito!"
echo "---------------------------------------------------------"
echo "Archivos guardados en: $(pwd)/$OUTPUT_DIR"
echo "---------------------------------------------------------"
