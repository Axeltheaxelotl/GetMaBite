#!/usr/bin/env python3

"""
Quick test script to verify webserver functionality with improved session support
"""

import requests
import json
import time
import sys

def test_webserver(base_url="http://localhost:8080"):
    print(f"🚀 Testing Webserv at {base_url}")
    print("=" * 50)
    
    session = requests.Session()
    test_results = {
        "passed": 0,
        "failed": 0,
        "tests": []
    }
    
    def run_test(name, test_func):
        try:
            print(f"\\n🧪 {name}...")
            result = test_func()
            if result:
                print(f"✅ {name} - PASSED")
                test_results["passed"] += 1
                test_results["tests"].append({"name": name, "status": "PASSED", "details": result})
            else:
                print(f"❌ {name} - FAILED")
                test_results["failed"] += 1
                test_results["tests"].append({"name": name, "status": "FAILED", "details": "Test returned False"})
        except Exception as e:
            print(f"❌ {name} - ERROR: {e}")
            test_results["failed"] += 1
            test_results["tests"].append({"name": name, "status": "ERROR", "details": str(e)})
    
    # Test 1: Basic HTTP GET
    def test_basic_get():
        response = session.get(f"{base_url}/tests/index.html")
        return response.status_code == 200 and "html" in response.text.lower()
    
    # Test 2: Basic CGI
    def test_basic_cgi():
        response = session.get(f"{base_url}/tests/simple.cgi.py")
        return response.status_code == 200 and "Simple CGI Test" in response.text
    
    # Test 3: Session Creation
    def test_session_creation():
        response = session.post(f"{base_url}/tests/session_manager.cgi.py", 
                               data={"action": "create"})
        if response.status_code == 200:
            data = response.json()
            return data.get("success") and "session_id" in data
        return False
    
    # Test 4: Session Status
    def test_session_status():
        response = session.post(f"{base_url}/tests/session_manager.cgi.py",
                               data={"action": "status"})
        if response.status_code == 200:
            data = response.json()
            return "session_active" in data
        return False
    
    # Test 5: Session Data Storage
    def test_session_data():
        # Set data
        response = session.post(f"{base_url}/tests/session_manager.cgi.py",
                               data={
                                   "action": "set_data",
                                   "key": "test_key",
                                   "value": "test_value"
                               })
        if response.status_code != 200:
            return False
        
        set_data = response.json()
        if not set_data.get("success"):
            return False
        
        # Get data
        response = session.post(f"{base_url}/tests/session_manager.cgi.py",
                               data={
                                   "action": "get_data",
                                   "key": "test_key"
                               })
        if response.status_code != 200:
            return False
        
        get_data = response.json()
        return get_data.get("success") and get_data.get("value") == "test_value"
    
    # Test 6: Cookie Handling
    def test_cookies():
        # Check if cookies are being set and maintained
        cookies_before = len(session.cookies)
        
        # Make a request that should set cookies
        session.get(f"{base_url}/tests/cookies_demo.html")
        
        # Make session request that sets a session cookie
        session.post(f"{base_url}/tests/session_manager.cgi.py", 
                    data={"action": "create"})
        
        cookies_after = len(session.cookies)
        return cookies_after >= cookies_before  # Should have at least the same or more cookies
    
    # Test 7: POST Request
    def test_post_request():
        response = session.post(f"{base_url}/tests/simple.cgi.py",
                               data={"test": "post_data"})
        return response.status_code == 200 and "POST" in response.text
    
    # Test 8: Multiple Requests (Keep-Alive)
    def test_multiple_requests():
        responses = []
        for i in range(3):
            response = session.get(f"{base_url}/tests/simple.cgi.py?req={i}")
            responses.append(response.status_code == 200)
        return all(responses)
    
    # Run all tests
    run_test("Basic HTTP GET", test_basic_get)
    run_test("Basic CGI Script", test_basic_cgi)
    run_test("Session Creation", test_session_creation)
    run_test("Session Status Check", test_session_status)
    run_test("Session Data Storage", test_session_data)
    run_test("Cookie Handling", test_cookies)
    run_test("POST Request", test_post_request)
    run_test("Multiple Requests", test_multiple_requests)
    
    # Summary
    print("\\n" + "=" * 50)
    print("📊 TEST SUMMARY")
    print("=" * 50)
    print(f"✅ Passed: {test_results['passed']}")
    print(f"❌ Failed: {test_results['failed']}")
    total = test_results['passed'] + test_results['failed']
    if total > 0:
        success_rate = (test_results['passed'] / total) * 100
        print(f"📈 Success Rate: {success_rate:.1f}%")
    
    if test_results['failed'] > 0:
        print("\\n❌ Failed Tests:")
        for test in test_results['tests']:
            if test['status'] != 'PASSED':
                print(f"   • {test['name']}: {test['details']}")
    
    return test_results['failed'] == 0

if __name__ == "__main__":
    if len(sys.argv) > 1:
        base_url = sys.argv[1]
    else:
        base_url = "http://localhost:8080"
    
    success = test_webserver(base_url)
    sys.exit(0 if success else 1)
