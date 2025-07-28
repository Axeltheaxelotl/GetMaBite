#!/usr/bin/env python3
"""
Simple cookie test using basic HTTP requests
"""

import socket
import time

def send_http_request(host, port, path, cookie=None):
    """Send a raw HTTP request and return the response"""
    try:
        # Create socket
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5)
        sock.connect((host, port))
        
        # Build HTTP request
        request = f"GET {path} HTTP/1.1\r\n"
        request += f"Host: {host}:{port}\r\n"
        request += "User-Agent: CookieTest/1.0\r\n"
        if cookie:
            request += f"Cookie: {cookie}\r\n"
        request += "Connection: close\r\n\r\n"
        
        # Send request
        sock.send(request.encode())
        
        # Receive response
        response = b""
        while True:
            data = sock.recv(4096)
            if not data:
                break
            response += data
        
        sock.close()
        return response.decode('utf-8', errors='ignore')
        
    except Exception as e:
        return f"Error: {e}"

def extract_cookies(response):
    """Extract Set-Cookie headers from HTTP response"""
    cookies = []
    lines = response.split('\n')
    for line in lines:
        if line.startswith('Set-Cookie:'):
            cookie = line[11:].strip()
            cookies.append(cookie)
    return cookies

def main():
    host = "localhost"
    port = 8081
    
    print("🍪 Simple Cookie Test for Webserv")
    print("=" * 40)
    
    # Test 1: First request (should create session)
    print("\n📝 Test 1: First request (new session)")
    response1 = send_http_request(host, port, "/small.html")
    
    if "Set-Cookie:" in response1:
        cookies1 = extract_cookies(response1)
        print(f"✅ Cookies received: {len(cookies1)}")
        for cookie in cookies1:
            print(f"   🍪 {cookie}")
        
        # Extract session ID for next request
        session_cookie = None
        for cookie in cookies1:
            if "SESSIONID=" in cookie:
                session_cookie = cookie.split(';')[0]  # Just the name=value part
                print(f"📌 Session cookie extracted: {session_cookie}")
                break
    else:
        print("❌ No cookies found in response")
        session_cookie = None
    
    # Test 2: Second request with session cookie
    print("\n📝 Test 2: Request with existing session cookie")
    if session_cookie:
        response2 = send_http_request(host, port, "/small.html", session_cookie)
        
        if "200 OK" in response2:
            print("✅ Request successful with existing session")
            
            # Check if server still sends cookies (might or might not)
            if "Set-Cookie:" in response2:
                cookies2 = extract_cookies(response2)
                print(f"   🍪 Additional cookies: {len(cookies2)}")
                for cookie in cookies2:
                    print(f"      {cookie}")
            else:
                print("   ℹ️  No new cookies (session maintained)")
        else:
            print("❌ Request failed")
    else:
        print("⚠️  Skipping (no session cookie from first request)")
    
    # Test 3: Request without cookie (should create new session)
    print("\n📝 Test 3: Request without cookie (new session)")
    time.sleep(1)  # Small delay
    response3 = send_http_request(host, port, "/small.html")
    
    if "Set-Cookie:" in response3:
        cookies3 = extract_cookies(response3)
        print(f"✅ New session created: {len(cookies3)} cookies")
        for cookie in cookies3:
            print(f"   🍪 {cookie}")
    else:
        print("❌ No new session created")
    
    print("\n🎉 Cookie test completed!")

if __name__ == "__main__":
    main()
