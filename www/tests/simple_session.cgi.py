#!/usr/bin/env python3
"""
Simple Session Manager - Clean implementation from scratch
Handles session creation, storage, and management with proper cookie handling
"""

import os
import sys
import json
import time
import hashlib
import urllib.parse
from datetime import datetime

# Configuration
SESSION_STORAGE = "/tmp/webserv_simple_sessions"
COOKIE_NAME = "SIMPLE_SESSION"
SESSION_LIFETIME = 3600  # 1 hour

def ensure_storage_dir():
    """Create session storage directory if it doesn't exist"""
    if not os.path.exists(SESSION_STORAGE):
        os.makedirs(SESSION_STORAGE, 0o755)

def get_session_path(session_id):
    """Get full path to session file"""
    return os.path.join(SESSION_STORAGE, f"{session_id}.json")

def generate_session_id():
    """Generate a unique session ID"""
    timestamp = str(time.time())
    random_data = os.urandom(32)
    return hashlib.sha256(f"{timestamp}{random_data}".encode()).hexdigest()[:32]

def read_session(session_id):
    """Load session data from file. Returns (data, was_expired) tuple"""
    if not session_id:
        return None, False
    
    session_path = get_session_path(session_id)
    if not os.path.exists(session_path):
        return None, False
    
    try:
        with open(session_path, 'r') as f:
            data = json.load(f)
        
        # Check if session is expired
        if time.time() - data.get('created', 0) > SESSION_LIFETIME:
            os.remove(session_path)  # Clean up expired session
            return None, True  # Return True to indicate session was expired
        
        # Update last accessed time
        data['last_accessed'] = time.time()
        with open(session_path, 'w') as f:
            json.dump(data, f)
        
        return data, False
    except (json.JSONDecodeError, OSError):
        return None, False

def write_session(session_id, data):
    """Save session data to file"""
    ensure_storage_dir()
    session_path = get_session_path(session_id)
    
    try:
        with open(session_path, 'w') as f:
            json.dump(data, f, indent=2)
        return True
    except OSError:
        return False

def delete_session(session_id):
    """Delete session file"""
    if not session_id:
        return False
    
    session_path = get_session_path(session_id)
    try:
        if os.path.exists(session_path):
            os.remove(session_path)
        return True
    except OSError:
        return False

def get_current_session_id():
    """Extract session ID from HTTP_COOKIE"""
    cookie_header = os.environ.get('HTTP_COOKIE', '')
    for cookie in cookie_header.split(';'):
        cookie = cookie.strip()
        if '=' in cookie:
            name, value = cookie.split('=', 1)
            if name.strip() == COOKIE_NAME:
                return value.strip()
    return None

def create_cookie_header(session_id, delete=False):
    """Create Set-Cookie header"""
    if delete:
        return f"Set-Cookie: {COOKIE_NAME}=; Path=/; Max-Age=0; Expires=Thu, 01 Jan 1970 00:00:00 GMT"
    else:
        return f"Set-Cookie: {COOKIE_NAME}={session_id}; Path=/; Max-Age={SESSION_LIFETIME}"

def get_all_sessions():
    """Get list of all active sessions"""
    sessions = []
    if os.path.exists(SESSION_STORAGE):
        for filename in os.listdir(SESSION_STORAGE):
            if filename.endswith('.json'):
                session_id = filename[:-5]  # Remove .json extension
                session_data, _ = read_session(session_id)  # Ignore expired flag here
                if session_data:
                    sessions.append({
                        'id': session_id,
                        'created': session_data.get('created', 0),
                        'last_accessed': session_data.get('last_accessed', 0),
                        'data_keys': list(session_data.get('user_data', {}).keys())
                    })
    return sessions

