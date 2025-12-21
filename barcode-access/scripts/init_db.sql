-- Barcode Access Control System
-- Database initialization script for PostgreSQL

-- Create database (run as postgres superuser)
-- CREATE DATABASE barcode_access;
-- CREATE USER access_user WITH ENCRYPTED PASSWORD 'your_secure_password';
-- GRANT ALL PRIVILEGES ON DATABASE barcode_access TO access_user;

-- Connect to barcode_access database and run the following:

-- Enable UUID extension
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
CREATE INDEX IF NOT EXISTS idx_tickets_created_at ON tickets(created_at);

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

-- Grant permissions to access_user
GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA public TO access_user;
GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA public TO access_user;

-- Sample data for testing (optional)
-- INSERT INTO tickets (uuid) VALUES
--     ('550e8400-e29b-41d4-a716-446655440000'),
--     ('6ba7b810-9dad-11d1-80b4-00c04fd430c8'),
--     ('6ba7b811-9dad-11d1-80b4-00c04fd430c8');

COMMENT ON TABLE tickets IS 'Stores all valid ticket UUIDs for the event';
COMMENT ON TABLE access_logs IS 'Logs all access attempts (granted and denied)';
