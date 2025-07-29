#!/usr/bin/env python3

# Form processing CGI
import os
import sys
import urllib.parse

print("Content-Type: text/html")
print()

print("""<!DOCTYPE html>
<html>
<head>
    <title>Form Processor CGI</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        .form-container { max-width: 500px; }
        .field { margin: 10px 0; }
        label { display: block; margin-bottom: 5px; }
        input, textarea { width: 100%; padding: 5px; }
        button { background: #007bff; color: white; padding: 10px 20px; border: none; border-radius: 3px; }
        .result { background: #f8f9fa; padding: 15px; border-left: 4px solid #007bff; margin: 20px 0; }
    </style>
</head>
<body>
    <h1>📝 Form Processing CGI</h1>
""")

method = os.environ.get('REQUEST_METHOD', 'GET')

if method == 'POST':
    content_length = os.environ.get('CONTENT_LENGTH')
    if content_length and content_length.isdigit():
        try:
            post_data = sys.stdin.read(int(content_length))
            parsed_data = urllib.parse.parse_qs(post_data)
            
            print('<div class="result">')
            print('<h3>✅ Form Submitted Successfully!</h3>')
            print('<p><strong>Received data:</strong></p>')
            print('<ul>')
            for key, values in parsed_data.items():
                for value in values:
                    print(f'<li><strong>{key}:</strong> {value}</li>')
            print('</ul>')
            print('</div>')
        except Exception as e:
            print(f'<div class="result"><h3>❌ Error processing form:</h3><p>{e}</p></div>')

print("""
    <div class="form-container">
        <form method="post" action="/tests/form_processor.cgi.py">
            <div class="field">
                <label for="name">Name:</label>
                <input type="text" id="name" name="name" required>
            </div>
            <div class="field">
                <label for="email">Email:</label>
                <input type="email" id="email" name="email" required>
            </div>
            <div class="field">
                <label for="message">Message:</label>
                <textarea id="message" name="message" rows="4" required></textarea>
            </div>
            <div class="field">
                <button type="submit">Submit Form</button>
            </div>
        </form>
    </div>
</body>
</html>
""")
