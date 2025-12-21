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

```bash
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    libpq-dev \
    libcurl4-openssl-dev \
    libevdev-dev \
    postgresql-client
```

### 2. Compilar

```bash
cd barcode-access
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 3. Configurar base de datos

```bash
# Iniciar PostgreSQL con Docker (desarrollo)
docker-compose up -d postgres

# O configurar PostgreSQL manualmente y ejecutar:
psql -h localhost -U postgres -f scripts/init_db.sql
```

### 4. Configurar servidor

```bash
cp config.ini.example config.ini
# Editar config.ini con los datos de tu base de datos
```

### 5. Configurar cliente

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
