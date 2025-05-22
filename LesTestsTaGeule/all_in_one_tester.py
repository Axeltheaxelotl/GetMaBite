import tkinter as tk
from tkinter import ttk, scrolledtext, messagebox
import threading
import requests
import socket
import time

# --- Test Functions ---
def test_error_pages(log, url_base):
    ERRORS = [
        (400, "/badrequest"),
        (401, "/unauthorized"),
        (403, "/forbidden"),
        (404, "/notfound"),
        (405, "/tests/post_result.txt", "POST"),
        (413, "/tests/post_result.txt", "POST", "A" * (1024*1024*2)),
        (500, "/cause500"),
    ]
    for err in ERRORS:
        code = err[0]
        path = err[1]
        method = err[2] if len(err) > 2 else "GET"
        data = err[3] if len(err) > 3 else None
        url = url_base + path
        try:
            if method == "POST":
                resp = requests.post(url, data=data)
            else:
                resp = requests.get(url)
            log(f"[Error {code}] {url} => {resp.status_code}")
            if resp.status_code == code:
                log(f"  -> OK ({code})")
            else:
                log(f"  -> FAIL (expected {code}, got {resp.status_code})")
        except Exception as e:
            log(f"  -> ERROR: {e}")

def test_body_size(log, url, max_body_size=1024*1024):
    for size, expected in [
        (max_body_size - 1, 200),
        (max_body_size, 200),
        (max_body_size + 1, 413),
    ]:
        body = "A" * size
        try:
            resp = requests.post(url, data=body)
            log(f"[BodySize] {size} bytes => {resp.status_code}")
            if resp.status_code == expected:
                log(f"  -> OK ({expected})")
            else:
                log(f"  -> FAIL (expected {expected}, got {resp.status_code})")
        except Exception as e:
            log(f"  -> ERROR: {e}")

def test_cookies(log, url):
    s = requests.Session()
    try:
        for i in range(3):
            resp = s.get(url)
            log(f"[Cookie] Visit {i+1}: {resp.text.strip()}")
    except Exception as e:
        log(f"  -> ERROR: {e}")

def test_fragmented_request(log, host, port):
    try:
        s = socket.socket()
        s.connect((host, port))
        s.send(b"POST / HTTP/1.1\r\nHost: localhost:8081\r\nContent-Length: 13\r\n\r\n")
        time.sleep(0.5)
        s.send(b"test=fragment")
        resp = s.recv(4096).decode()
        log(f"[Fragmented] Response: {resp[:200]}")
        s.close()
    except Exception as e:
        log(f"  -> ERROR: {e}")

def test_timeout(log, host, port, timeout=10):
    try:
        s = socket.socket()
        s.settimeout(timeout+5)
        s.connect((host, port))
        s.send(b"GET / HTTP/1.1\r\nHost: localhost\r\n\r\n")
        log("[Timeout] Sent partial request, waiting for timeout...")
        start = time.time()
        try:
            data = s.recv(1024)
            log(f"[Timeout] Received: {data}")
        except socket.timeout:
            log("[Timeout] No response (timeout as expected)")
        elapsed = time.time() - start
        log(f"[Timeout] Elapsed: {elapsed:.2f}s")
        s.close()
    except Exception as e:
        log(f"  -> ERROR: {e}")

def test_stress(log, url, threads=5, reqs=10):
    def worker(tid):
        for i in range(reqs):
            try:
                resp = requests.get(url)
                log(f"[Stress][T{tid}] Req {i+1}: {resp.status_code}")
            except Exception as e:
                log(f"[Stress][T{tid}] Req {i+1}: ERROR {e}")
    thread_list = []
    for t in range(threads):
        th = threading.Thread(target=worker, args=(t,))
        thread_list.append(th)
        th.start()
    for th in thread_list:
        th.join()
    log("[Stress] Done.")

def test_post_delete(log, url):
    try:
        resp = requests.post(url, data="Ceci est un test POST")
        log(f"[POST] {resp.status_code}: {resp.text.strip()}")
        resp = requests.delete(url)
        log(f"[DELETE] {resp.status_code}: {resp.text.strip()}")
    except Exception as e:
        log(f"  -> ERROR: {e}")

