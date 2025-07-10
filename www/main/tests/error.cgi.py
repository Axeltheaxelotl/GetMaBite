#!/usr/bin/env python3
import sys
import os

# This CGI script intentionally has an error
print("Content-Type: text/html")
print()

# This will cause an error
print("Current time:", undefined_variable)
