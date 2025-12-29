#!/bin/bash
set -e

# Nombre de la carpeta de salida
OUTPUT_DIR="Pi Zero W original 2017"
IMAGE_NAME="barcode-access-rpi-zero"

echo "=== Compilación Cruzada para Raspberry Pi Zero W (ARMv6) ==="
echo "Carpeta de destino: $OUTPUT_DIR"

# Crear carpeta de salida
mkdir -p "$OUTPUT_DIR"

# Intentar habilitar emulación QEMU (necesario para compilar ARM en x86)
echo "[1/4] Verificando emulación QEMU..."
if docker run --rm --privileged multiarch/qemu-user-static --reset -p yes > /dev/null 2>&1; then
    echo "      Emulación QEMU configurada."
else
    echo "      (Nota: Si falla el siguiente paso, asegúrate de tener qemu-user-static instalado)"
fi

# Construir la imagen Docker
echo "[2/4] Construyendo imagen Docker (esto puede tardar unos minutos)..."
# Usamos --platform linux/arm/v6 explícitamente
docker build --platform linux/arm/v6 -f Dockerfile.rpi -t $IMAGE_NAME .

# Crear un contenedor temporal para copiar los archivos
echo "[3/4] Extrayendo binarios compilados..."
CONTAINER_ID=$(docker create --platform linux/arm/v6 $IMAGE_NAME)

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
echo "Recuerda copiar la carpeta entera a tu Raspberry Pi."
echo "---------------------------------------------------------"
