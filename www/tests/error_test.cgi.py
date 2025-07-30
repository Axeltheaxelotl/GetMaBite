#!/usr/bin/env python3

# Test CGI script that intentionally fails
import os
import nonexistent_module  # This will cause ModuleNotFoundError

print("Content-Type: text/html")
print()
print("<html><body><h1>This should never be reached</h1></body></html>")
