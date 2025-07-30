#!/usr/bin/env python3

import os
import sys
import cgi
import urllib.parse
import time

# Debug script to test CGI parsing
print("Content-Type: text/plain")
print()

print("=== CGI DEBUG INFORMATION ===")
print(f"REQUEST_METHOD: {os.environ.get('REQUEST_METHOD', 'Not set')}")
print(f"CONTENT_TYPE: {os.environ.get('CONTENT_TYPE', 'Not set')}")
print(f"CONTENT_LENGTH: {os.environ.get('CONTENT_LENGTH', 'Not set')}")
print(f"QUERY_STRING: {os.environ.get('QUERY_STRING', 'Not set')}")

print("\n=== STDIN DATA ===")
try:
    stdin_data = sys.stdin.read()
    print(f"Raw stdin: '{stdin_data}'")
    if stdin_data:
        print(f"Parsed pairs:")
        for pair in stdin_data.strip().split('&'):
            if '=' in pair:
                key, value = pair.split('=', 1)
                print(f"  {key} = {urllib.parse.unquote_plus(value)}")
except Exception as e:
    print(f"Error reading stdin: {e}")

print("\n=== CGI FIELDSTORAGE ===")
try:
    # Reset stdin if possible
    form = cgi.FieldStorage()
    time.sleep(10)  # Allow time for stdin to be read
    print(f"FieldStorage keys: {list(form.keys())}")
    for key in form.keys():
        print(f"  {key} = {form.getvalue(key)}")
except Exception as e:
    print(f"Error with FieldStorage: {e}")
