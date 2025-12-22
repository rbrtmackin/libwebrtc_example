// signaling-server.js
// Simple Node.js WebSocket signaling server for WebRTC

const express = require('express');
const http = require('http');
const WebSocket = require('ws');
const path = require('path');

const app = express();
const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

// Serve static files (client.html)
app.use(express.static(__dirname));

// Store client connections
let clients = new Map();

wss.on('connection', (ws) => {
    const clientId = Date.now().toString();
    clients.set(clientId, ws);
    
    console.log(`Client ${clientId} connected. Total clients: ${clients.size}`);
    
    ws.on('message', (message) => {
        try {
            const data = JSON.parse(message);
            console.log(`Received from ${clientId}:`, data.type);
            
            // Forward message to C++ server or other clients
            // For now, just echo back (you'd route to your C++ server here)
            
            if (data.type === 'offer') {
                console.log('Received offer from browser, need to forward to C++ server');
                // In a real setup, forward this to your C++ WebRTC server
            }
            
        } catch (error) {
            console.error('Error parsing message:', error);
        }
    });
    
    ws.on('close', () => {
        clients.delete(clientId);
        console.log(`Client ${clientId} disconnected. Total clients: ${clients.size}`);
    });
    
    ws.on('error', (error) => {
        console.error(`WebSocket error for client ${clientId}:`, error);
    });
});

const PORT = process.env.PORT || 8080;
server.listen(PORT, () => {
    console.log('='.repeat(50));
    console.log('WebRTC Signaling Server Running');
    console.log('='.repeat(50));
    console.log(`Server: http://localhost:${PORT}`);
    console.log(`WebSocket: ws://localhost:${PORT}`);
    console.log('');
    console.log('Open http://localhost:8080/client.html in your browser');
    console.log('='.repeat(50));
});
