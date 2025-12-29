-- Update script for Client Auto-Provisioning (v2)

-- Client Configurations table
-- Stores the mapping between Hardware ID and Door ID
CREATE TABLE IF NOT EXISTS client_configs (
    hardware_id VARCHAR(64) PRIMARY KEY, -- Unique ID from client (e.g., UUID or MAC)
    door_id INTEGER NOT NULL,            -- Assigned Door ID (1-4)
    description VARCHAR(100),            -- Friendly name (e.g., "Main Entrance")
    
    -- Network config (optional override)
    hik_host VARCHAR(64),
    hik_port INTEGER DEFAULT 80,
    hik_user VARCHAR(64),
    hik_password VARCHAR(64),
    
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    last_seen_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- Index for door_id
CREATE INDEX IF NOT EXISTS idx_client_configs_door ON client_configs(door_id);

COMMENT ON TABLE client_configs IS 'Stores auto-provisioning configuration for clients';
