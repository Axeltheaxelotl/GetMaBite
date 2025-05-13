<h1 align="center">server_name multiples</h1>
<p>
Le suppor des server_name multiples permet a un serveur HTTP d'associer plusieurs noms d'hote<br>
a une meme configuration de serveur. Cela est utile pour gerer plusieurs domaines ou sous-domaines<br>
avec une seule instance serveur.<br>

---

<b>Declaration des server_name dans .conf :</b><br>
<p>

```sh
server {
    listen 8081;
    server_name example.com www.exemple.com;
    root ./www/tests/;
    index index.html;
}
```
le serveur ecoute sur le port 8081 et repond pour le nom d hote