import tkinter as tk
from tkinter import scrolledtext
import socket
import threading
import time

class TimeoutLogTester:
    def __init__(self, master):
        self.master = master
        self.master.title("Timeout and Log Tester")

        # Create GUI elements
        self.log_label = tk.Label(master, text="Server Logs:")
        self.log_label.pack()

        self.log_text = scrolledtext.ScrolledText(master, width=80, height=20)
        self.log_text.pack()

        self.test_button = tk.Button(master, text="Start Test", command=self.start_test)
        self.test_button.pack()

        self.stop_button = tk.Button(master, text="Stop Test", command=self.stop_test, state=tk.DISABLED)
        self.stop_button.pack()

        self.status_label = tk.Label(master, text="Status: Ready")
        self.status_label.pack()

        self.running = False

    def start_test(self):
        self.running = True
        self.test_button.config(state=tk.DISABLED)
        self.stop_button.config(state=tk.NORMAL)
        self.status_label.config(text="Status: Running")
        threading.Thread(target=self.run_test).start()

    def stop_test(self):
        self.running = False
        self.test_button.config(state=tk.NORMAL)
        self.stop_button.config(state=tk.DISABLED)
        self.status_label.config(text="Status: Stopped")

    def run_test(self):
        try:
            # Connect to the server
            self.log_text.insert(tk.END, "Connecting to server...\n")
            self.log_text.see(tk.END)

            client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            client_socket.connect(("localhost", 8081))
            self.log_text.insert(tk.END, "Connected to server.\n")
            self.log_text.see(tk.END)

            # Send partial request and wait for timeout
            client_socket.sendall(b"GET / HTTP/1.1\r\nHost: localhost\r\n\r\n")
            self.log_text.insert(tk.END, "Sent partial request. Waiting for timeout...\n")
            self.log_text.see(tk.END)

            start_time = time.time()
            while self.running:
                try:
                    data = client_socket.recv(1024)
                    if not data:
                        break
                except socket.timeout:
                    pass

                elapsed_time = time.time() - start_time
                if elapsed_time > 10:  # Assuming timeout is set to 10 seconds
                    self.log_text.insert(tk.END, "Timeout occurred.\n")
                    self.log_text.see(tk.END)
                    break

            client_socket.close()
            self.log_text.insert(tk.END, "Connection closed.\n")
            self.log_text.see(tk.END)

        except Exception as e:
            self.log_text.insert(tk.END, f"Error: {e}\n")
            self.log_text.see(tk.END)

        finally:
            self.stop_test()

if __name__ == "__main__":
    root = tk.Tk()
    app = TimeoutLogTester(root)
    root.mainloop()
