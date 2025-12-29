// Barcode Access Control - Admin Panel JavaScript

const API_BASE = '/api';

// State
let currentTicketsPage = 0;
let currentLogsPage = 0;
const PAGE_SIZE = 20;

// Utility functions
function generateUUID() {
    return 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function(c) {
        const r = Math.random() * 16 | 0;
        const v = c === 'x' ? r : (r & 0x3 | 0x8);
        return v.toString(16);
    });
}

function formatDate(dateStr) {
    if (!dateStr) return '-';
    const date = new Date(dateStr);
    return date.toLocaleString('es-ES', {
        year: 'numeric',
        month: '2-digit',
        day: '2-digit',
        hour: '2-digit',
        minute: '2-digit',
        second: '2-digit'
    });
}

function showMessage(elementId, message, isSuccess) {
    const el = document.getElementById(elementId);
    el.textContent = message;
    el.className = 'form-message ' + (isSuccess ? 'success' : 'error');
    setTimeout(() => {
        el.className = 'form-message';
    }, 5000);
}

// API calls
async function apiCall(endpoint, method = 'GET', body = null) {
    const options = {
        method,
        headers: {
            'Content-Type': 'application/json'
        }
    };
    if (body) {
        options.body = JSON.stringify(body);
    }
    const response = await fetch(API_BASE + endpoint, options);
    return await response.json();
}

// Load stats
async function loadStats() {
    try {
        const data = await apiCall('/stats');
        if (data.success) {
            const stats = data.stats;
            document.getElementById('stat-total').textContent = stats.total_tickets || 0;
            document.getElementById('stat-available').textContent = stats.available_tickets || 0;
            document.getElementById('stat-used').textContent = stats.used_tickets || 0;
            document.getElementById('stat-attempts').textContent = stats.total_access_attempts || 0;
            document.getElementById('stat-granted').textContent = stats.granted_access || 0;
            document.getElementById('stat-denied').textContent = stats.denied_access || 0;

            // Update door stats
            updateDoorStats(stats.door_stats || []);
        }
    } catch (error) {
        console.error('Error loading stats:', error);
    }
}

// Update door stats
function updateDoorStats(doorStats) {
    const container = document.getElementById('doors-stats');

    // Create cards for doors 1-4
    let html = '';
    for (let i = 1; i <= 4; i++) {
        const stats = doorStats.find(d => d.door_id === i) || {
            door_id: i,
            total_attempts: 0,
            granted: 0,
            denied: 0
        };

        html += `
            <div class="door-card">
                <h3>
                    <span class="door-icon">P${i}</span>
                    Puerta ${i}
                </h3>
                <div class="door-stats">
                    <div class="door-stat-item">
                        <span class="value">${stats.total_attempts}</span>
                        <span class="label">Total</span>
                    </div>
                    <div class="door-stat-item">
                        <span class="value" style="color: var(--success-color)">${stats.granted}</span>
                        <span class="label">Permitidos</span>
                    </div>
                    <div class="door-stat-item">
                        <span class="value" style="color: var(--danger-color)">${stats.denied}</span>
                        <span class="label">Denegados</span>
                    </div>
                </div>
            </div>
        `;
    }
    container.innerHTML = html;
}

// Load tickets
async function loadTickets(page = 0) {
    currentTicketsPage = page;
    const filter = document.getElementById('filter-tickets').value;
    const search = document.getElementById('search-tickets').value;

    try {
        const data = await apiCall(`/tickets?limit=${PAGE_SIZE}&offset=${page * PAGE_SIZE}&filter=${filter}&search=${search}`);
        if (data.success) {
            renderTicketsTable(data.tickets || []);
            renderPagination('tickets-pagination', data.total, page, loadTickets);
        }
    } catch (error) {
        console.error('Error loading tickets:', error);
    }
}

function renderTicketsTable(tickets) {
    const tbody = document.getElementById('tickets-table-body');

    if (tickets.length === 0) {
        tbody.innerHTML = '<tr><td colspan="8" style="text-align: center">No hay tickets</td></tr>';
        return;
    }

    tbody.innerHTML = tickets.map(ticket => `
        <tr>
            <td class="uuid">${ticket.uuid}</td>
            <td>
                <span class="badge ${ticket.used ? 'badge-warning' : 'badge-success'}">
                    ${ticket.used ? 'Usado' : 'Disponible'}
                </span>
            </td>
            <td>${ticket.maxUses === -1 ? 'Ilimitado' : ticket.maxUses}</td>
            <td>${ticket.currentUses}</td>
            <td>${ticket.used_at ? formatDate(ticket.used_at) : '-'}</td>
            <td>${ticket.used_at_door ? 'Puerta ' + ticket.used_at_door : '-'}</td>
            <td>${formatDate(ticket.created_at)}</td>
            <td>
                <button class="btn btn-danger btn-sm" onclick="deleteTicket('${ticket.uuid}')">
                    Eliminar
                </button>
            </td>
        </tr>
    `).join('');
}

