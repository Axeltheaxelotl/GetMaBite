#!/usr/bin/env python3
import os
import sys

def main():
    # Proper CGI headers (no HTTP response line)
    print("Content-Type: text/html")
    print()  # Empty line to separate headers from body
    
    # Simple HTML response
    html = """<!DOCTYPE html>
<html>
<head>
    <title>Simple CGI Test</title>
    <style>
        body {{ font-family: Arial, sans-serif; margin: 20px; }}
        .info {{ background-color: #f0f0f0; padding: 10px; margin: 10px 0; }}
    </style>
</head>
<body>
    <h1>Simple CGI Test</h1>
    <div class="info">
        <h2>Request Information</h2>
        <p><strong>REQUEST_METHOD:</strong> {method}</p>
        <p><strong>QUERY_STRING:</strong> {query}</p>
        <p><strong>SCRIPT_NAME:</strong> {script_name}</p>
        <p><strong>SERVER_NAME:</strong> {server_name}</p>
        <p><strong>SERVER_PORT:</strong> {server_port}</p>
    </div>
    
    <div class="info">
        <h2>POST Data</h2>
        <p><strong>CONTENT_LENGTH:</strong> {content_length}</p>
        <p><strong>CONTENT_TYPE:</strong> {content_type}</p>
        <p><strong>Body:</strong> {body}</p>
    </div>
</body>
</html>"""
    
    # Get CGI environment variables
    method = os.environ.get('REQUEST_METHOD', 'GET')
    query = os.environ.get('QUERY_STRING', '')
    script_name = os.environ.get('SCRIPT_NAME', '')
    server_name = os.environ.get('SERVER_NAME', '')
    server_port = os.environ.get('SERVER_PORT', '')
    content_length = os.environ.get('CONTENT_LENGTH', '0')
    content_type = os.environ.get('CONTENT_TYPE', '')
    
    # Read POST data if present
    body = ''
    if method == 'POST' and content_length and content_length.isdigit():
        try:
            body = sys.stdin.read(int(content_length))
        except:
            body = 'Error reading POST data'
    
    # Format and print the response
    print(html.format(
        method=method,
        query=query,
        script_name=script_name,
        server_name=server_name,
        server_port=server_port,
        content_length=content_length,
        content_type=content_type,
        body=body
    ))

if __name__ == "__main__":
    main()
