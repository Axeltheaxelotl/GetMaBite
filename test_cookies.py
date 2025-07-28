#!/usr/bin/env python3
"""
Test script for cookie and session functionality in webserv
"""

import requests
import sys
import time

class Colors:
    GREEN = '\033[92m'
    RED = '\033[91m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    RESET = '\033[0m'

def print_success(msg):
    print(f"{Colors.GREEN}✓ {msg}{Colors.RESET}")

def print_error(msg):
    print(f"{Colors.RED}✗ {msg}{Colors.RESET}")

def print_info(msg):
    print(f"{Colors.BLUE}ℹ {msg}{Colors.RESET}")

def print_warning(msg):
    print(f"{Colors.YELLOW}⚠ {msg}{Colors.RESET}")

def test_session_creation(base_url):
    """Test that a new session is created when no cookie is sent"""
    print_info("Testing session creation...")
    
    try:
        response = requests.get(f"{base_url}/", timeout=5)
        
        if response.status_code == 200:
            print_success(f"Request successful (status: {response.status_code})")
            
            # Check if Set-Cookie header is present
            if 'Set-Cookie' in response.headers:
                cookie_header = response.headers['Set-Cookie']
                print_success(f"Set-Cookie header found: {cookie_header}")
                
                # Check if it's a session cookie
                if 'SESSIONID=' in cookie_header:
                    print_success("Session cookie (SESSIONID) created")
                    
                    # Extract session ID
                    session_id = cookie_header.split('SESSIONID=')[1].split(';')[0]
                    print_info(f"Session ID: {session_id}")
                    return session_id
                else:
                    print_error("SESSIONID cookie not found in Set-Cookie header")
            else:
                print_error("No Set-Cookie header found")
        else:
            print_error(f"Request failed with status: {response.status_code}")
            
    except requests.exceptions.RequestException as e:
        print_error(f"Request failed: {e}")
    
    return None

def test_session_persistence(base_url, session_id):
    """Test that session persists when cookie is sent back"""
    print_info("Testing session persistence...")
    
    if not session_id:
        print_error("No session ID to test with")
        return False
    
    try:
        # Send request with session cookie
        cookies = {'SESSIONID': session_id}
        response = requests.get(f"{base_url}/", cookies=cookies, timeout=5)
        
        if response.status_code == 200:
            print_success(f"Request with cookie successful (status: {response.status_code})")
            
            # Check if the same session ID is maintained
            if 'Set-Cookie' in response.headers:
                cookie_header = response.headers['Set-Cookie']
                if session_id in cookie_header:
                    print_success("Session ID maintained")
                    return True
                else:
                    print_warning("Different session ID returned (possible session renewal)")
                    return True
            else:
                print_info("No Set-Cookie header in response (session maintained)")
                return True
        else:
            print_error(f"Request failed with status: {response.status_code}")
            
    except requests.exceptions.RequestException as e:
        print_error(f"Request failed: {e}")
    
    return False

def test_multiple_cookies(base_url):
    """Test setting and receiving multiple cookies"""
    print_info("Testing multiple cookies...")
    
    try:
        # First request to get initial session
        response1 = requests.get(f"{base_url}/", timeout=5)
        
        if response1.status_code == 200:
            session = requests.Session()
            
            # Make several requests to see cookie behavior
            for i in range(3):
                response = session.get(f"{base_url}/", timeout=5)
                
                if response.status_code == 200:
                    print_success(f"Request {i+1} successful")
                    
                    if 'Set-Cookie' in response.headers:
                        print_info(f"Set-Cookie in request {i+1}: {response.headers['Set-Cookie']}")
                    
                    # Check what cookies the session is sending
                    if session.cookies:
                        cookie_str = "; ".join([f"{name}={value}" for name, value in session.cookies.items()])
                        print_info(f"Cookies being sent: {cookie_str}")
                else:
                    print_error(f"Request {i+1} failed with status: {response.status_code}")
                    
                time.sleep(0.5)  # Small delay between requests
                
            return True
        else:
            print_error(f"Initial request failed with status: {response1.status_code}")
            
    except requests.exceptions.RequestException as e:
        print_error(f"Request failed: {e}")
    
    return False

def test_cookie_security(base_url):
    """Test cookie security attributes"""
    print_info("Testing cookie security attributes...")
    
    try:
        response = requests.get(f"{base_url}/", timeout=5)
        
        if response.status_code == 200 and 'Set-Cookie' in response.headers:
            cookie_header = response.headers['Set-Cookie']
            print_info(f"Cookie header: {cookie_header}")
            
            # Check for security attributes
            security_checks = {
                'HttpOnly': 'HttpOnly' in cookie_header,
                'Path=/': 'Path=/' in cookie_header,
                'Secure': 'Secure' in cookie_header
            }
            
            for attr, present in security_checks.items():
                if present:
                    print_success(f"Security attribute '{attr}' found")
                else:
                    if attr == 'Secure':
                        print_info(f"Security attribute '{attr}' not found (normal for HTTP)")
                    else:
                        print_warning(f"Security attribute '{attr}' not found")
            
            return True
        else:
            print_error("No cookies to check security attributes")
            
    except requests.exceptions.RequestException as e:
        print_error(f"Request failed: {e}")
    
    return False

def main():
    if len(sys.argv) > 1:
        base_url = sys.argv[1]
    else:
        base_url = "http://localhost:8081"
    
    print(f"{Colors.BLUE}=== Cookie and Session Test for Webserv ==={Colors.RESET}")
    print(f"Testing server at: {base_url}")
    print()
    
    # Test 1: Session Creation
    print(f"{Colors.YELLOW}Test 1: Session Creation{Colors.RESET}")
    session_id = test_session_creation(base_url)
    print()
    
    # Test 2: Session Persistence
    print(f"{Colors.YELLOW}Test 2: Session Persistence{Colors.RESET}")
    if session_id:
        test_session_persistence(base_url, session_id)
    else:
        print_warning("Skipping session persistence test (no session ID)")
    print()
    
    # Test 3: Multiple Cookies
    print(f"{Colors.YELLOW}Test 3: Multiple Cookies{Colors.RESET}")
    test_multiple_cookies(base_url)
    print()
    
    # Test 4: Cookie Security
    print(f"{Colors.YELLOW}Test 4: Cookie Security Attributes{Colors.RESET}")
    test_cookie_security(base_url)
    print()
    
    print(f"{Colors.BLUE}=== Cookie Tests Complete ==={Colors.RESET}")

if __name__ == "__main__":
    main()
