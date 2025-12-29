-- Migration script to change ticket UUIDs to NanoIDs (VARCHAR)

-- 1. Alter 'tickets' table
-- We need to change the type from UUID to VARCHAR(20)
-- Since we are moving from specific (UUID) to general (VARCHAR), implicit casting should work for existing data.
ALTER TABLE tickets ALTER COLUMN uuid TYPE VARCHAR(20) USING uuid::text;

-- 2. Alter 'access_logs' table
ALTER TABLE access_logs ALTER COLUMN ticket_uuid TYPE VARCHAR(20) USING ticket_uuid::text;

-- 3. Drop UUID extension usage (optional, kept if used elsewhere)
-- DROP EXTENSION IF EXISTS "uuid-ossp";
