# Barcode Access Control System

Sistema de control de acceso mediante códigos de barras/QR para eventos, integrado con controladoras Hikvision DS-K2604T.

## Arquitectura

```
┌─────────────────────────────────────────────────────────────┐
│                     Servidor Central                         │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │
│  │   Oat++     │  │ PostgreSQL  │  │     Web Admin UI    │  │
│  │   API       │  │     DB      │  │     (HTML/JS)       │  │
│  └─────────────┘  └─────────────┘  └─────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                              │
        ┌─────────────────────┼─────────────────────┐
        │                     │                     │
   ┌────▼────┐           ┌────▼────┐          ┌────▼────┐
   │ Cliente │           │ Cliente │          │ Cliente │
   │ Puerta1 │           │ Puerta2 │          │ Puerta3 │
   └────┬────┘           └────┬────┘          └────┬────┘
        │                     │                     │
        └─────────────────────┼─────────────────────┘
                              │
                    ┌─────────▼─────────┐
                    │    DS-K2604T      │
                    │   (4 puertas)     │
                    └───────────────────┘
```

## Requisitos

### Compilación
- CMake 3.16+
- GCC 9+ o Clang 10+ (C++17)
- libpq-dev (PostgreSQL client)
- libcurl4-openssl-dev
- libevdev-dev (para lector USB)

### Ejecución
- PostgreSQL 12+
- Controladora Hikvision DS-K2604T (o compatible)
- Lectores de código de barras USB (modo HID)

## Instalación

### 1. Instalar dependencias (Debian/Ubuntu)

Asegúrate de tener las herramientas de compilación y las bibliotecas necesarias.

```bash
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    pkg-config \
    libpq-dev \
    libcurl4-openssl-dev \
    libevdev-dev \
    libatomic1 \
    postgresql-client
```

### 2. Instalar dependencias de `oatpp` (Método Manual)

Este proyecto depende de las bibliotecas `oatpp` y `oatpp-postgresql`. El método de compilación actual que usa `FetchContent` de CMake puede ser inestable. La forma más robusta de compilar es instalar estas dependencias manualmente en el sistema antes de compilar el proyecto principal.

> **Nota**: Realiza estos pasos en un directorio temporal fuera del proyecto.

#### a. Instalar `oatpp` (v1.3.0)

```bash
# Clona el repositorio
git clone --depth 1 --branch 1.3.0 https://github.com/oatpp/oatpp.git
cd oatpp

# Compila e instala
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
cd ../..
rm -rf oatpp
```

#### b. Instalar `oatpp-postgresql` (v1.3.0)

```bash
# Clona el repositorio
git clone --depth 1 --branch 1.3.0 https://github.com/oatpp/oatpp-postgresql.git
cd oatpp-postgresql

# Compila e instala
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
cd ../..
rm -rf oatpp-postgresql
```

### 3. Compilar el Proyecto

Una vez instaladas las dependencias, puedes compilar el proyecto principal.

```bash
# Desde el directorio raíz del proyecto
rm -rf build # Limpia el directorio de compilación anterior si existe
mkdir build && cd build
cmake ..
make -j$(nproc)
```

Los ejecutables se encontrarán en `build/server/access-server` y `build/client/access-client`.

### 4. Configurar base de datos

```bash
# Iniciar PostgreSQL con Docker (desarrollo)
docker-compose up -d postgres

# O configurar PostgreSQL manualmente y ejecutar:
psql -h localhost -U postgres -f scripts/init_db.sql
```

### 5. Configurar servidor

```bash
cp config.ini.example config.ini
# Editar config.ini con los datos de tu base de datos
```

### 6. Configurar cliente

```bash
cp client.ini.example client.ini
# Editar client.ini con:
# - ID de puerta (1-4)
# - Dispositivo de entrada del lector
# - URL del servidor central
# - Credenciales de la controladora Hikvision
```

## Uso

### Servidor Central

```bash
./access-server [config.ini]
```

El servidor expone:
- **Puerto 8080**: API REST y Web Admin UI

### Cliente de Puerta

```bash
sudo ./access-client [client.ini]
```

> **Nota**: Se requiere `sudo` o permisos de grupo `input` para leer dispositivos USB.

### Encontrar dispositivo de entrada

```bash
# Listar dispositivos de entrada
ls -la /dev/input/by-id/

# Ver eventos en tiempo real (para identificar el lector)
sudo evtest
```

## API Endpoints

### Tickets

| Método | Endpoint | Descripción |
|--------|----------|-------------|
| POST | `/api/tickets` | Crear ticket |
| GET | `/api/tickets` | Listar tickets |
| GET | `/api/tickets/{uuid}` | Obtener ticket |
| DELETE | `/api/tickets/{uuid}` | Eliminar ticket |

### Acceso

| Método | Endpoint | Descripción |
|--------|----------|-------------|
| POST | `/api/access/validate` | Validar y usar ticket |
| GET | `/api/access/logs` | Listar logs de acceso |
| GET | `/api/access/logs/door/{id}` | Logs por puerta |
| GET | `/api/access/logs/ticket/{uuid}` | Logs por ticket |

### Estadísticas

| Método | Endpoint | Descripción |
|--------|----------|-------------|
| GET | `/api/stats` | Estadísticas generales |

## Variables de Entorno

### Servidor
- `SERVER_HOST`: IP de escucha (default: 0.0.0.0)
- `SERVER_PORT`: Puerto (default: 8080)
- `DB_HOST`: Host PostgreSQL
- `DB_PORT`: Puerto PostgreSQL
- `DB_NAME`: Nombre de base de datos
- `DB_USER`: Usuario
- `DB_PASSWORD`: Contraseña

### Cliente
- `DOOR_ID`: ID de puerta (1-4)
- `INPUT_DEVICE`: Ruta al dispositivo de entrada
- `SERVER_URL`: URL del servidor central
- `HIK_HOST`: IP de la controladora Hikvision
- `HIK_PORT`: Puerto (default: 80)
- `HIK_USER`: Usuario
- `HIK_PASSWORD`: Contraseña

## Licencia

MIT License