# --- GUI ---
class AllInOneTester:
    def __init__(self, root):
        self.root = root
        root.title("Webserv All-In-One Tester")
        self.url_var = tk.StringVar(value="http://localhost:8081")
        self.port_var = tk.IntVar(value=8081)
        self.body_url_var = tk.StringVar(value="http://localhost:8081")
        self.stress_threads = tk.IntVar(value=5)
        self.stress_reqs = tk.IntVar(value=10)
        self.max_body_size = tk.IntVar(value=1024*1024)
        self.create_widgets()

    def create_widgets(self):
        nb = ttk.Notebook(self.root)
        nb.pack(fill=tk.BOTH, expand=True)
        self.log_text = scrolledtext.ScrolledText(self.root, width=100, height=25)
        self.log_text.pack(fill=tk.BOTH, expand=True)
        def log(msg):
            self.log_text.insert(tk.END, str(msg)+"\n")
            self.log_text.see(tk.END)
        self.log = log

        # Error Pages Tab
        f1 = ttk.Frame(nb)
        ttk.Label(f1, text="Base URL:").pack()
        url_entry = ttk.Entry(f1, textvariable=self.url_var, width=50)
        url_entry.pack()
        ttk.Button(f1, text="Test Error Pages", command=lambda: threading.Thread(target=test_error_pages, args=(self.log, self.url_var.get())).start()).pack(pady=5)
        nb.add(f1, text="Error Pages")

        # Body Size Tab
        f2 = ttk.Frame(nb)
        ttk.Label(f2, text="POST URL:").pack()
        body_url_entry = ttk.Entry(f2, textvariable=self.body_url_var, width=50)
        body_url_entry.pack()
        ttk.Label(f2, text="Max Body Size (bytes):").pack()
        max_body_entry = ttk.Entry(f2, textvariable=self.max_body_size, width=20)
        max_body_entry.pack()
        ttk.Button(f2, text="Test Body Size", command=lambda: threading.Thread(target=test_body_size, args=(self.log, self.body_url_var.get(), self.max_body_size.get())).start()).pack(pady=5)
        nb.add(f2, text="Body Size")

        # Cookies Tab
        f3 = ttk.Frame(nb)
        ttk.Label(f3, text="Base URL:").pack()
        cookie_url_entry = ttk.Entry(f3, textvariable=self.url_var, width=50)
        cookie_url_entry.pack()
        ttk.Button(f3, text="Test Cookies", command=lambda: threading.Thread(target=test_cookies, args=(self.log, self.url_var.get())).start()).pack(pady=5)
        nb.add(f3, text="Cookies")

        # Fragmented Request Tab
        f4 = ttk.Frame(nb)
        ttk.Label(f4, text="Host:").pack()
        host_entry = ttk.Entry(f4, width=30)
        host_entry.insert(0, "localhost")
        host_entry.pack()
        ttk.Label(f4, text="Port:").pack()
        port_entry = ttk.Entry(f4, textvariable=self.port_var, width=10)
        port_entry.pack()
        ttk.Button(f4, text="Test Fragmented Request", command=lambda: threading.Thread(target=test_fragmented_request, args=(self.log, host_entry.get(), self.port_var.get())).start()).pack(pady=5)
        nb.add(f4, text="Fragmented")

        # Timeout Tab
        f5 = ttk.Frame(nb)
        ttk.Label(f5, text="Host:").pack()
        host2_entry = ttk.Entry(f5, width=30)
        host2_entry.insert(0, "localhost")
        host2_entry.pack()
        ttk.Label(f5, text="Port:").pack()
        port2_entry = ttk.Entry(f5, textvariable=self.port_var, width=10)
        port2_entry.pack()
        ttk.Button(f5, text="Test Timeout", command=lambda: threading.Thread(target=test_timeout, args=(self.log, host2_entry.get(), self.port_var.get())).start()).pack(pady=5)
        nb.add(f5, text="Timeout")

        # Stress Tab
        f6 = ttk.Frame(nb)
        ttk.Label(f6, text="URL:").pack()
        stress_url_entry = ttk.Entry(f6, textvariable=self.url_var, width=50)
        stress_url_entry.pack()
        ttk.Label(f6, text="Threads:").pack()
        threads_entry = ttk.Entry(f6, textvariable=self.stress_threads, width=10)
        threads_entry.pack()
        ttk.Label(f6, text="Requests per Thread:").pack()
        reqs_entry = ttk.Entry(f6, textvariable=self.stress_reqs, width=10)
        reqs_entry.pack()
        ttk.Button(f6, text="Test Stress", command=lambda: threading.Thread(target=test_stress, args=(self.log, self.url_var.get(), self.stress_threads.get(), self.stress_reqs.get())).start()).pack(pady=5)
        nb.add(f6, text="Stress")

        # POST/DELETE Tab
        f7 = ttk.Frame(nb)
        ttk.Label(f7, text="POST/DELETE URL:").pack()
        pd_url_var = tk.StringVar(value="http://localhost:8081/tests/post_result.txt")
        pd_url_entry = ttk.Entry(f7, textvariable=pd_url_var, width=50)
        pd_url_entry.pack()
        ttk.Button(f7, text="Test POST/DELETE", command=lambda: threading.Thread(target=test_post_delete, args=(self.log, pd_url_var.get())).start()).pack(pady=5)
        nb.add(f7, text="POST/DELETE")

if __name__ == "__main__":
    root = tk.Tk()
    app = AllInOneTester(root)
    root.mainloop()
