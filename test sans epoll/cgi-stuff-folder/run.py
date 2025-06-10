#!/usr/bin/env python3
import cgi
import sys
import os
import time

def main():
    # Print valid HTTP response header
    print("HTTP/1.1 200 OK")
    print("Content-Type: text/html")
    print()
    # Send debug output to stderr so it doesn't interfere with HTTP body
    for i in range(5, 0, -1):
        sys.stderr.write(f"[DEBUG] Waiting: {i} seconds remaining...\n")
        sys.stderr.flush()
        time.sleep(1)
    form = cgi.FieldStorage()
    args = {key: form.getvalue(key) for key in form.keys()}
    
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
        <h2>Command Line Arguments</h2>
        <ul>
    """
    
    if len(sys.argv) > 1:
        for arg in sys.argv[1:]:
            html += f"<li>{arg}</li>"
    else:
        html += "<li>No command line arguments provided.</li>"
    
    html += """
        </ul>
        <h2>Environment Variables</h2>
        <ul>
    """
    
    for key, value in sorted(os.environ.items()):
        html += f"<li><span class='key'>{key}:</span> <span class='value'>{value}</span></li>"
    
    html += """
        </ul>
    </body>
    </html>
    """
    
    print(html)

if __name__ == "__main__":
    main()