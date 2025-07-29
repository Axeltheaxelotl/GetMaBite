#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Simple test script to verify session_manager.cgi.py functionality
"""

import subprocess
import os
import sys

def test_session_manager():
    print("Testing session_manager.cgi.py functionality...")
    
    # Test 1: Create session
    print("\n1. Testing session creation...")
    env = os.environ.copy()
    env['REQUEST_METHOD'] = 'POST'
    env['CONTENT_TYPE'] = 'application/x-www-form-urlencoded'
    env['CONTENT_LENGTH'] = '13'
    
    try:
        process = subprocess.Popen(
            ['/usr/bin/python3', 'session_manager.cgi.py'],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            env=env,
            text=True
        )
        
        stdout, stderr = process.communicate(input='action=create')
        
        if process.returncode == 0:
            print("✅ Session creation test passed")
            print("Output:", stdout.split('\n\n', 1)[-1] if '\n\n' in stdout else stdout)
        else:
            print("❌ Session creation test failed")
            print("Error:", stderr)
    except Exception as e:
        print(f"❌ Error running session manager: {e}")

    # Test 2: Check session status
    print("\n2. Testing session status...")
    try:
        process = subprocess.Popen(
            ['/usr/bin/python3', 'session_manager.cgi.py'],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            env=env,
            text=True
        )
        
        stdout, stderr = process.communicate(input='action=status')
        
        if process.returncode == 0:
            print("✅ Session status test passed")
            print("Output:", stdout.split('\n\n', 1)[-1] if '\n\n' in stdout else stdout)
        else:
            print("❌ Session status test failed")
            print("Error:", stderr)
    except Exception as e:
        print(f"❌ Error checking session status: {e}")

    # Test 3: Check permissions
    print("\n3. Testing file permissions...")
    script_path = './session_manager.cgi.py'
    if os.path.exists(script_path):
        stat_info = os.stat(script_path)
        permissions = oct(stat_info.st_mode)[-3:]
        print(f"Session manager permissions: {permissions}")
        if permissions == '755':
            print("✅ Permissions are correct")
        else:
            print("❌ Permissions should be 755")
    else:
        print("❌ session_manager.cgi.py not found")

    # Test 4: Check /tmp directory permissions
    print("\n4. Testing /tmp directory access...")
    try:
        import tempfile
        with tempfile.NamedTemporaryFile(prefix='webserv_test_', delete=True) as tmp:
            print("✅ Can write to /tmp directory")
    except Exception as e:
        print(f"❌ Cannot write to /tmp: {e}")

if __name__ == "__main__":
    os.chdir('/media/smasse/GrosseByte/Projects/P6/GetMaBite/www/tests')
    test_session_manager()
