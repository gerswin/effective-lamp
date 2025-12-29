# Barcode Access Control System - Gemini Context

## Project Overview

This project implements a barcode/QR code access control system for events, integrated with Hikvision DS-K2604T controllers. It consists of a central server (Oat++ API with PostgreSQL DB and Web Admin UI) and multiple client applications for each door.

**Key Technologies:**
*   **Backend:** C++17, Oat++ (web framework), Oatpp-PostgreSQL (ORM), PostgreSQL
*   **Frontend:** HTML/JS (for Web Admin UI)
*   **Client:** C++17, interacting with USB barcode readers and Hikvision controllers.

## Building and Running

The project requires CMake 3.16+, GCC 9+ (or Clang 10+), libpq-dev, libcurl4-openssl-dev, and libevdev-dev.

### Dependencies Installation (Debian/Ubuntu)

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

### Oat++ and Oatpp-PostgreSQL Installation

It is recommended to install `oatpp` (v1.3.0) and `oatpp-postgresql` (v1.3.0) manually into the system.

#### Install `oatpp`

```bash
git clone --depth 1 --branch 1.3.0 https://github.com/oatpp/oatpp.git
cd oatpp
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
cd ../..
rm -rf oatpp
```

#### Install `oatpp-postgresql`

```bash
git clone --depth 1 --branch 1.3.0 https://github.com/oatpp/oatpp-postgresql.git
cd oatpp-postgresql
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
cd ../..
rm -rf oatpp-postgresql
```

### Compile the Project

From the project root directory:

```bash
rm -rf build
mkdir build && cd build
cmake ..
make -j$(nproc)
```

Executables will be found in `build/server/access-server` and `build/client/access-client`.

### Database Configuration

To set up the PostgreSQL database:

```bash
# Start PostgreSQL with Docker (for development)
docker-compose up -d postgres

# OR configure manually and run:
psql -h localhost -U postgres -f scripts/init_db.sql
```

Copy and edit `config.ini.example` to `config.ini` for server database settings.
Copy and edit `client.ini.example` to `client.ini` for client door ID, input device, server URL, and Hikvision controller credentials.

### Running the Server and Client

**Server:**
```bash
./access-server [config.ini]
```
The server exposes REST API and Web Admin UI on port 8080.

**Client:**
```bash
sudo ./access-client [client.ini]
```
`sudo` or `input` group permissions are required for USB device access.

## API Endpoints

The server provides the following API endpoints:

*   **/api/tickets**: CRUD operations for tickets.
*   **/api/access/validate**: Validate and use a ticket.
*   **/api/access/logs**: Retrieve access logs.
*   **/api/stats**: General statistics.

## Environment Variables

Both server and client can be configured using environment variables, overriding `config.ini` and `client.ini` respectively.

**Server:** `SERVER_HOST`, `SERVER_PORT`, `DB_HOST`, `DB_PORT`, `DB_NAME`, `DB_USER`, `DB_PASSWORD`
**Client:** `DOOR_ID`, `INPUT_DEVICE`, `SERVER_URL`, `HIK_HOST`, `HIK_PORT`, `HIK_USER`, `HIK_PASSWORD`

## Development Conventions

*   **Language:** C++17
*   **Build System:** CMake
*   **Frameworks:** Oat++ for web services and ORM.
*   **Testing:** The `tests/` directory indicates unit and integration tests are present.
*   **Code Structure:** Follows a common C++ project structure with `client/`, `server/`, `common/`, and `tests/` directories. DTOs and services are organized under `server/src/dto` and `server/src/service`.

## License

MIT License
