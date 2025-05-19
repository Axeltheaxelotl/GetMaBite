import tkinter as tk
from tkinter import messagebox
import requests

def send_request():
    url = url_entry.get()
    try:
        body_size = int(body_size_entry.get())
    except ValueError:
        messagebox.showerror("Invalid Input", "Body size must be an integer.")
        return

    body = "A" * body_size
    try:
        response = requests.post(url, data=body)
        result_text.set(f"Response Code: {response.status_code}\nResponse Body: {response.text[:200]}...")
    except Exception as e:
        result_text.set(f"Error: {e}")

# Create the main window
root = tk.Tk()
root.title("Client Max Body Size Tester")

# URL input
url_label = tk.Label(root, text="Server URL:")
url_label.pack()
url_entry = tk.Entry(root, width=50)
url_entry.insert(0, "http://localhost:8081")
url_entry.pack()

# Body size input
body_size_label = tk.Label(root, text="Body Size (bytes):")
body_size_label.pack()
body_size_entry = tk.Entry(root, width=20)
body_size_entry.insert(0, "1024")
body_size_entry.pack()

# Send button
send_button = tk.Button(root, text="Send Request", command=send_request)
send_button.pack()

# Result display
result_text = tk.StringVar()
result_label = tk.Label(root, textvariable=result_text, wraplength=400, justify="left")
result_label.pack()

# Run the application
root.mainloop()
