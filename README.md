# Proxy
Projet de fin de semestre 3 du DUT Informatique.  
Création d'un proxy en C permettant la communication entre un client et un serveur.

## Fonctionnalités
**Client**  
Le proxy ouvre un port sur la machine et attend la connexion d'un client.  
Une fois cela fait, le proxy demande les informations de connexion au client pour se connecter au serveur.  

**Serveur**  
Une fois les informations reçu de la part du client, le proxy se connecte au serveur.  
Une fois la connexion établie avec le serveur, le proxy se charge de faire la liaison entre le client et le serveur en faisant transiter les informations.
