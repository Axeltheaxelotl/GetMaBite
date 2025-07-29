#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>

int main() {
    char *method = getenv("REQUEST_METHOD");
    char *query = getenv("QUERY_STRING");
    char *content_type = getenv("CONTENT_TYPE");
    char *content_length_str = getenv("CONTENT_LENGTH");
    char *server_name = getenv("SERVER_NAME");
    char *server_port = getenv("SERVER_PORT");
    char *user_agent = getenv("HTTP_USER_AGENT");
    char *remote_addr = getenv("REMOTE_ADDR");
    
    time_t current_time;
    struct tm *time_info;
    char time_buffer[100];
    
    time(&current_time);
    time_info = localtime(&current_time);
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", time_info);
    
    // Output HTTP headers
    printf("Content-Type: text/html\r\n\r\n");
    
    // HTML output
    printf("<!DOCTYPE html>\n");
    printf("<html>\n");
    printf("<head>\n");
    printf("    <title>C CGI Demo</title>\n");
    printf("    <style>\n");
    printf("        body { \n");
    printf("            font-family: 'Courier New', monospace; \n");
    printf("            margin: 20px; \n");
    printf("            background: linear-gradient(45deg, #2c3e50, #34495e, #2c3e50);\n");
    printf("            color: #ecf0f1;\n");
    printf("            background-size: 400%% 400%%;\n");
    printf("            animation: backgroundShift 10s ease infinite;\n");
    printf("        }\n");
    printf("        @keyframes backgroundShift {\n");
    printf("            0%%, 100%% { background-position: 0%% 50%%; }\n");
    printf("            50%% { background-position: 100%% 50%%; }\n");
    printf("        }\n");
    printf("        .container { \n");
    printf("            max-width: 900px; \n");
    printf("            margin: 0 auto; \n");
    printf("            background: rgba(44, 62, 80, 0.9); \n");
    printf("            padding: 30px; \n");
    printf("            border-radius: 15px;\n");
    printf("            border: 2px solid #3498db;\n");
    printf("            box-shadow: 0 0 20px rgba(52, 152, 219, 0.3);\n");
    printf("        }\n");
    printf("        .c-section { \n");
    printf("            background: rgba(52, 73, 94, 0.8); \n");
    printf("            padding: 20px; \n");
    printf("            margin: 20px 0; \n");
    printf("            border-radius: 10px; \n");
    printf("            border-left: 5px solid #e74c3c;\n");
    printf("        }\n");
    printf("        .info-table {\n");
    printf("            width: 100%%;\n");
    printf("            border-collapse: collapse;\n");
    printf("            margin: 15px 0;\n");
    printf("        }\n");
    printf("        .info-table th, .info-table td {\n");
    printf("            text-align: left;\n");
    printf("            padding: 8px 12px;\n");
    printf("            border-bottom: 1px solid #34495e;\n");
    printf("        }\n");
    printf("        .info-table th {\n");
    printf("            background-color: #2c3e50;\n");
    printf("            color: #3498db;\n");
    printf("            font-weight: bold;\n");
    printf("        }\n");
    printf("        .code-block {\n");
    printf("            background: #1a1a1a;\n");
    printf("            color: #00ff00;\n");
    printf("            padding: 15px;\n");
    printf("            border-radius: 8px;\n");
    printf("            font-family: 'Courier New', monospace;\n");
    printf("            overflow-x: auto;\n");
    printf("            border: 1px solid #333;\n");
    printf("        }\n");
    printf("        .highlight { color: #f39c12; font-weight: bold; }\n");
    printf("        .c-logo { text-align: center; font-size: 4em; margin: 20px 0; }\n");
    printf("    </style>\n");
    printf("</head>\n");
    printf("<body>\n");
    printf("    <div class=\"container\">\n");
    printf("        <div class=\"c-logo\">⚡ C CGI ⚡</div>\n");
    printf("        <h1 style=\"text-align: center; color: #3498db;\">Low-Level C CGI Script</h1>\n");
    printf("        <p style=\"text-align: center; font-style: italic;\">Compiled C code serving web content at maximum performance</p>\n");
    
    printf("        <div class=\"c-section\">\n");
    printf("            <h3>🌐 HTTP Request Information</h3>\n");
    printf("            <table class=\"info-table\">\n");
    printf("                <tr><th>Property</th><th>Value</th></tr>\n");
    printf("                <tr><td>Request Method</td><td>%s</td></tr>\n", method ? method : "Not available");
    printf("                <tr><td>Query String</td><td>%s</td></tr>\n", query ? query : "None");
    printf("                <tr><td>Content Type</td><td>%s</td></tr>\n", content_type ? content_type : "Not specified");
    printf("                <tr><td>Content Length</td><td>%s bytes</td></tr>\n", content_length_str ? content_length_str : "0");
    printf("                <tr><td>User Agent</td><td>%.60s%s</td></tr>\n", 
           user_agent ? user_agent : "Unknown", 
           (user_agent && strlen(user_agent) > 60) ? "..." : "");
    printf("                <tr><td>Remote Address</td><td>%s</td></tr>\n", remote_addr ? remote_addr : "Unknown");
    printf("            </table>\n");
    printf("        </div>\n");
    
    printf("        <div class=\"c-section\">\n");
    printf("            <h3>🖥️ Server Environment</h3>\n");
    printf("            <table class=\"info-table\">\n");
    printf("                <tr><th>Property</th><th>Value</th></tr>\n");
    printf("                <tr><td>Server Name</td><td>%s</td></tr>\n", server_name ? server_name : "Unknown");
    printf("                <tr><td>Server Port</td><td>%s</td></tr>\n", server_port ? server_port : "Unknown");
    printf("                <tr><td>Gateway Interface</td><td>%s</td></tr>\n", getenv("GATEWAY_INTERFACE") ? getenv("GATEWAY_INTERFACE") : "CGI/1.1");
    printf("                <tr><td>Server Software</td><td>%s</td></tr>\n", getenv("SERVER_SOFTWARE") ? getenv("SERVER_SOFTWARE") : "Unknown");
    printf("            </table>\n");
    printf("        </div>\n");
    
    printf("        <div class=\"c-section\">\n");
    printf("            <h3>⚙️ Runtime Information</h3>\n");
    printf("            <table class=\"info-table\">\n");
    printf("                <tr><th>Property</th><th>Value</th></tr>\n");
    printf("                <tr><td>Current Time</td><td>%s</td></tr>\n", time_buffer);
    printf("                <tr><td>Process ID</td><td>%d</td></tr>\n", getpid());
    printf("                <tr><td>Parent Process ID</td><td>%d</td></tr>\n", getppid());
    printf("                <tr><td>User ID</td><td>%d</td></tr>\n", getuid());
    printf("            </table>\n");
    printf("        </div>\n");
    
    printf("        <div class=\"c-section\">\n");
    printf("            <h3>💻 C Code Sample</h3>\n");
    printf("            <div class=\"code-block\">\n");
    printf("#include &lt;stdio.h&gt;<br/>\n");
    printf("#include &lt;stdlib.h&gt;<br/>\n");
    printf("#include &lt;string.h&gt;<br/><br/>\n");
    printf("int main() {<br/>\n");
    printf("&nbsp;&nbsp;&nbsp;&nbsp;char *method = getenv(\"REQUEST_METHOD\");<br/>\n");
    printf("&nbsp;&nbsp;&nbsp;&nbsp;char *query = getenv(\"QUERY_STRING\");<br/><br/>\n");
    printf("&nbsp;&nbsp;&nbsp;&nbsp;printf(\"Content-Type: text/html\\r\\n\\r\\n\");<br/>\n");
    printf("&nbsp;&nbsp;&nbsp;&nbsp;printf(\"&lt;h1&gt;Hello from C CGI!&lt;/h1&gt;\");<br/><br/>\n");
    printf("&nbsp;&nbsp;&nbsp;&nbsp;if (method && strcmp(method, \"GET\") == 0) {<br/>\n");
    printf("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;printf(\"&lt;p&gt;GET request processed&lt;/p&gt;\");<br/>\n");
    printf("&nbsp;&nbsp;&nbsp;&nbsp;}<br/><br/>\n");
    printf("&nbsp;&nbsp;&nbsp;&nbsp;return 0;<br/>\n");
    printf("}<br/>\n");
    printf("            </div>\n");
    printf("        </div>\n");
    
    printf("        <div class=\"c-section\">\n");
    printf("            <h3>🚀 Performance Notes</h3>\n");
    printf("            <ul>\n");
    printf("                <li><span class=\"highlight\">Compiled Binary:</span> Maximum execution speed</li>\n");
    printf("                <li><span class=\"highlight\">Memory Efficient:</span> Minimal memory footprint</li>\n");
    printf("                <li><span class=\"highlight\">Direct System Access:</span> Low-level environment variable access</li>\n");
    printf("                <li><span class=\"highlight\">No Runtime Dependencies:</span> Self-contained executable</li>\n");
    printf("            </ul>\n");
    printf("        </div>\n");
    
    printf("        <p style=\"text-align: center; font-style: italic; color: #95a5a6; margin-top: 30px;\">\n");
    printf("            ⚡ Powered by pure C • Compiled for maximum performance • Process ID: %d ⚡\n", getpid());
    printf("        </p>\n");
    printf("    </div>\n");
    printf("</body>\n");
    printf("</html>\n");
    
    return 0;
}
