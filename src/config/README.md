<h1 align="center">server_name multiples</h1>

<h2 align="center">J AI PAS ENCORE TESTER ALOR VOUS POUVAIENT LE FAIRE</h2>

<p>
Le suppor des server_name multiples permet a un serveur HTTP d'associer plusieurs noms d'hote<br>
a une meme configuration de serveur. Cela est utile pour gerer plusieurs domaines ou sous-domaines<br>
avec une seule instance serveur.<br>

---

<b>1. Declaration des server_name dans .conf :</b><br>
<p>

```sh
server {
    listen 8081;
    server_name example.com www.exemple.com;
    root ./www/tests/;
    index index.html;
}
```
le serveur ecoute sur le port 8081 et repond pour le nom d hote example.com et www.example.com<br>

<b>2. Stockage server_name</b><br>
Les server_name sont stockes dans un vecteur de chaines de caracteres dans la classe Server :

```sh
std::vector<std::string> server_names;
```

Chaque serveur peut avoir plusieurs noms d'hote associes.<br>

<b>3. Verification des server_name</b><br>

---

<h3>RESUME:</h3><br>
<p>
1. L'orsqu'une requete HTTP est recue, le serveur extrait le host de l'en-tete Host.<br>
2. La methode findMatchingServer recherche un serveur dont le server_name correspond au host et<br>
dont le port correspondant a celui de la requete.<br>
3. Si un serveur correspondant est trouve il est utilise pour traiter la requete.<br>
4. Sinon, le serveur par default pour le port et utilise.
</p>