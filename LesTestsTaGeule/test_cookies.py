from flask import Flask, request, make_response
import tkinter as tk
from tkinter import scrolledtext
import requests
import os

app = Flask(__name__)

@app.route('/')
def index():
    # Vérifie si un cookie existe
    visit_count = request.cookies.get('visit_count', '0')
    visit_count = int(visit_count) + 1

    # Crée une réponse avec un cookie mis à jour
    response = make_response(f"nb visistes : {visit_count}")
    response.set_cookie('visit_count', str(visit_count), max_age=3600)
    return response

@app.route('/tests/post_result.txt', methods=['POST', 'DELETE'])
def handle_post_delete():
    file_path = './LesTestsTaGeule/post_result.txt'
    directory = os.path.dirname(file_path)

    # Vérifie si le répertoire existe, sinon le crée
    if not os.path.exists(directory):
        os.makedirs(directory)

    if request.method == 'POST':
        # Écrit le contenu dans le fichier
        content = request.data.decode('utf-8')
        with open(file_path, 'w') as f:
            f.write(content)
        return f"Contenu écrit : {content}", 200

    elif request.method == 'DELETE':
        # Supprime le fichier s'il existe
        if os.path.exists(file_path):
            os.remove(file_path)
            return "Fichier supprimé.", 200
        else:
            return "Fichier introuvable.", 404

class CookieTesterApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Test des Cookies et Méthodes HTTP")

        # Zone de logs
        self.log_label = tk.Label(root, text="Logs :")
        self.log_label.pack()

        self.log_text = scrolledtext.ScrolledText(root, width=80, height=20)
        self.log_text.pack()

        # Boutons pour tester les cookies et les méthodes HTTP
        self.cookie_button = tk.Button(root, text="Tester les Cookies", command=self.test_cookies)
        self.cookie_button.pack(pady=5)

        self.delete_button = tk.Button(root, text="Tester DELETE", command=self.test_delete)
        self.delete_button.pack(pady=5)

        self.post_button = tk.Button(root, text="Tester POST", command=self.test_post)
        self.post_button.pack(pady=5)

    def log(self, message):
        self.log_text.insert(tk.END, message + "\n")
        self.log_text.see(tk.END)

    def test_cookies(self):
        base_url = "http://localhost:8081"
        try:
            # Étape 1 : Envoyer une requête pour définir un cookie
            response = requests.get(base_url)
            self.log(f"Response Headers (Set-Cookie): {response.headers.get('Set-Cookie')}")

            # Étape 2 : Extraire les cookies de la réponse
            cookies = response.cookies
            self.log(f"Cookies reçus : {cookies}")

            # Étape 3 : Envoyer une autre requête avec les cookies
            response = requests.get(base_url, cookies=cookies)
            self.log(f"Response with cookies: {response.text}")
        except Exception as e:
            self.log(f"Erreur : {e}")

    def test_delete(self):
        url = "http://localhost:8081/tests/post_result.txt"
        try:
            response = requests.delete(url)
            self.log(f"DELETE Response: {response.status_code} {response.text}")
        except Exception as e:
            self.log(f"Erreur : {e}")

    def test_post(self):
        url = "http://localhost:8081/tests/post_result.txt"
        data = "Ceci est un test POST"
        try:
            response = requests.post(url, data=data)
            self.log(f"POST Response: {response.status_code} {response.text}")
        except Exception as e:
            self.log(f"Erreur : {e}")

if __name__ == "__main__":
    # Lancer le serveur Flask dans un thread séparé
    import threading
    threading.Thread(target=lambda: app.run(debug=True, port=8081, use_reloader=False)).start()

    # Lancer l'interface graphique
    root = tk.Tk()
    app = CookieTesterApp(root)
    root.mainloop()
