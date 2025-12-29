# Compilación del Cliente en Raspberry Pi Zero

Para compilar únicamente el cliente en una Raspberry Pi Zero y evitar instalar las dependencias pesadas del servidor (como `oatpp`), sigue estos pasos.

## 1. Instalar dependencias necesarias

Solo necesitas las herramientas de compilación y las librerías que usa el cliente (CURL y evdev).

```bash
sudo apt update
sudo apt install -y build-essential cmake libcurl4-openssl-dev libevdev-dev
```

## 2. Preparar la compilación

El proyecto incluye un archivo `CMakeLists_client.txt` optimizado para esta tarea.

```bash
# Hacemos una copia de seguridad del archivo CMake original (si existe)
if [ -f CMakeLists.txt ]; then
    mv CMakeLists.txt CMakeLists.txt.original
fi

# Usamos el archivo específico para el cliente
cp CMakeLists_client.txt CMakeLists.txt
```

## 3. Compilar

Ahora procedemos a compilar. En una Raspberry Pi Zero (modelo original) esto tomará unos minutos.

```bash
mkdir -p build
cd build
cmake ..
make
```

## 4. Ejecutar

Una vez termine, tendrás el ejecutable `access-client`. Recuerda configurar tu archivo `.ini` antes de ejecutarlo:

```bash
# Copiar configuración de ejemplo (si no lo has hecho ya)
if [ ! -f ../client.ini ]; then
    cp ../client.ini.example ../client.ini
fi

# Ejecutar (requiere permisos para leer USB)
sudo ./access-client ../client.ini
```
