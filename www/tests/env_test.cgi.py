#!/usr/bin/env python3

import os
import sys

print("Content-Type: text/html")
print()

# Test script to verify all CGI environment variables
required_vars = [
    'REQUEST_METHOD',
    'SCRIPT_NAME', 
    'QUERY_STRING',
    'SERVER_PROTOCOL',
    'GATEWAY_INTERFACE',
    'REQUEST_URI',
    'PATH_INFO',
    'SERVER_NAME',
    'SERVER_PORT',
    'REMOTE_ADDR',
    'REMOTE_HOST',
    'SERVER_SOFTWARE',
    'CONTENT_TYPE',
    'CONTENT_LENGTH',
    'DOCUMENT_ROOT'
]

print("<html><head><title>CGI Environment Test</title></head><body>")
print("<h1>CGI Environment Variables Test</h1>")

print("<h2>Required CGI Variables Status:</h2>")
print("<table border='1' cellpadding='5'>")
print("<tr><th>Variable</th><th>Value</th><th>Status</th></tr>")

for var in required_vars:
    value = os.environ.get(var, 'NOT SET')
    status = "✅ SET" if var in os.environ else "❌ MISSING"
    color = "green" if var in os.environ else "red"
    print(f"<tr><td><strong>{var}</strong></td><td style='color: {color}'>{value}</td><td style='color: {color}'>{status}</td></tr>")

print("</table>")

print("<h2>All Environment Variables:</h2>")
print("<table border='1' cellpadding='5'>")
print("<tr><th>Variable</th><th>Value</th></tr>")

for key, value in sorted(os.environ.items()):
    print(f"<tr><td><strong>{key}</strong></td><td>{value}</td></tr>")

print("</table>")
print("</body></html>")
