// signaling-relay.js
// Bridges browser WebSocket to C++ HTTP server

const express = require('express');
const http = require('http');
const WebSocket = require('ws');
const axios = require('axios');

const app = express();
const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

// Serve static files
app.use(express.static(__dirname));

// C++ server configuration
const CPP_SERVER = 'http://localhost:9090'; // C++ HTTP endpoint

console.log('\n' + '='.repeat(60));
console.log('🌉 WebRTC Signaling Relay');
console.log('='.repeat(60));
console.log('Relays signaling between:');
console.log(`  Browser ←→ Node.js (port 8080) ←→ C++ Server (port 9090)`);
console.log('='.repeat(60));
console.log('');

wss.on('connection', (ws) => {
    console.log('✅ Browser connected to relay');
    
    ws.on('message', async (message) => {
        try {
            const data = JSON.parse(message);
            console.log(`📨 From browser: ${data.type}`);
            
            // Forward to C++ server via HTTP POST
            try {
                const response = await axios.post(`${CPP_SERVER}/signaling`, data, {
                    timeout: 5000
                });
                
                console.log(`📬 From C++ server: ${response.data.type}`);
                
                // Send C++ response back to browser
                ws.send(JSON.stringify(response.data));
                
            } catch (error) {
                if (error.code === 'ECONNREFUSED') {
                    console.error('❌ C++ server not running on port 9090');
                    ws.send(JSON.stringify({
                        type: 'error',
                        message: 'C++ server not running. Start webrtc_server.exe on port 9090'
                    }));
                } else {
                    console.error('❌ Error connecting to C++ server:', error.message);
                }
            }
            
        } catch (error) {
            console.error('❌ Error processing message:', error);
        }
    });
    
    ws.on('close', () => {
        console.log('👋 Browser disconnected from relay');
    });
    
    ws.on('error', (error) => {
        console.error('❌ WebSocket error:', error);
    });
});

const PORT = 8080;
server.listen(PORT, () => {
    console.log('✅ Signaling relay running on port', PORT);
    console.log('');
    console.log('📝 Next steps:');
    console.log('  1. Start C++ server: webrtc_server.exe (will run on port 9090)');
    console.log('  2. Open browser: http://localhost:8080/client-cpp.html');
    console.log('');
});
