#!/usr/bin/env python3
import sys
import os

print("Content-Type: text/html")
print()

print("<html><body>")
print("<h1>CGI Script with stderr</h1>")
print("<p>This should be captured as stdout</p>")

# This will go to stderr and should be captured
print("<p>Error message</p>", file=sys.stderr)
print("</body></html>")
