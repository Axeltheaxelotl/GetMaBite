#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import cgi
import cgitb
import os
import sys
import json
import hashlib
import time
import urllib.parse
from datetime import datetime, timedelta

# Enable CGI error reporting
cgitb.enable()

# Session storage directory (should be writable by web server)
SESSION_DIR = "/tmp/webserv_sessions"
SESSION_TIMEOUT = 1800  # 30 minutes

def ensure_session_dir():
    """Ensure session directory exists"""
    try:
        if not os.path.exists(SESSION_DIR):
            os.makedirs(SESSION_DIR, mode=0o755)
        return True
    except OSError:
        return False

def generate_session_id():
    """Generate a secure session ID"""
    return hashlib.sha256(f"{time.time()}{os.urandom(16)}".encode()).hexdigest()

def get_session_file(session_id):
    """Get the path to a session file"""
    return os.path.join(SESSION_DIR, f"sess_{session_id}")

def load_session(session_id):
    """Load session data from file"""
    if not session_id:
        return None
    
    session_file = get_session_file(session_id)
    if not os.path.exists(session_file):
        return None
    
    try:
        with open(session_file, 'r') as f:
            session_data = json.load(f)
        
        # Check if session has expired
        last_activity = datetime.fromisoformat(session_data.get('last_activity', '1970-01-01'))
        if datetime.now() - last_activity > timedelta(seconds=SESSION_TIMEOUT):
            os.remove(session_file)
            return None
        
        return session_data
    except (json.JSONDecodeError, OSError, ValueError):
        return None

def save_session(session_id, session_data):
    """Save session data to file"""
    if not ensure_session_dir():
        return False
    
    session_file = get_session_file(session_id)
    session_data['last_activity'] = datetime.now().isoformat()
    
    try:
        with open(session_file, 'w') as f:
            json.dump(session_data, f, indent=2)
        return True
    except OSError:
        return False

def cleanup_expired_sessions():
    """Remove expired session files"""
    if not os.path.exists(SESSION_DIR):
        return
    
    current_time = datetime.now()
    for filename in os.listdir(SESSION_DIR):
        if filename.startswith('sess_'):
            filepath = os.path.join(SESSION_DIR, filename)
            try:
                stat_info = os.stat(filepath)
                file_time = datetime.fromtimestamp(stat_info.st_mtime)
                if current_time - file_time > timedelta(seconds=SESSION_TIMEOUT):
                    os.remove(filepath)
            except OSError:
                pass

def get_cookie_value(cookie_name):
    """Extract cookie value from HTTP_COOKIE"""
    http_cookie = os.environ.get('HTTP_COOKIE', '')
    for cookie in http_cookie.split(';'):
        cookie = cookie.strip()
        if '=' in cookie:
            name, value = cookie.split('=', 1)
            if name == cookie_name:
                return urllib.parse.unquote(value)
    return None

def set_cookie(name, value, expires_minutes=30):
    """Generate Set-Cookie header"""
    if expires_minutes > 0:
        expires = datetime.now() + timedelta(minutes=expires_minutes)
        expires_str = expires.strftime('%a, %d %b %Y %H:%M:%S GMT')
        return f"Set-Cookie: {name}={urllib.parse.quote(value)}; expires={expires_str}; path=/; HttpOnly"
    else:
        # For negative expires, set expiration to past date to delete cookie
        expires_str = 'Thu, 01 Jan 1970 00:00:00 GMT'
        return f"Set-Cookie: {name}=; expires={expires_str}; path=/; HttpOnly"