// Load access logs
async function loadLogs(page = 0) {
    currentLogsPage = page;
    const filter = document.getElementById('filter-logs').value;

    try {
        const data = await apiCall(`/access/logs?limit=${PAGE_SIZE}&offset=${page * PAGE_SIZE}`);
        if (data.success) {
            let logs = data.logs || [];

            // Client-side filtering
            if (filter === 'granted') {
                logs = logs.filter(l => l.granted);
            } else if (filter === 'denied') {
                logs = logs.filter(l => !l.granted);
            }

            renderLogsTable(logs);
            renderPagination('logs-pagination', data.total, page, loadLogs);
        }
    } catch (error) {
        console.error('Error loading logs:', error);
    }
}

function renderLogsTable(logs) {
    const tbody = document.getElementById('logs-table-body');

    if (logs.length === 0) {
        tbody.innerHTML = '<tr><td colspan="7" style="text-align: center">No hay logs</td></tr>';
        return;
    }

    tbody.innerHTML = logs.map(log => `
        <tr>
            <td>${log.id}</td>
            <td class="uuid">${log.ticket_uuid || '-'}</td>
            <td>Puerta ${log.door_id}</td>
            <td>
                <span class="badge ${log.granted ? 'badge-success' : 'badge-danger'}">
                    ${log.granted ? 'Permitido' : 'Denegado'}
                </span>
            </td>
            <td>${log.reason || '-'}</td>
            <td>${log.attempts}</td>
            <td>${formatDate(log.scanned_at)}</td>
        </tr>
    `).join('');
}

// Render pagination
function renderPagination(containerId, total, currentPage, loadFunction) {
    const container = document.getElementById(containerId);
    const totalPages = Math.ceil(total / PAGE_SIZE);

    if (totalPages <= 1) {
        container.innerHTML = '';
        return;
    }

    let html = '';

    // Previous button
    html += `<button ${currentPage === 0 ? 'disabled' : ''} onclick="${loadFunction.name}(${currentPage - 1})">Anterior</button>`;

    // Page numbers
    for (let i = 0; i < totalPages; i++) {
        if (i === 0 || i === totalPages - 1 || Math.abs(i - currentPage) <= 2) {
            html += `<button class="${i === currentPage ? 'active' : ''}" onclick="${loadFunction.name}(${i})">${i + 1}</button>`;
        } else if (i === currentPage - 3 || i === currentPage + 3) {
            html += `<button disabled>...</button>`;
        }
    }

    // Next button
    html += `<button ${currentPage === totalPages - 1 ? 'disabled' : ''} onclick="${loadFunction.name}(${currentPage + 1})">Siguiente</button>`;

    container.innerHTML = html;
}

// Create ticket
async function createTicket() {
    const uuid = document.getElementById('ticket-uuid').value.trim();
    const maxUses = parseInt(document.getElementById('ticket-max-uses').value.trim());

    if (!uuid) {
        showMessage('ticket-message', 'UUID es requerido', false);
        return;
    }

    try {
        const data = await apiCall('/tickets', 'POST', { uuid, maxUses });
        if (data.success) {
            showMessage('ticket-message', 'Ticket creado correctamente', true);
            document.getElementById('ticket-uuid').value = '';
            document.getElementById('ticket-max-uses').value = '-1';
            loadTickets(currentTicketsPage);
            loadStats();
        } else {
            showMessage('ticket-message', data.message || 'Error al crear ticket', false);
        }
    } catch (error) {
        showMessage('ticket-message', 'Error de conexión', false);
    }
}

// Delete ticket
async function deleteTicket(uuid) {
    if (!confirm('¿Está seguro de eliminar este ticket?')) return;

    try {
        const data = await apiCall(`/tickets/${uuid}`, 'DELETE');
        if (data.success) {
            loadTickets(currentTicketsPage);
            loadStats();
        }
    } catch (error) {
        console.error('Error deleting ticket:', error);
    }
}

// Tab switching
function initTabs() {
    document.querySelectorAll('.tab').forEach(tab => {
        tab.addEventListener('click', () => {
            // Remove active class from all tabs and contents
            document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
            document.querySelectorAll('.tab-content').forEach(c => c.classList.remove('active'));

            // Add active class to clicked tab and corresponding content
            tab.classList.add('active');
            document.getElementById('tab-' + tab.dataset.tab).classList.add('active');

            // Load data for the tab
            if (tab.dataset.tab === 'tickets') {
                loadTickets(currentTicketsPage);
            } else if (tab.dataset.tab === 'logs') {
                loadLogs(currentLogsPage);
            }
        });
    });
}

// Event listeners
document.addEventListener('DOMContentLoaded', () => {
    initTabs();

    // Add ticket form
    document.getElementById('add-ticket-form').addEventListener('submit', (e) => {
        e.preventDefault();
        createTicket();
    });

    // Generate UUID button
    document.getElementById('generate-uuid').addEventListener('click', () => {
        document.getElementById('ticket-uuid').value = generateUUID();
    });

    // Filter/search event listeners
    document.getElementById('filter-tickets').addEventListener('change', () => loadTickets(0));
    document.getElementById('search-tickets').addEventListener('input', () => loadTickets(0));
    document.getElementById('filter-logs').addEventListener('change', () => loadLogs(0));

    // Initial load
    loadStats();
    loadTickets(0);

    // Auto-refresh stats every 5 seconds
    setInterval(loadStats, 5000);
});
