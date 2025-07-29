#!/usr/bin/php
<?php
// PHP CGI script that works with CLI PHP

// Output HTTP headers for CGI
echo "Content-Type: text/html; charset=UTF-8\r\n\r\n";

// Function to safely get environment variable
function safeGetEnv($key, $default = 'Not available') {
    return isset($_SERVER[$key]) ? htmlspecialchars($_SERVER[$key]) : $default;
}

// Get POST data if available
$postData = '';
if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    $postData = file_get_contents('php://input');
}

?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>PHP CGI Demo</title>
    <style>
        body { 
            font-family: 'Arial', sans-serif; 
            margin: 20px; 
            background: linear-gradient(45deg, #FF6B6B, #4ECDC4, #45B7D1, #96CEB4);
            background-size: 400% 400%;
            animation: gradientShift 8s ease infinite;
            color: #333;
        }
        
        @keyframes gradientShift {
            0% { background-position: 0% 50%; }
            50% { background-position: 100% 50%; }
            100% { background-position: 0% 50%; }
        }
        
        .container { 
            max-width: 800px; 
            margin: 0 auto; 
            background: rgba(255,255,255,0.95); 
            padding: 30px; 
            border-radius: 20px;
            box-shadow: 0 10px 30px rgba(0,0,0,0.3);
        }
        
        .php-section { 
            background: #f8f9fa; 
            padding: 20px; 
            margin: 20px 0; 
            border-radius: 10px; 
            border-left: 5px solid #007bff;
        }
        
        .info-grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 15px;
            margin: 15px 0;
        }
        
        .info-item {
            background: #e9ecef;
            padding: 10px;
            border-radius: 5px;
        }
        
        .label { font-weight: bold; color: #495057; }
        .value { color: #6c757d; word-break: break-all; }
        
        .php-logo {
            text-align: center;
            font-size: 3em;
            margin: 20px 0;
        }
        
        .code-sample {
            background: #2d3748;
            color: #e2e8f0;
            padding: 15px;
            border-radius: 8px;
            font-family: 'Courier New', monospace;
            overflow-x: auto;
            margin: 15px 0;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="php-logo">🐘 PHP CGI</div>
        <h1 style="text-align: center; color: #343a40;">PHP Server-Side Processing</h1>
        
        <div class="php-section">
            <h3>🌐 HTTP Request Information</h3>
            <div class="info-grid">
                <div class="info-item">
                    <div class="label">Request Method:</div>
                    <div class="value"><?= safeGetEnv('REQUEST_METHOD') ?></div>
                </div>
                <div class="info-item">
                    <div class="label">Query String:</div>
                    <div class="value"><?= safeGetEnv('QUERY_STRING') ?: 'None' ?></div>
                </div>
                <div class="info-item">
                    <div class="label">Content Type:</div>
                    <div class="value"><?= safeGetEnv('CONTENT_TYPE') ?></div>
                </div>
                <div class="info-item">
                    <div class="label">Content Length:</div>
                    <div class="value"><?= safeGetEnv('CONTENT_LENGTH') ?: '0' ?> bytes</div>
                </div>
            </div>
        </div>
        
        <div class="php-section">
            <h3>🖥️ Server Environment</h3>
            <div class="info-grid">
                <div class="info-item">
                    <div class="label">Server Name:</div>
                    <div class="value"><?= safeGetEnv('SERVER_NAME') ?></div>
                </div>
                <div class="info-item">
                    <div class="label">Server Port:</div>
                    <div class="value"><?= safeGetEnv('SERVER_PORT') ?></div>
                </div>
                <div class="info-item">
                    <div class="label">Document Root:</div>
                    <div class="value"><?= safeGetEnv('DOCUMENT_ROOT') ?></div>
                </div>
                <div class="info-item">
                    <div class="label">Script Name:</div>
                    <div class="value"><?= safeGetEnv('SCRIPT_NAME') ?></div>
                </div>
            </div>
        </div>
        
        <div class="php-section">
            <h3>🐘 PHP Environment</h3>
            <div class="info-grid">
                <div class="info-item">
                    <div class="label">PHP Version:</div>
                    <div class="value"><?= phpversion() ?></div>
                </div>
                <div class="info-item">
                    <div class="label">PHP SAPI:</div>
                    <div class="value"><?= php_sapi_name() ?></div>
                </div>
                <div class="info-item">
                    <div class="label">Memory Limit:</div>
                    <div class="value"><?= ini_get('memory_limit') ?></div>
                </div>
                <div class="info-item">
                    <div class="label">Max Execution Time:</div>
                    <div class="value"><?= ini_get('max_execution_time') ?> seconds</div>
                </div>
            </div>
        </div>
        
        <div class="php-section">
            <h3>⏰ Runtime Details</h3>
            <div class="info-grid">
                <div class="info-item">
                    <div class="label">Current Time:</div>
                    <div class="value"><?= date('Y-m-d H:i:s T') ?></div>
                </div>
                <div class="info-item">
                    <div class="label">Process ID:</div>
                    <div class="value"><?= getmypid() ?></div>
                </div>
                <div class="info-item">
                    <div class="label">User Agent:</div>
                    <div class="value"><?= substr(safeGetEnv('HTTP_USER_AGENT'), 0, 50) ?>...</div>
                </div>
                <div class="info-item">
                    <div class="label">Remote Address:</div>
                    <div class="value"><?= safeGetEnv('REMOTE_ADDR') ?></div>
                </div>
            </div>
        </div>
        
        <?php if (!empty($postData)): ?>
        <div class="php-section">
            <h3>📨 POST Data Received</h3>
            <div class="code-sample">
                <?= htmlspecialchars($postData) ?>
            </div>
        </div>
        <?php endif; ?>
        
        <div class="php-section">
            <h3>💻 Sample PHP Code</h3>
            <div class="code-sample">
&lt;?php<br/>
// PHP CGI example with superglobals<br/>
$method = $_SERVER['REQUEST_METHOD'];<br/>
$query = $_GET;<br/>
$post = $_POST;<br/>
<br/>
echo "Request processed at: " . date('Y-m-d H:i:s');<br/>
echo "Server: {$_SERVER['SERVER_NAME']}";<br/>
<br/>
// Process form data<br/>
if ($method === 'POST') {<br/>
&nbsp;&nbsp;&nbsp;&nbsp;foreach ($_POST as $key =&gt; $value) {<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;echo "Field: $key = $value\n";<br/>
&nbsp;&nbsp;&nbsp;&nbsp;}<br/>
}<br/>
?&gt;
            </div>
        </div>
        
        <p style="text-align: center; font-style: italic; color: #6c757d;">
            🚀 Generated by PHP <?= phpversion() ?> • Server-side magic in action!
        </p>
    </div>
</body>
</html>
