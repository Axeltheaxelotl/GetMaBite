import subprocess
import threading
import requests
import tkinter as tk
from tkinter import scrolledtext
import signal
import sys

# Fonction pour envoyer des requêtes au serveur
def envoyer_requetes(url, nb_threads, nb_requetes_par_thread, zone_logs):
    def travail(thread_id):
        for i in range(nb_requetes_par_thread):
            try:
                # Envoi de la requête GET
                reponse = requests.get(url)
                # Affichage du statut de la requête dans la zone de logs
                zone_logs.insert(tk.END, f"Thread {thread_id}, Requête {i + 1}: {reponse.status_code}\n")
                zone_logs.see(tk.END)
            except Exception as e:
                # Gestion des erreurs et affichage dans la zone de logs
                zone_logs.insert(tk.END, f"Thread {thread_id}, Requête {i + 1}: Erreur - {e}\n")
                zone_logs.see(tk.END)

    # Création et démarrage des threads
    threads = []
    for thread_id in range(nb_threads):
        thread = threading.Thread(target=travail, args=(thread_id,))
        threads.append(thread)
        thread.start()

    # Attente de la fin de tous les threads
    for thread in threads:
        thread.join()

def run_command_with_logs(command, log_text):
    process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, shell=True)

    def read_output(pipe, tag):
        for line in iter(pipe.readline, ''):
            log_text.insert(tk.END, f"[{tag}] {line}")
            log_text.see(tk.END)
        pipe.close()

    threading.Thread(target=read_output, args=(process.stdout, "STDOUT")).start()
    threading.Thread(target=read_output, args=(process.stderr, "STDERR")).start()

    process.wait()
    log_text.insert(tk.END, f"\n[INFO] Command finished with exit code {process.returncode}\n")
    log_text.see(tk.END)

# Classe principale pour l'application de test de stress
class ApplicationStressTest:
    def __init__(self, fenetre):
        self.fenetre = fenetre
        self.fenetre.title("Test de Stress")
        self.fenetre.configure(bg="#ffffff")  # Couleur de fond de la fenêtre

        # Éléments de l'interface
        self.label_logs = tk.Label(fenetre, text="Logs :", bg="#f0f0f0", fg="#333333", font=("Arial", 12, "bold"))
        self.label_logs.pack(pady=5)

        self.zone_logs = scrolledtext.ScrolledText(fenetre, width=80, height=20, bg="#ffffff", fg="#000000", font=("Courier", 10))
        self.zone_logs.pack(pady=5)

        self.bouton_demarrer = tk.Button(fenetre, text="Démarrer le Test", command=self.demarrer_test, bg="#4CAF50", fg="#ffffff", font=("Arial", 12, "bold"))
        self.bouton_demarrer.pack(pady=10)

        self.label_status = tk.Label(fenetre, text="Statut : Prêt", bg="#f0f0f0", fg="#333333", font=("Arial", 12, "italic"))
        self.label_status.pack(pady=5)

        self.command_entry = tk.Entry(fenetre, width=80)
        self.command_entry.pack(pady=5)
        self.command_entry.insert(0, "./webserv config.conf")

        self.run_button = tk.Button(fenetre, text="Run Command", command=self.run_command, bg="#008CBA", fg="#ffffff", font=("Arial", 12, "bold"))
        self.run_button.pack(pady=10)

        # Handle Ctrl+C gracefully
        signal.signal(signal.SIGINT, self.handle_interrupt)

    # Fonction appelée lors du clic sur le bouton "Démarrer le Test"
    def demarrer_test(self):
        self.label_status.config(text="Statut : En cours", fg="#FF5722")  # Mise à jour du statut
        url = "http://localhost:8081"  # URL du serveur à tester
        nb_threads = 10  # Nombre de threads
        nb_requetes_par_thread = 50  # Nombre de requêtes par thread

        # Ajout d'un message dans la zone de logs
        self.zone_logs.insert(tk.END, "Démarrage du test de stress...\n")
        self.zone_logs.see(tk.END)

        # Lancement des requêtes dans un thread séparé pour ne pas bloquer l'interface
        threading.Thread(target=envoyer_requetes, args=(url, nb_threads, nb_requetes_par_thread, self.zone_logs)).start()

    def run_command(self):
        command = self.command_entry.get()
        self.zone_logs.insert(tk.END, f"[INFO] Running command: {command}\n")
        self.zone_logs.see(tk.END)
        threading.Thread(target=run_command_with_logs, args=(command, self.zone_logs)).start()

    def handle_interrupt(self, signum, frame):
        self.zone_logs.insert(tk.END, "\n[INFO] Interruption détectée. Arrêt en cours...\n")
        self.zone_logs.see(tk.END)
        sys.exit(0)

if __name__ == "__main__":
    fenetre = tk.Tk()
    app = ApplicationStressTest(fenetre)
    fenetre.mainloop()