def main():
    # Cleanup expired sessions periodically
    cleanup_expired_sessions()
    
    # Parse form data with better error handling
    form_dict = {}
    action = 'status'
    
    try:
        form = cgi.FieldStorage()
        if form.keys():
            # We have proper CGI form data
            action = form.getvalue('action', 'status')
            for key in form.keys():
                form_dict[key] = form.getvalue(key)
        else:
            # No CGI form data, try reading from stdin
            raise Exception("No form data, falling back to stdin")
    except Exception:
        # Fallback: try to read from stdin for testing
        import sys
        try:
            stdin_data = sys.stdin.read().strip()
            if stdin_data and '=' in stdin_data:
                pairs = stdin_data.split('&')
                for pair in pairs:
                    if '=' in pair:
                        key, value = pair.split('=', 1)
                        form_dict[key] = urllib.parse.unquote_plus(value)
                action = form_dict.get('action', 'status')
        except Exception:
            action = 'status'
    
    def get_form_value(key, default=''):
        """Get form value from either FieldStorage or dict"""
        if form_dict:
            return form_dict.get(key, default)
        try:
            return form.getvalue(key, default) if 'form' in locals() else default
        except:
            return default
    
    # Get current session ID from cookie
    session_id = get_cookie_value('WEBSERV_SESSION_ID')
    session_data = load_session(session_id) if session_id else None
    
    response_data = {
        'success': True,
        'action': action,
        'timestamp': datetime.now().isoformat(),
        'session_id': session_id,
        'session_active': bool(session_data)
    }
    
    headers = ["Content-Type: application/json"]
    
    if action == 'create':
        # Create new session
        new_session_id = generate_session_id()
        new_session_data = {
            'created': datetime.now().isoformat(),
            'last_activity': datetime.now().isoformat(),
            'user_agent': os.environ.get('HTTP_USER_AGENT', 'Unknown'),
            'remote_addr': os.environ.get('REMOTE_ADDR', 'Unknown'),
            'data': {}
        }
        
        if save_session(new_session_id, new_session_data):
            headers.append(set_cookie('WEBSERV_SESSION_ID', new_session_id, 30))
            response_data.update({
                'session_id': new_session_id,
                'session_data': new_session_data,
                'message': 'Session created successfully'
            })
        else:
            response_data.update({
                'success': False,
                'message': 'Failed to create session'
            })
    
    elif action == 'destroy':
        # Destroy current session
        if session_id and session_data:
            session_file = get_session_file(session_id)
            try:
                if os.path.exists(session_file):
                    os.remove(session_file)
                headers.append(set_cookie('WEBSERV_SESSION_ID', '', -1))  # Expire cookie
                response_data.update({
                    'message': 'Session destroyed successfully',
                    'session_id': None,
                    'session_active': False
                })
            except OSError:
                response_data.update({
                    'success': False,
                    'message': 'Failed to destroy session'
                })
        else:
            response_data.update({
                'message': 'No active session to destroy'
            })
    
    elif action == 'set_data':
        # Set session data
        key = get_form_value('key', '')
        value = get_form_value('value', '')
        
        if not session_data:
            response_data.update({
                'success': False,
                'message': 'No active session'
            })
        elif not key:
            response_data.update({
                'success': False,
                'message': 'Key is required'
            })
        else:
            session_data['data'][key] = value
            if save_session(session_id, session_data):
                response_data.update({
                    'message': f'Session data set: {key}={value}',
                    'session_data': session_data
                })
            else:
                response_data.update({
                    'success': False,
                    'message': 'Failed to save session data'
                })
    
    elif action == 'get_data':
        # Get session data
        key = get_form_value('key', '')
        
        if not session_data:
            response_data.update({
                'success': False,
                'message': 'No active session'
            })
        elif key and key in session_data.get('data', {}):
            response_data.update({
                'key': key,
                'value': session_data['data'][key],
                'message': f'Session data retrieved: {key}'
            })
        elif key:
            response_data.update({
                'success': False,
                'message': f'Key not found: {key}'
            })
        else:
            response_data.update({
                'session_data': session_data.get('data', {}),
                'message': 'All session data retrieved'
            })
    
    elif action == 'status':
        # Get session status
        if session_data:
            created = datetime.fromisoformat(session_data['created'])
            last_activity = datetime.fromisoformat(session_data['last_activity'])
            age_seconds = int((datetime.now() - created).total_seconds())
            idle_seconds = int((datetime.now() - last_activity).total_seconds())
            
            response_data.update({
                'session_data': {
                    'id': session_id,
                    'created': session_data['created'],
                    'last_activity': session_data['last_activity'],
                    'age_seconds': age_seconds,
                    'idle_seconds': idle_seconds,
                    'user_agent': session_data.get('user_agent', 'Unknown'),
                    'remote_addr': session_data.get('remote_addr', 'Unknown'),
                    'data_count': len(session_data.get('data', {})),
                    'data': session_data.get('data', {})
                },
                'message': 'Session status retrieved'
            })
            
            # Update last activity
            save_session(session_id, session_data)
        else:
            response_data.update({
                'message': 'No active session'
            })
    
    elif action == 'list_all':
        # List all active sessions (for debugging)
        sessions = []
        if os.path.exists(SESSION_DIR):
            for filename in os.listdir(SESSION_DIR):
                if filename.startswith('sess_'):
                    session_file = os.path.join(SESSION_DIR, filename)
                    try:
                        with open(session_file, 'r') as f:
                            session_info = json.load(f)
                        sessions.append({
                            'id': filename[5:],  # Remove 'sess_' prefix
                            'created': session_info.get('created'),
                            'last_activity': session_info.get('last_activity'),
                            'data_count': len(session_info.get('data', {}))
                        })
                    except (json.JSONDecodeError, OSError):
                        pass
        
        response_data.update({
            'sessions': sessions,
            'total_sessions': len(sessions),
            'message': f'Found {len(sessions)} active sessions'
        })
    
    else:
        response_data.update({
            'success': False,
            'message': f'Unknown action: {action}'
        })
    
    # Output headers
    for header in headers:
        print(header)
    print()  # Empty line to separate headers from body
    
    # Output JSON response
    print(json.dumps(response_data, indent=2))

if __name__ == '__main__':
    main()
