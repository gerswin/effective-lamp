#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <memory>
#include <string>
#include <libpq-fe.h>
#include "common.hpp"

namespace db {

class Database {
public:
    static Database& getInstance();

    bool connect(const barcode_access::DatabaseConfig& config);
    void disconnect();
    bool isConnected() const;

    // Execute query and return result
    PGresult* execute(const std::string& query);
    PGresult* executeParams(const std::string& query,
                            const std::vector<std::string>& params);

    // Transaction support
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();

    // Initialize schema
    bool initializeSchema();

private:
    Database() = default;
    ~Database();
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    PGconn* conn_ = nullptr;
};

// RAII wrapper for PGresult
class PGResultGuard {
public:
    explicit PGResultGuard(PGresult* result) : result_(result) {}
    ~PGResultGuard() { if (result_) PQclear(result_); }

    PGresult* get() const { return result_; }
    PGresult* operator->() const { return result_; }

    bool ok() const {
        if (!result_) return false;
        ExecStatusType status = PQresultStatus(result_);
        return status == PGRES_COMMAND_OK || status == PGRES_TUPLES_OK;
    }

    int rowCount() const { return result_ ? PQntuples(result_) : 0; }
    int colCount() const { return result_ ? PQnfields(result_) : 0; }

    std::string getValue(int row, int col) const {
        if (!result_ || PQgetisnull(result_, row, col)) return "";
        return PQgetvalue(result_, row, col);
    }

private:
    PGresult* result_;
};

} // namespace db

#endif // DATABASE_HPP