def main():
    """Main CGI handler"""
    # Parse request data
    content_length = int(os.environ.get('CONTENT_LENGTH', 0))
    if content_length > 0:
        post_data = sys.stdin.read(content_length)
        params = urllib.parse.parse_qs(post_data)
        action = params.get('action', ['info'])[0]
    else:
        action = 'info'
    
    # Get current session
    current_session_id = get_current_session_id()
    current_session = None
    session_was_expired = False
    
    if current_session_id:
        current_session, session_was_expired = read_session(current_session_id)
    
    response = {
        'success': True,
        'action': action,
        'timestamp': time.time(),
        'session_id': current_session_id,
        'has_session': bool(current_session),
        'message': ''
    }
    
    headers = ['Content-Type: application/json']
    
    # If session was expired, clear the stale cookie
    if session_was_expired:
        headers.append(create_cookie_header('', delete=True))
        response.update({
            'session_id': None,
            'has_session': False,
            'message': 'Previous session was expired and cleared'
        })
        
        # For expired sessions, only allow creating a new session
        if action not in ['create', 'info', 'list']:
            response.update({
                'success': False,
                'message': 'Previous session was expired and cleared. Please create a new session.'
            })
            # Output response early for expired sessions with invalid actions
            for header in headers:
                print(header)
            print()  # Empty line between headers and body
            print(json.dumps(response, indent=2))
            return
    
    # Handle different actions
    if action == 'create':
        # Create new session
        new_session_id = generate_session_id()
        session_data = {
            'created': time.time(),
            'last_accessed': time.time(),
            'user_data': {}
        }
        
        if write_session(new_session_id, session_data):
            headers.append(create_cookie_header(new_session_id))
            response.update({
                'session_id': new_session_id,
                'has_session': True,
                'message': 'Session created successfully'
            })
        else:
            response.update({
                'success': False,
                'message': 'Failed to create session'
            })
    
    elif action == 'destroy':
        # Destroy current session
        if current_session_id:
            delete_session(current_session_id)
            headers.append(create_cookie_header('', delete=True))
            response.update({
                'session_id': None,
                'has_session': False,
                'message': 'Session destroyed'
            })
        else:
            response.update({
                'message': 'No session to destroy'
            })
    
    elif action == 'set':
        # Set session data
        key = params.get('key', [''])[0]
        value = params.get('value', [''])[0]
        
        if not current_session:
            response.update({
                'success': False,
                'message': 'No active session'
            })
        elif not key:
            response.update({
                'success': False,
                'message': 'Key is required'
            })
        else:
            current_session['user_data'][key] = value
            if write_session(current_session_id, current_session):
                response.update({
                    'message': f'Set {key} = {value}',
                    'data': current_session['user_data']
                })
            else:
                response.update({
                    'success': False,
                    'message': 'Failed to save session data'
                })
    
    elif action == 'get':
        # Get session data
        key = params.get('key', [''])[0]
        
        if not current_session:
            response.update({
                'success': False,
                'message': 'No active session'
            })
        elif key:
            user_data = current_session.get('user_data', {})
            if key in user_data:
                response.update({
                    'key': key,
                    'value': user_data[key],
                    'message': f'Retrieved {key}'
                })
            else:
                response.update({
                    'success': False,
                    'message': f'Key "{key}" not found'
                })
        else:
            response.update({
                'data': current_session.get('user_data', {}),
                'message': 'Retrieved all session data'
            })
    
    elif action == 'list':
        # List all sessions
        all_sessions = get_all_sessions()
        response.update({
            'sessions': all_sessions,
            'total': len(all_sessions),
            'message': f'Found {len(all_sessions)} active sessions'
        })
    
    elif action == 'info':
        # Get session info
        if current_session:
            response.update({
                'session_info': {
                    'created': current_session.get('created', 0),
                    'last_accessed': current_session.get('last_accessed', 0),
                    'age_seconds': time.time() - current_session.get('created', 0),
                    'data_count': len(current_session.get('user_data', {})),
                    'data': current_session.get('user_data', {})
                },
                'message': 'Session info retrieved'
            })
        else:
            response.update({
                'message': 'No active session'
            })
    
    else:
        response.update({
            'success': False,
            'message': f'Unknown action: {action}'
        })
    
    # Output response
    for header in headers:
        print(header)
    print()  # Empty line between headers and body
    print(json.dumps(response, indent=2))

if __name__ == '__main__':
    main()
