#include  <stdio.h>
#include  <unistd.h>
#include  <stdlib.h>
#include  <sys/socket.h>
#include  <netdb.h>
#include  <string.h>
#include  <stdbool.h>

// Socket : Flux de données, permettant à des machines locales ou distantes de communiquer entre elles via des protocoles.
// addrinfo : getaddrinfo combine les possibilités offertes par les fonctions getservbyname() et getservbyport(), ainbsi que l'adresse internet
// Permet aussi éliminer les dépendances IPv4/IPv6
// Pipe -> Buffer
// The SOCKADDR_STORAGE structure stores socket address information
// socklen_t : un type intégral opaque non signé d'une longueur d'au moins 32 bits.

#define SERVADDR "localhost"        // Définition de l'adresse IP d'écoute
#define SERVPORT "0"                // Définition du port d'écoute, si 0 port choisi dynamiquement
#define LISTENLEN 1                 // Taille du tampon de demande de connexion
#define MAXBUFFERLEN 1024			// Taille maximum du Buffer
#define MAXHOSTLEN 64				// Taille maximum de l'IP
#define MAXPORTLEN 6				// Taille maximum du port

int main(void){
	int ecode;                       // Code de retour des fonctions
	char serverAddr[MAXHOSTLEN];     // Adresse du serveur
	char serverPort[MAXPORTLEN];     // Port du server
	int descSockRDV;                 // Descripteur de socket de rendez-vous
	int descSockCOM;                 // Descripteur de socket de communication
	int descSockSERV;                // Descripteur de socket de serveur
	int descSockDATASERV;            // Descripteur de socket de data serveur
	int descSockDATACLIENT;          // Descripteur de socket de data client
	struct addrinfo hints;           // Contrôle la fonction getaddrinfo
	struct addrinfo *res,*resPtr;    // Contient le résultat de la fonction getaddrinfo
	/* Pour getaddrinfo : 
		ai_family
		    Ce champ indique la famille d'adresse désirée des adresses renvoyées. Les valeurs valides de ce champ inclus AF_INET et AF_INET6. La valeur AF_UNSPEC indique que getaddrinfo() doit renvoyer les adresses de socket de n'importe quelle famille d'adresses (par exemple, IPv4 ou IPv6) pouvant être utilisées avec node et service.
		ai_socktype
		    Ce champ indique le type préféré de socket, par exemple SOCK_STREAM ou SOCK_DGRAM. Mettre 0 dans ce champ indique que getaddrinfo() peut renvoyer n'importe quel type d'adresses de socket.
		ai_protocol
		    Ce champ indique le protocole des adresses de socket renvoyées. Mettre 0 dans ce champ indique que getaddrinfo() peut renvoyer des adresses de socket de n'importe quel type.
		ai_flags
		    Ce champ indique des options supplémentaires, décrites plus loin. Divers attributs sont regroupés par un OU binaire. */
	struct sockaddr_storage myinfo;  // Informations sur la connexion de RDV
	struct sockaddr_storage from;    // Informations sur le client connecté
	socklen_t len;                   // Variable utilisée pour stocker les longueurs des structures de socket
	char buffer[MAXBUFFERLEN];       // Tampon de communication entre le client et le serveur
	bool isConnected;				 // Contrôle l'état de la connexion


    /* --- Création du serveur proxy --- */
	// Publication du socket au niveau du système, assignation d'une adresse IP et d'un numéro de port et mise à zéro de hints
	memset(&hints, 0, sizeof(hints));

	// Initialisation de hints (paramètres pour les connexions)
	hints.ai_flags = AI_PASSIVE;      	// Mode serveur, nous allons utiliser la fonction bind
	hints.ai_socktype = SOCK_STREAM;  	// Mode de connexion : TCP
	hints.ai_family = AF_INET;			// Traitement IPv4

	// Récupération des informations du serveur proxy
	ecode = getaddrinfo(SERVADDR, SERVPORT, &hints, &res);
	if (ecode) {
		fprintf(stderr,"getaddrinfo: %s\n", gai_strerror(ecode));
		exit(1);
	}

	// Création du socket IPv4/TCP
	descSockRDV = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (descSockRDV == -1) {
		perror("Erreur lors de la creation du socket");
		exit(4);
	}

	// Publication du socket
	// bind Fournir un nom à une socket
	ecode = bind(descSockRDV, res->ai_addr, res->ai_addrlen);
	if (ecode == -1) {
		perror("Erreur lors de la liaison du socket de RDV");
		exit(3);
	}

	// On libère addrinfo pour de prochains calculs
	freeaddrinfo(res);

	// Récupération du nom de la machine et du numéro de port pour affichage à l'écran
	len=sizeof(struct sockaddr_storage);
	ecode=getsockname(descSockRDV, (struct sockaddr *) &myinfo, &len);
	if (ecode == -1) {
		perror("Nom serveur : getsockname");
		exit(4);
	}
	ecode = getnameinfo((struct sockaddr*)&myinfo, sizeof(myinfo), serverAddr,MAXHOSTLEN, serverPort, MAXPORTLEN, NI_NUMERICHOST | NI_NUMERICSERV);
	if (ecode != 0) {
	// stderr : ce dernier flux est associé à la sortie standard d'erreur de votre application. Tout comme stdout, ce flux est normalement redirigé sur la console de l'application.
	// The gai_strerror() function shall return a text string describing an error value for the getaddrinfo() and getnameinfo() functions listed in the <netdb.h> header.
		fprintf(stderr, "Erreur dans getnameinfo : %s\n", gai_strerror(ecode));
		exit(4);
	}

	// Affichage de l'adresse et du port d'écoute pour se connecter
	printf("L'adresse d'écoute est : %s\n", serverAddr);
	printf("Le port d'écoute est : %s\n", serverPort);

	// Définition de la taille du tampon contenant les demandes de connexion
	// listen - Attendre des connexions sur une socket
	ecode = listen(descSockRDV, LISTENLEN);
	if (ecode == -1) {
		perror("Erreur initialisation buffer d'écoute");
		exit(5);
	}

	len = sizeof(struct sockaddr_storage);

	// Attente connexion du client
	// Lors d'une connexion, creation d'une socket de communication avec le client
	descSockCOM = accept(descSockRDV, (struct sockaddr *) &from, &len);
	if (descSockCOM == -1){
		perror("Erreur d'acceptation\n");
		exit(6);
	}


	/* --- Connexion du client --- */
	// Message de confirmation de connexion
	// strcpy : copier le contenu d'une chaîne de caractères dans une autre
	strcpy(buffer, "220 Connexion établie\r\n");
	write(descSockCOM, buffer, strlen(buffer));

	// Récupération des informations de connexion
	ecode = read(descSockCOM, buffer, MAXBUFFERLEN);
	if (ecode == -1) {
		perror("Problème de lecture\n");
		exit(7);
	}
	buffer[ecode] = '\0';

	// Extraction du login et du serveur
	char nomlogin [MAXBUFFERLEN];
	char nomserveur [MAXBUFFERLEN];

	// Séparation des commandes entrées par le client
	// Pour lire pseudo@server : "%[^@]@%s"
	sscanf(buffer,"%[^@]@%s", nomlogin, nomserveur);
	printf("%s\n", nomlogin);
	printf("%s\n", nomserveur);


	/* --- Connexion au serveur distant --- */
	// Connexion au serveur
	// 21 = le port ftp
	ecode = getaddrinfo(nomserveur, "21", &hints, &res);
	if (ecode) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ecode));
		exit(1);
	}

	// Indique si on est bien connecté au serveur
	isConnected = false;
	resPtr = res;

	// Boucle de connexion
	while(!isConnected && resPtr!=NULL) {
		//Création du socket IPv4/TCP
		descSockSERV = socket(resPtr->ai_family, resPtr->ai_socktype, resPtr->ai_protocol);
		if (descSockSERV == -1) {
			perror("Erreur lors de la creation du socket");
			exit(2);
		}

  		//Connexion au serveur
		ecode = connect(descSockSERV, resPtr->ai_addr, resPtr->ai_addrlen);
		if (ecode == -1) {
			resPtr = resPtr->ai_next;
			close(descSockSERV);
		}
		// Si il n'y a pas d'erreur, changer la valeur du booleen
		else isConnected = true;
	}

	// On libère addrinfo pour de prochains calculs
	freeaddrinfo(res);

	// Message d'erreur si aucune connexion
	if (!isConnected) {
		perror("Connexion impossible");
		exit(2);
	}

	// Réponse du serveur
	ecode = read(descSockSERV, buffer, MAXBUFFERLEN);
	if (ecode == -1) {
		perror("Problème de lecture\n");
		exit(3);
	}
	buffer[ecode] = '\0';

	// Envoi du login au serveur
	// On ajoute "\r\n" pour terminer la phrase
	strcat(nomlogin, "\r\n");
	write(descSockSERV, nomlogin, strlen(nomlogin));

	// Lecture du retour du serveur
	ecode = read(descSockSERV, buffer, MAXBUFFERLEN);
	if (ecode == -1) {
		perror("Problème de lecture\n");
		exit(3);
	}

	// Afficher le contenu du buffer dans la console
	buffer[ecode] = '\0';
	printf("%s\n", buffer);

	// Transfert des données du serveur au client
	write(descSockCOM, buffer, strlen(buffer));

	// Récupération de la réponse du client
	ecode = read(descSockCOM, buffer, MAXBUFFERLEN);
	if (ecode == -1) {
		perror("Problème de lecture\n");
		exit(3);
	}
	buffer[ecode] = '\0';

	// Transfert de la réponse au serveur
	write(descSockSERV, buffer, strlen(buffer));

	// Lecture du retour du serveur
	ecode = read(descSockSERV, buffer, MAXBUFFERLEN);
	if (ecode == -1) {
		perror("Problème de lecture\n");
		exit(3);
	}

	// Afficher le contenu du buffer dans la console
	buffer[ecode] = '\0';
	printf("%s\n", buffer);

	// Commandes automatiques (SYST et PORT)
	// Transfert des données du serveur au client
	write(descSockCOM, buffer, strlen(buffer));

	// Lecture de la commande automatique
	ecode = read(descSockCOM, buffer, MAXBUFFERLEN);
	if (ecode == -1) {
		perror("Problème de lecture\n");
		exit(3);
	}

	// Afficher le contenu du buffer dans la console
	buffer[ecode] = '\0';
	printf("%s\n", buffer);

	// Envoi de la commande SYST
	write(descSockSERV, buffer, strlen(buffer));

	// Lecture de la réponse
	ecode = read(descSockSERV, buffer, MAXBUFFERLEN);
	if (ecode == -1) {
		perror("Problème de lecture\n");
		exit(3);
	}

	// Afficher le contenu du buffer dans la console
	buffer[ecode] = '\0';
	printf("%s\n", buffer);

	// Transfert des données du serveur au client
	write(descSockCOM, buffer, strlen(buffer));

	// Lecture de la commande PORT
	ecode = read(descSockCOM, buffer, MAXBUFFERLEN);
	if (ecode == -1) {
		perror("Problème de lecture\n");
		exit(3);
	}

	// Afficher le contenu du buffer dans la console
	buffer[ecode] = '\0';
	printf("%s\n", buffer);

	// Transfert de données
	while (strncmp(buffer, "QUIT", 4) != 0) {
		if (strncmp(buffer, "PORT", 4) != 0) {
			// Transfert de la commande au serveur
			write(descSockSERV, buffer, strlen(buffer));

			// Lecture de la réponse du serveur
			ecode = read(descSockSERV, buffer, MAXBUFFERLEN);
			if (ecode == -1) {
				perror("Problème de lecture\n");
				exit(3);
			}
			buffer[ecode] = '\0';

			// Transfert la réponse au client
			write(descSockCOM, buffer, strlen(buffer));
		} else {
			// Extraction de l'adresse et du port client
			int ic1, ic2, ic3, ic4, pc1, pc2;
			//The C library function int sscanf(const char *str, const char *format, ...) reads formatted input from a string.
			sscanf(buffer, "PORT %d,%d,%d,%d,%d,%d", &ic1, &ic2, &ic3, &ic4, &pc1, &pc2);

			// Récupération de l'adresse et du port client
			char adresseClient[MAXBUFFERLEN];
			char portClient[MAXBUFFERLEN];
			sprintf(adresseClient, "%d.%d.%d.%d", ic1, ic2, ic3, ic4);
			sprintf(portClient, "%d", pc1*256 + pc2);

			// Affichage des informations client dans la console
			printf("%s\n", adresseClient);
			printf("%s\n", portClient);

			// Envoi de la commande PASV
			write(descSockSERV, "PASV\r\n", 6);

			// Récupération des informations du serveur
			ecode = read(descSockSERV, buffer, MAXBUFFERLEN);
			if (ecode == -1) {
				perror("Problème de lecture\n");
				exit(3);
			}

			// Afficher le contenu du buffer dans la console
			buffer[ecode] = '\0';
			printf("%s\n", buffer);

			// Extraction de l'adresse et du port du serveur
			int is1, is2, is3, is4, ps1, ps2;
			sscanf(buffer, "%*[^(](%d,%d,%d,%d,%d,%d", &is1, &is2, &is3, &is4, &ps1, &ps2);

			// Récupération de l'adresse et du port du serveur
			char adresseServeur[MAXBUFFERLEN];
			char portServeur[MAXBUFFERLEN];
			sprintf(adresseServeur, "%d.%d.%d.%d", is1, is2, is3, is4);
			sprintf(portServeur, "%d", ps1*256 + ps2);

			// Affichage des informations du serveur dans la console
			printf("%s\n", adresseServeur);
			printf("%s\n", portServeur);

			// Confirmation de la commande PORT
			write(descSockCOM, "200 PORT commande réussie\r\n", strlen("200 PORT commande réussie\r\n"));

			// Lecture de la commande LIST
			ecode = read(descSockCOM, buffer, MAXBUFFERLEN);
			if (ecode == -1) {
				perror("Problème de lecture\n");
				exit(3);
			}

			// Afficher le contenu du buffer dans la console
			buffer[ecode] = '\0';
			printf("%s\n", buffer);

			// Envoi de la commande LIST au serveur
			write(descSockSERV, buffer, strlen(buffer));

			// Ouverture de la connexion de données coté serveur
			ecode = getaddrinfo(adresseServeur, portServeur, &hints, &res);
			if (ecode) {
				fprintf(stderr,"getaddrinfo: %s\n", gai_strerror(ecode));
				exit(1);
			}

			// Indique si on est bien connecté au serveur
			isConnected = false;
			resPtr = res;

			// Boucle de connexion
			while(!isConnected && resPtr!=NULL){
				//Création du socket IPv4/TCP
				descSockDATASERV = socket(resPtr->ai_family, resPtr->ai_socktype, resPtr->ai_protocol);
				if (descSockDATASERV == -1) {
					perror("Erreur lors de la creation du socket");
					exit(2);
				}

				//Connexion au serveur
				ecode = connect(descSockDATASERV, resPtr->ai_addr, resPtr->ai_addrlen);
				if (ecode == -1) {
					resPtr = resPtr->ai_next;
					close(descSockDATASERV);
				}

				// Si il n'y a pas d'erreur, changer la valeur du booleen
				else isConnected = true;
			}

			// On libère addrinfo pour de prochains calculs
			freeaddrinfo(res);

			// Message d'erreur si aucune connexion
			if (!isConnected) {
				perror("Connexion impossible");
				exit(2);
			}

			// Ouverture de la connexion de données coté client
			ecode = getaddrinfo(adresseClient, portClient, &hints, &res);
			if (ecode) {
				fprintf(stderr,"getaddrinfo: %s\n", gai_strerror(ecode));
				exit(1);
			}

			// Indique si on est bien connecté au serveur
			isConnected = false;
			resPtr = res;

			// Boucle de connexion
			while(!isConnected && resPtr!=NULL){
				//Création du socket IPv4/TCP
				descSockDATACLIENT = socket(resPtr->ai_family, resPtr->ai_socktype, resPtr->ai_protocol);
				if (descSockDATACLIENT == -1) {
					perror("Erreur lors de la creation du socket");
					exit(2);
				}

				//Connexion au serveur
				ecode = connect(descSockDATACLIENT, resPtr->ai_addr, resPtr->ai_addrlen);
				if (ecode == -1) {
					resPtr = resPtr->ai_next;
				close(descSockDATACLIENT);
				}

				// Si il n'y a pas d'erreur, changer la valeur du booleen
				else isConnected = true;
			}

			// On libère addrinfo pour de prochains calculs
			freeaddrinfo(res);

			// Message d'erreur si aucune connexion
			if (!isConnected) {
				perror("Connexion impossible");
				exit(2);
			}

			// Récupération de la réponse du serveur ftp
			ecode = read(descSockSERV, buffer, MAXBUFFERLEN);
			if (ecode == -1) {
				perror("Problème de lecture\n");
				exit(3);
			}

			// Afficher le contenu du buffer dans la console
			buffer[ecode] = '\0';
			printf("%s\n", buffer);

			// Transfert du résultat au client
			write(descSockCOM, buffer, strlen(buffer));

			// Récupération du résultat de la commande
			ecode = read(descSockDATASERV, buffer, MAXBUFFERLEN);
			if (ecode == -1) {
				perror("Problème de lecture\n");
				exit(3);
			}

			// Afficher le contenu du buffer dans la console
			buffer[ecode] = '\0';
			printf("%s\n", buffer);

			// Boucle de transmission de l'information
			while(ecode != 0) {
				// Transfert du résultat au client
				write(descSockDATACLIENT, buffer, strlen(buffer));

				// Récupération du résultat de la commande
				ecode = read(descSockDATASERV, buffer, MAXBUFFERLEN);
				if (ecode == -1) {
					perror("Problème de lecture\n");
					exit(3);
				}

				// Afficher le contenu du buffer dans la console
				buffer[ecode] = '\0';
				printf("%s\n", buffer);
			}

			// Fermeture des sockets de data
			close(descSockDATACLIENT);
			close(descSockDATASERV);

			// Récupération de la réponse de transfert
			ecode = read(descSockSERV, buffer, MAXBUFFERLEN);
			if (ecode == -1) {
				perror("Problème de lecture\n");
				exit(3);
			}

			// Afficher le contenu du buffer dans la console
			buffer[ecode] = '\0';
			printf("%s\n", buffer);

			// Transfert de la réponse
			write(descSockCOM, buffer, strlen(buffer));
		}

		// Lecture de commande client
		ecode = read(descSockCOM, buffer, MAXBUFFERLEN);
		if (ecode == -1) {
			perror("Problème de lecture\n");
			exit(3);
		}

		// Afficher le contenu du buffer dans la console
		buffer[ecode] = '\0';
		printf("%s\n", buffer);
	}

	//Fermeture de la connexion
	close(descSockCOM);
	close(descSockRDV);
	close(descSockSERV);
	close(descSockDATASERV);
	close(descSockDATACLIENT);
}
