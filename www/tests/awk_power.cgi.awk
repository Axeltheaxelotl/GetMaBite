#!/usr/bin/awk -f

BEGIN {
    # Get environment variables
    method = ENVIRON["REQUEST_METHOD"]
    query = ENVIRON["QUERY_STRING"]
    server_name = ENVIRON["SERVER_NAME"]
    server_port = ENVIRON["SERVER_PORT"]
    user_agent = ENVIRON["HTTP_USER_AGENT"]
    script_name = ENVIRON["SCRIPT_NAME"]
    
    # Output HTTP headers
    print "Content-Type: text/html"
    print ""
    
    # HTML output
    print "<!DOCTYPE html>"
    print "<html>"
    print "<head>"
    print "    <title>AWK CGI Demo</title>"
    print "    <style>"
    print "        body {"
    print "            font-family: 'Monaco', 'Courier New', monospace;"
    print "            margin: 20px;"
    print "            background: linear-gradient(135deg, #ff9a56, #ff6b6b);"
    print "            color: #2c3e50;"
    print "        }"
    print "        .container {"
    print "            max-width: 800px;"
    print "            margin: 0 auto;"
    print "            background: rgba(255,255,255,0.95);"
    print "            padding: 30px;"
    print "            border-radius: 15px;"
    print "            box-shadow: 0 10px 30px rgba(0,0,0,0.2);"
    print "        }"
    print "        .awk-section {"
    print "            background: #f8f9fa;"
    print "            padding: 20px;"
    print "            margin: 20px 0;"
    print "            border-radius: 10px;"
    print "            border-left: 5px solid #ff6b6b;"
    print "        }"
    print "        .info-table {"
    print "            width: 100%;"
    print "            border-collapse: collapse;"
    print "            margin: 15px 0;"
    print "        }"
    print "        .info-table th, .info-table td {"
    print "            text-align: left;"
    print "            padding: 10px;"
    print "            border-bottom: 1px solid #dee2e6;"
    print "        }"
    print "        .info-table th {"
    print "            background-color: #e9ecef;"
    print "            font-weight: bold;"
    print "            color: #495057;"
    print "        }"
    print "        .code-block {"
    print "            background: #2d3748;"
    print "            color: #68d391;"
    print "            padding: 15px;"
    print "            border-radius: 8px;"
    print "            font-family: 'Monaco', monospace;"
    print "            overflow-x: auto;"
    print "            line-height: 1.5;"
    print "        }"
    print "        .awk-logo {"
    print "            text-align: center;"
    print "            font-size: 3.5em;"
    print "            margin: 20px 0;"
    print "            color: #ff6b6b;"
    print "        }"
    print "        .highlight { color: #ff6b6b; font-weight: bold; }"
    print "    </style>"
    print "</head>"
    print "<body>"
    print "    <div class=\"container\">"
    print "        <div class=\"awk-logo\">🔧 AWK CGI 🔧</div>"
    print "        <h1 style=\"text-align: center; color: #2c3e50;\">AWK Pattern-Action CGI Script</h1>"
    print "        <p style=\"text-align: center; font-style: italic;\">Text processing power meets web development</p>"
    
    print "        <div class=\"awk-section\">"
    print "            <h3>🌐 HTTP Request Information</h3>"
    print "            <table class=\"info-table\">"
    print "                <tr><th>Property</th><th>Value</th></tr>"
    print "                <tr><td>Request Method</td><td>" (method ? method : "Not available") "</td></tr>"
    print "                <tr><td>Query String</td><td>" (query ? query : "None") "</td></tr>"
    print "                <tr><td>Server Name</td><td>" (server_name ? server_name : "Unknown") "</td></tr>"
    print "                <tr><td>Server Port</td><td>" (server_port ? server_port : "Unknown") "</td></tr>"
    print "                <tr><td>Script Name</td><td>" (script_name ? script_name : "Unknown") "</td></tr>"
    if (user_agent) {
        print "                <tr><td>User Agent</td><td>" substr(user_agent, 1, 60) "...</td></tr>"
    } else {
        print "                <tr><td>User Agent</td><td>Unknown</td></tr>"
    }
    print "            </table>"
    print "        </div>"
    
    print "        <div class=\"awk-section\">"
    print "            <h3>⚙️ AWK Environment</h3>"
    print "            <table class=\"info-table\">"
    print "                <tr><th>Property</th><th>Value</th></tr>"
    
    # Get current time using system date command
    "date" | getline current_time
    close("date")
    print "                <tr><td>Current Time</td><td>" current_time "</td></tr>"
    
    # Get AWK version (this might not work on all systems)
    "awk --version 2>/dev/null | head -1" | getline awk_version
    close("awk --version 2>/dev/null | head -1")
    if (awk_version == "") {
        awk_version = "GNU AWK (version unavailable)"
    }
    print "                <tr><td>AWK Version</td><td>" awk_version "</td></tr>"
    
    # Get process ID
    "echo $$" | getline pid
    close("echo $$")
    print "                <tr><td>Process ID</td><td>" pid "</td></tr>"
    
    # Get system info
    "uname -s" | getline os_name
    close("uname -s")
    print "                <tr><td>Operating System</td><td>" os_name "</td></tr>"
    
    print "            </table>"
    print "        </div>"
    
    print "        <div class=\"awk-section\">"
    print "            <h3>🔍 AWK Pattern Matching Demo</h3>"
    print "            <p>Here's how AWK processes patterns and actions:</p>"
    print "            <div class=\"code-block\">"
    print "#!/usr/bin/awk -f<br/><br/>"
    print "# Pattern-Action structure<br/>"
    print "BEGIN {<br/>"
    print "&nbsp;&nbsp;print \"Content-Type: text/html\"<br/>"
    print "&nbsp;&nbsp;print \"\"<br/>"
    print "&nbsp;&nbsp;# Process environment variables<br/>"
    print "&nbsp;&nbsp;method = ENVIRON[\"REQUEST_METHOD\"]<br/>"
    print "}<br/><br/>"
    print "# Pattern matching for different request types<br/>"
    print "/GET/ { print \"Processing GET request\" }<br/>"
    print "/POST/ { print \"Processing POST request\" }<br/><br/>"
    print "# Field processing (if reading input)<br/>"
    print "NF > 0 {<br/>"
    print "&nbsp;&nbsp;for (i = 1; i <= NF; i++) {<br/>"
    print "&nbsp;&nbsp;&nbsp;&nbsp;print \"Field\", i \":\", $i<br/>"
    print "&nbsp;&nbsp;}<br/>"
    print "}<br/><br/>"
    print "END {<br/>"
    print "&nbsp;&nbsp;print \"AWK processing complete\"<br/>"
    print "}"
    print "            </div>"
    print "        </div>"
    
    print "        <div class=\"awk-section\">"
    print "            <h3>✨ AWK Features Demonstration</h3>"
    print "            <ul>"
    print "                <li><span class=\"highlight\">Pattern Matching:</span> Built-in regex and pattern recognition</li>"
    print "                <li><span class=\"highlight\">Field Processing:</span> Automatic field splitting ($1, $2, etc.)</li>"
    print "                <li><span class=\"highlight\">Built-in Variables:</span> NR, NF, FS, RS, and more</li>"
    print "                <li><span class=\"highlight\">String Functions:</span> substr(), gsub(), match(), split()</li>"
    print "                <li><span class=\"highlight\">Math Functions:</span> sin(), cos(), int(), rand()</li>"
    print "                <li><span class=\"highlight\">System Integration:</span> Environment variables and shell commands</li>"
    print "            </ul>"
    print "        </div>"
    
    print "        <p style=\"text-align: center; font-style: italic; color: #6c757d; margin-top: 30px;\">"
    print "            🔧 Powered by AWK • Pattern-Action Programming • Process ID: " pid " 🔧"
    print "        </p>"
    print "    </div>"
    print "</body>"
    print "</html>"
}
