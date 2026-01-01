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
//const CPP_SERVER = 'http://96.237.100.238:19090'; // C++ HTTP endpoint. use localhost internally
const CPP_SERVER = 'http://localhost:9090'; // C++ HTTP endpoint. use localhost internally

console.log('\n' + '='.repeat(60));
console.log('🌉 WebRTC Signaling Relay');
console.log('='.repeat(60));
console.log('Relays signaling between:');
console.log(`  Browser ←→ Node.js (port 8080) ←→ C++ Server (port 9090)`);
console.log('='.repeat(60));
console.log('');

wss.on('connection', (ws) => {
    const sessionId = Date.now().toString() + '-' + Math.random().toString(36).substr(2, 9);
    console.log(`✅ Browser connected - Session ID: ${sessionId}`);
    
    ws.on('message', async (message) => {
        try {
            const data = JSON.parse(message);
            
            // Add session ID to message
            data.sessionId = sessionId;
            
            console.log(`📨 [${sessionId}] From browser: ${data.type}`);
            
            // Forward to C++ server via HTTP POST
            try {
                console.log(`🔄 [${sessionId}] Forwarding to C++ server at ${CPP_SERVER}/signaling`);
                
                const response = await axios.post(`${CPP_SERVER}/signaling`, data, {
                    timeout: 5000,
                    headers: {
                        'Content-Type': 'application/json'
                    }
                });
                
                console.log(`📬 [${sessionId}] From C++ server: ${response.data.type}`);
                
                // Send C++ response back to browser
                ws.send(JSON.stringify(response.data));
                
            } catch (error) {
                if (error.code === 'ECONNREFUSED') {
                    console.error(`❌ [${sessionId}] C++ server not running on port 9090`);
                    ws.send(JSON.stringify({
                        type: 'error',
                        message: 'C++ server not running. Start webrtc_server.exe on port 9090',
                        sessionId: sessionId
                    }));
                } else if (error.response) {
                    console.error(`❌ [${sessionId}] C++ server error:`, error.response.status, error.response.data);
                    ws.send(JSON.stringify({
                        type: 'error',
                        message: `C++ server error: ${error.response.status}`,
                        sessionId: sessionId
                    }));
                } else {
                    console.error(`❌ [${sessionId}] Error connecting to C++ server:`, error.message);
                    ws.send(JSON.stringify({
                        type: 'error',
                        message: error.message,
                        sessionId: sessionId
                    }));
                }
            }
            
        } catch (error) {
            console.error(`❌ [${sessionId}] Error processing message:`, error);
        }
    });
    
    ws.on('close', async () => {
        console.log(`👋 [${sessionId}] Browser disconnected`);
        
        // Notify C++ server to clean up this session
        try {
            await axios.post(`${CPP_SERVER}/signaling`, {
                type: 'close',
                sessionId: sessionId
            }, {
                timeout: 1000,
                headers: { 'Content-Type': 'application/json' }
            });
            console.log(`🧹 [${sessionId}] Cleanup notification sent to C++`);
        } catch (error) {
            // Ignore cleanup errors
        }
    });
    
    ws.on('error', (error) => {
        console.error(`❌ [${sessionId}] WebSocket error:`, error);
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
