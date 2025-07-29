#!/usr/bin/env node

// Node.js CGI script
const os = require('os');
const process = require('process');

// Get environment variables
const method = process.env.REQUEST_METHOD || 'GET';
const query = process.env.QUERY_STRING || '';
const contentType = process.env.CONTENT_TYPE || '';
const contentLength = process.env.CONTENT_LENGTH || '0';
const serverName = process.env.SERVER_NAME || 'unknown';
const serverPort = process.env.SERVER_PORT || 'unknown';
const userAgent = process.env.HTTP_USER_AGENT || 'unknown';
const remoteAddr = process.env.REMOTE_ADDR || 'unknown';

// Read POST data if available
let postData = '';
if (method === 'POST' && parseInt(contentLength) > 0) {
    process.stdin.setEncoding('utf8');
    process.stdin.on('data', chunk => {
        postData += chunk;
    });
    process.stdin.on('end', () => {
        outputResponse();
    });
} else {
    outputResponse();
}

function outputResponse() {
    // Output HTTP headers
    console.log('Content-Type: text/html');
    console.log('');

    // HTML output
    console.log(`<!DOCTYPE html>
<html>
<head>
    <title>Node.js CGI Demo</title>
    <style>
        body { 
            font-family: 'SF Pro Display', -apple-system, BlinkMacSystemFont, sans-serif; 
            margin: 20px; 
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            color: white;
        }
        .container { 
            max-width: 1000px; 
            margin: 0 auto; 
            background: rgba(255,255,255,0.1); 
            padding: 40px; 
            border-radius: 20px;
            backdrop-filter: blur(20px);
            border: 1px solid rgba(255,255,255,0.2);
            box-shadow: 0 15px 35px rgba(0,0,0,0.1);
        }
        .node-section { 
            background: rgba(255,255,255,0.05); 
            padding: 25px; 
            margin: 25px 0; 
            border-radius: 15px; 
            border-left: 5px solid #68d391;
        }
        .info-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
            gap: 20px;
            margin: 20px 0;
        }
        .info-card {
            background: rgba(255,255,255,0.1);
            padding: 15px;
            border-radius: 10px;
            border: 1px solid rgba(255,255,255,0.1);
        }
        .label { 
            font-weight: 600; 
            color: #ffd700; 
            display: block;
            margin-bottom: 5px;
        }
        .value { 
            color: #e2e8f0; 
            word-break: break-all; 
            font-family: 'SF Mono', Monaco, monospace;
        }
        .node-logo {
            text-align: center;
            font-size: 4em;
            margin: 30px 0;
            text-shadow: 0 0 20px rgba(104, 211, 145, 0.5);
        }
        .code-block {
            background: #1a202c;
            color: #68d391;
            padding: 20px;
            border-radius: 12px;
            font-family: 'SF Mono', Monaco, monospace;
            overflow-x: auto;
            border: 1px solid rgba(104, 211, 145, 0.2);
            line-height: 1.6;
        }
        .stats-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
            gap: 15px;
            margin: 20px 0;
        }
        .stat-item {
            text-align: center;
            background: rgba(255,255,255,0.1);
            padding: 15px;
            border-radius: 10px;
        }
        .stat-value {
            font-size: 1.5em;
            font-weight: bold;
            color: #68d391;
        }
        .stat-label {
            font-size: 0.9em;
            color: #cbd5e0;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="node-logo">🚀 Node.js CGI</div>
        <h1 style="text-align: center; margin-bottom: 10px;">JavaScript Server-Side Execution</h1>
        <p style="text-align: center; font-style: italic; opacity: 0.8;">Powered by V8 Engine • Event-driven • Non-blocking I/O</p>
        
        <div class="node-section">
            <h3>🌐 HTTP Request Details</h3>
            <div class="info-grid">
                <div class="info-card">
                    <span class="label">Request Method:</span>
                    <span class="value">${method}</span>
                </div>
                <div class="info-card">
                    <span class="label">Query String:</span>
                    <span class="value">${query || 'None'}</span>
                </div>
                <div class="info-card">
                    <span class="label">Content Type:</span>
                    <span class="value">${contentType || 'Not specified'}</span>
                </div>
                <div class="info-card">
                    <span class="label">Content Length:</span>
                    <span class="value">${contentLength} bytes</span>
                </div>
                <div class="info-card">
                    <span class="label">User Agent:</span>
                    <span class="value">${userAgent.substring(0, 50)}...</span>
                </div>
                <div class="info-card">
                    <span class="label">Remote Address:</span>
                    <span class="value">${remoteAddr}</span>
                </div>
            </div>
        </div>
        
        <div class="node-section">
            <h3>🖥️ Server Environment</h3>
            <div class="info-grid">
                <div class="info-card">
                    <span class="label">Server Name:</span>
                    <span class="value">${serverName}</span>
                </div>
                <div class="info-card">
                    <span class="label">Server Port:</span>
                    <span class="value">${serverPort}</span>
                </div>
                <div class="info-card">
                    <span class="label">Script Name:</span>
                    <span class="value">${process.env.SCRIPT_NAME || 'Unknown'}</span>
                </div>
                <div class="info-card">
                    <span class="label">Document Root:</span>
                    <span class="value">${process.env.DOCUMENT_ROOT || 'Unknown'}</span>
                </div>
            </div>
        </div>
        
        <div class="node-section">
            <h3>⚡ Node.js Runtime Information</h3>
            <div class="stats-grid">
                <div class="stat-item">
                    <div class="stat-value">${process.version}</div>
                    <div class="stat-label">Node Version</div>
                </div>
                <div class="stat-item">
                    <div class="stat-value">${process.versions.v8}</div>
                    <div class="stat-label">V8 Engine</div>
                </div>
                <div class="stat-item">
                    <div class="stat-value">${process.pid}</div>
                    <div class="stat-label">Process ID</div>
                </div>
                <div class="stat-item">
                    <div class="stat-value">${Math.round(process.memoryUsage().rss / 1024 / 1024)}MB</div>
                    <div class="stat-label">Memory Usage</div>
                </div>
                <div class="stat-item">
                    <div class="stat-value">${process.platform}</div>
                    <div class="stat-label">Platform</div>
                </div>
                <div class="stat-item">
                    <div class="stat-value">${process.arch}</div>
                    <div class="stat-label">Architecture</div>
                </div>
            </div>
        </div>
        
        <div class="node-section">
            <h3>🖥️ System Information</h3>
            <div class="info-grid">
                <div class="info-card">
                    <span class="label">Hostname:</span>
                    <span class="value">${os.hostname()}</span>
                </div>
                <div class="info-card">
                    <span class="label">Operating System:</span>
                    <span class="value">${os.type()} ${os.release()}</span>
                </div>
                <div class="info-card">
                    <span class="label">CPU Count:</span>
                    <span class="value">${os.cpus().length} cores</span>
                </div>
                <div class="info-card">
                    <span class="label">Total Memory:</span>
                    <span class="value">${Math.round(os.totalmem() / 1024 / 1024 / 1024)}GB</span>
                </div>
                <div class="info-card">
                    <span class="label">Free Memory:</span>
                    <span class="value">${Math.round(os.freemem() / 1024 / 1024 / 1024)}GB</span>
                </div>
                <div class="info-card">
                    <span class="label">Uptime:</span>
                    <span class="value">${Math.round(os.uptime() / 3600)}h</span>
                </div>
            </div>
        </div>
        
        ${postData ? `
        <div class="node-section">
            <h3>📨 POST Data Received</h3>
            <div class="code-block">
                ${postData.replace(/</g, '&lt;').replace(/>/g, '&gt;')}
            </div>
        </div>
        ` : ''}
        
        <div class="node-section">
            <h3>💻 Node.js Code Sample</h3>
            <div class="code-block">
const express = require('express');<br/>
const app = express();<br/><br/>

// Middleware for parsing request data<br/>
app.use(express.urlencoded({ extended: true }));<br/>
app.use(express.json());<br/><br/>

// CGI-style route handler<br/>
app.all('/cgi-bin/script', (req, res) => {<br/>
&nbsp;&nbsp;const method = req.method;<br/>
&nbsp;&nbsp;const query = req.query;<br/>
&nbsp;&nbsp;const body = req.body;<br/><br/>
  
&nbsp;&nbsp;res.setHeader('Content-Type', 'text/html');<br/>
&nbsp;&nbsp;res.send(\`&lt;h1&gt;Request processed by Node.js&lt;/h1&gt;<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&lt;p&gt;Method: \${method}&lt;/p&gt;<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&lt;p&gt;Time: \${new Date().toISOString()}&lt;/p&gt;\`);<br/>
});<br/><br/>

app.listen(3000, () => {<br/>
&nbsp;&nbsp;console.log('Server running on port 3000');<br/>
});
            </div>
        </div>
        
        <p style="text-align: center; font-style: italic; color: #cbd5e0; margin-top: 40px;">
            🚀 Executed by Node.js ${process.version} • V8 Engine ${process.versions.v8} • PID: ${process.pid}
        </p>
    </div>
</body>
</html>`);
}
