import cgi
import sys

#!/usr/bin/env python3

def main():
    print("Content-Type: text/html")
    print()
    
    form = cgi.FieldStorage()
    args = {key: form.getvalue(key) for key in form.keys()}
    
    html = "<html><body><ul>"
    for key, value in args.items():
        html += f"<li>{key}: {value}</li>"
    html += "</ul>"
    
    html += "<h1>Command Line Arguments</h1><ul>"
    for arg in sys.argv:
        html += f"<li>{arg}</li>"
    html += "</ul></body></html>"
    
    print(html)

if __name__ == "__main__":
    main()