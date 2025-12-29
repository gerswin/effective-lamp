#include "db/database.hpp"
#include <sstream>
#include <iostream>
#include <mutex>

namespace db {

Database& Database::getInstance() {
    static Database instance;
    return instance;
}

Database::~Database() {
    disconnect();
}

bool Database::connect(const barcode_access::DatabaseConfig& config) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_config = config; // Store config
    return reconnect();
}

void Database::disconnect() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (conn_) {
        PQfinish(conn_);
        conn_ = nullptr;
    }
}

bool Database::reconnect() {
    // Lock is already acquired in connect/execute/executeParams before calling reconnect.
    // std::lock_guard<std::mutex> lock(m_mutex); // No need to lock here again, as caller already holds the lock.
    disconnect(); // disconnect already acquires the lock.
    std::ostringstream connStr;
    connStr << "host=" << m_config.host
            << " port=" << m_config.port
            << " dbname=" << m_config.database
            << " user=" << m_config.user;

    if (!m_config.password.empty()) {
        connStr << " password=" << m_config.password;
    }

    conn_ = PQconnectdb(connStr.str().c_str());

    if (PQstatus(conn_) != CONNECTION_OK) {
        std::cerr << "Database connection failed: " << PQerrorMessage(conn_) << std::endl;
        PQfinish(conn_);
        conn_ = nullptr;
        return false;
    }

    std::cout << "Database reconnected successfully" << std::endl;
    return true;
}

bool Database::isConnected() const {
    return conn_ && PQstatus(conn_) == CONNECTION_OK;
}

PGresult* Database::execute(const std::string& query) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (!isConnected()) {
        std::cerr << "Database not connected. Reconnecting..." << std::endl;
        if(!reconnect()) {
            return nullptr;
        }
    }
    return PQexec(conn_, query.c_str());
}

PGresult* Database::executeParams(const std::string& query,
                                   const std::vector<std::string>& params) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (!isConnected()) {
        std::cerr << "Database not connected. Reconnecting..." << std::endl;
        if(!reconnect()) {
            return nullptr;
        }
    }

    std::vector<const char*> paramValues;
    paramValues.reserve(params.size());
    for (const auto& param : params) {
        paramValues.push_back(param.c_str());
    }

    return PQexecParams(conn_, query.c_str(),
                        static_cast<int>(params.size()),
                        nullptr,  // paramTypes
                        paramValues.data(),
                        nullptr,  // paramLengths
                        nullptr,  // paramFormats
                        0);       // resultFormat (text)
}

bool Database::beginTransaction() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    PGResultGuard result(execute("BEGIN"));
    return result.ok();
}

bool Database::commitTransaction() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    PGResultGuard result(execute("COMMIT"));
    return result.ok();
}

bool Database::rollbackTransaction() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    PGResultGuard result(execute("ROLLBACK"));
    return result.ok();
}

bool Database::initializeSchema() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    const char* schema = R"(
        -- Create extension for UUID if not exists
        CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

        -- Tickets table
        CREATE TABLE IF NOT EXISTS tickets (
            uuid UUID PRIMARY KEY,
            used BOOLEAN DEFAULT FALSE,
            max_uses INTEGER DEFAULT -1,
            current_uses INTEGER DEFAULT 0,
            used_at TIMESTAMP WITH TIME ZONE NULL,
            used_at_door INTEGER NULL,
            created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
        );

        -- Add columns if they don't exist (for migration)
        ALTER TABLE tickets ADD COLUMN IF NOT EXISTS max_uses INTEGER DEFAULT -1;
        ALTER TABLE tickets ADD COLUMN IF NOT EXISTS current_uses INTEGER DEFAULT 0;

        -- Create index on used status for faster queries
        CREATE INDEX IF NOT EXISTS idx_tickets_used ON tickets(used);

        -- Access logs table
        CREATE TABLE IF NOT EXISTS access_logs (
            id SERIAL PRIMARY KEY,
            ticket_uuid UUID,
            door_id INTEGER NOT NULL,
            granted BOOLEAN NOT NULL,
            attempts INTEGER DEFAULT 1,
            reason VARCHAR(100),
            scanned_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
        );

        -- Create indexes for access logs
        CREATE INDEX IF NOT EXISTS idx_access_logs_ticket ON access_logs(ticket_uuid);
        CREATE INDEX IF NOT EXISTS idx_access_logs_door ON access_logs(door_id);
        CREATE INDEX IF NOT EXISTS idx_access_logs_scanned_at ON access_logs(scanned_at);
        CREATE INDEX IF NOT EXISTS idx_access_logs_granted ON access_logs(granted);
    )";

    PGResultGuard result(execute(schema));
    if (!result.ok()) {
        std::cerr << "Failed to initialize schema: " << PQerrorMessage(conn_) << std::endl;
        return false;
    }

    std::cout << "Database schema initialized successfully" << std::endl;
    return true;
}

} // namespace db
