#!/usr/bin/env python3
import cgi
import sys
import os
import time
import signal
import urllib.parse

def timeout_handler(signum, frame):
    # Timeout handler for CGI script
    raise TimeoutError("CGI script timed out")

def main():
    signal.signal(signal.SIGALRM, timeout_handler)
    signal.alarm(10)  # set timeout to 10 seconds
    try:
        # Print valid HTTP response header
        print("HTTP/1.1 200 OK")
        print("Content-Type: text/html")
        print()
        # Send debug output to stderr so it doesn't interfere with HTTP body
        
        query = os.environ.get("QUERY_STRING", "")
        args = urllib.parse.parse_qs(query)
        args = {k: v[0] if len(v) == 1 else v for k, v in args.items()}
        
        req_method = os.environ.get("REQUEST_METHOD", "GET").upper()
        if req_method == "POST":
            try:
                content_length = int(os.environ.get('CONTENT_LENGTH', 0))
                body = os.environ.get('HTTP_BODY', '')
            except ValueError:
                content_length = 0
                body = ""
        else:
            content_length = None
            body = None
        html = """
        <html>
        <head>
            <title>CGI Script Demo</title>
            <style>
                body { font-family: Arial, sans-serif; margin: 20px; }
                h1 { color: #333; }
                ul { list-style-type: none; padding: 0; }
                li { margin: 5px 0; }
                .key { font-weight: bold; color: #555; }
                .value { color: #007BFF; }
            </style>
        </head>
        <body>
            <h1>CGI Script Demo</h1>
            <h2>Form Data</h2>
            <ul>
        """
        
        if args:
            for key, value in args.items():
                html += f"<li><span class='key'>{key}:</span> <span class='value'>{value}</span></li>"
        else:
            html += "<li>No form data received.</li>"
        
        html += """
            </ul>
            <h2>Environment Variables</h2>
            <ul>
        """
        
        for key, value in sorted(os.environ.items()):
            html += f"<li><span class='key'>{key}:</span> <span class='value'>{value}</span></li>"
        html += """
            </ul>
        """
        if body is not None:
            # Only add body sections for POST requests
            html += """
            <h2>Request Body</h2>
            <pre>
        """ + body + """
            </pre>
            <h2>Debugging Information</h2>
            <ul>
                <li><span class='key'>CONTENT_LENGTH:</span> <span class='value'>""" + str(content_length) + """</span></li>
                <li><span class='key'>Raw Body:</span> <pre>""" + repr(body) + """</pre></li>
            </ul>
            """
        html += """
        </body>
        </html>
        """
        
        print(html)
    except TimeoutError:
        print("HTTP/1.1 504 Gateway Timeout")
        print("Content-Type: text/html")
        print()
        print("<html><body><h1>504 Gateway Timeout</h1></body></html>")
    finally:
        signal.alarm(0)

if __name__ == "__main__":
    main()