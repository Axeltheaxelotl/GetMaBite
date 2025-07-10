#!/usr/bin/env python3
import requests
import time
import subprocess
import signal
import os

def test_cgi():
    # Start webserv in background
    webserv_process = subprocess.Popen(
        ["./webserv", "config.conf"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        cwd="/home/smasse/Projects/P6/GetMaBite"
    )
    
    # Wait for server to start
    time.sleep(2)
    
    try:
        # Test GET request to CGI
        response = requests.get("http://localhost:8081/aaa.cgi.py", timeout=10)
        print("GET CGI Response:")
        print(f"Status: {response.status_code}")
        print(f"Content: {response.text[:200]}...")
        
        # Test POST request to CGI
        post_data = "test_data=hello&name=world"
        response = requests.post(
            "http://localhost:8081/aaa.cgi.py",
            data=post_data,
            headers={"Content-Type": "application/x-www-form-urlencoded"},
            timeout=10
        )
        print("\nPOST CGI Response:")
        print(f"Status: {response.status_code}")
        print(f"Content: {response.text[:200]}...")
        
    except Exception as e:
        print(f"Error testing CGI: {e}")
    
    finally:
        # Clean up
        webserv_process.terminate()
        webserv_process.wait()

if __name__ == "__main__":
    test_cgi()
