#!/usr/bin/env python3

# Minimal Python CGI with JSON output
import json
import os
import sys

print("Content-Type: application/json")
print()

data = {
    "message": "Hello from Python CGI!",
    "method": os.environ.get('REQUEST_METHOD', 'GET'),
    "query": os.environ.get('QUERY_STRING', ''),
    "time": __import__('time').strftime('%Y-%m-%d %H:%M:%S'),
    "environment": {
        "server": os.environ.get('SERVER_NAME', 'unknown'),
        "port": os.environ.get('SERVER_PORT', 'unknown'),
        "script": os.environ.get('SCRIPT_NAME', 'unknown'),
        "user_agent": os.environ.get('HTTP_USER_AGENT', 'unknown')[:50] + "..."
    }
}

if os.environ.get('REQUEST_METHOD') == 'POST':
    content_length = os.environ.get('CONTENT_LENGTH')
    if content_length and content_length.isdigit():
        try:
            post_data = sys.stdin.read(int(content_length))
            data["post_data"] = post_data
        except:
            data["post_data"] = "Error reading POST data"

print(json.dumps(data, indent=2))
