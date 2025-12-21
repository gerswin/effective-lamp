#include "db/database.hpp"
#include <sstream>
#include <iostream>

namespace db {

Database& Database::getInstance() {
    static Database instance;
    return instance;
}

Database::~Database() {
    disconnect();
}

bool Database::connect(const barcode_access::DatabaseConfig& config) {
    std::ostringstream connStr;
    connStr << "host=" << config.host
            << " port=" << config.port
            << " dbname=" << config.database
            << " user=" << config.user;

    if (!config.password.empty()) {
        connStr << " password=" << config.password;
    }

    conn_ = PQconnectdb(connStr.str().c_str());

    if (PQstatus(conn_) != CONNECTION_OK) {
        std::cerr << "Database connection failed: " << PQerrorMessage(conn_) << std::endl;
        PQfinish(conn_);
        conn_ = nullptr;
        return false;
    }

    std::cout << "Database connected successfully" << std::endl;
    return true;
}

void Database::disconnect() {
    if (conn_) {
        PQfinish(conn_);
        conn_ = nullptr;
    }
}

bool Database::isConnected() const {
    return conn_ && PQstatus(conn_) == CONNECTION_OK;
}

PGresult* Database::execute(const std::string& query) {
    if (!isConnected()) {
        std::cerr << "Database not connected" << std::endl;
        return nullptr;
    }
    return PQexec(conn_, query.c_str());
}

PGresult* Database::executeParams(const std::string& query,
                                   const std::vector<std::string>& params) {
    if (!isConnected()) {
        std::cerr << "Database not connected" << std::endl;
        return nullptr;
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
    PGResultGuard result(execute("BEGIN"));
    return result.ok();
}

bool Database::commitTransaction() {
    PGResultGuard result(execute("COMMIT"));
    return result.ok();
}

bool Database::rollbackTransaction() {
    PGResultGuard result(execute("ROLLBACK"));
    return result.ok();
}

bool Database::initializeSchema() {
    const char* schema = R"(
        -- Create extension for UUID if not exists
        CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

        -- Tickets table
        CREATE TABLE IF NOT EXISTS tickets (
            uuid UUID PRIMARY KEY,
            used BOOLEAN DEFAULT FALSE,
            used_at TIMESTAMP WITH TIME ZONE NULL,
            used_at_door INTEGER NULL,
            created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
        );

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
