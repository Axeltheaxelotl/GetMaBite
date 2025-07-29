#!/bin/bash

# Simple Shell CGI script
echo "Content-Type: text/html"
echo ""
echo "<html><head><title>Shell CGI</title></head><body>"
echo "<h1>Hello from Shell CGI!</h1>"
echo "<p>Current time: $(date)</p>"
echo "<p>Server: $SERVER_NAME:$SERVER_PORT</p>"
echo "<p>Script: $SCRIPT_NAME</p>"
echo "<p>Method: $REQUEST_METHOD</p>"
if [ "$REQUEST_METHOD" = "GET" ]; then
    echo "<p>Query String: $QUERY_STRING</p>"
fi
echo "<p>User Agent: $HTTP_USER_AGENT</p>"
echo "</body></html>"
