1) Compilation avec make all.
2) Mettez à jour l'annuaire dans serveurmaitre.h en fonction de l'IP des machines servant de serveur et en fonction
des numéros de ports que vous souhaitez utiliser. Par défaut, les 3 serveurs sont en local avec comme ports 2207, 2208 et 2209.
3) Lancement du serveur maître : si le tableau annuaire est à jour, il ne devrait pas y avoir de problème, le serveur maître vous indique si certains serveurs de l'annuaire ne sont pas disponibles.
4) Lancement du client avec l'IP du serveur maitre en parametre.
5) 
Requêtes disponibles pour le client :
GET <nom du fichier> : Transmets le fichier demandé (s'il existe) du serveur au client.
RM <nom du fichier> : Supprime le fichier donné en argument sur le serveur.
PUT <nom du fichier> : Mets le fichier donné en argument sur le serveur.
LS : Affiche le contenu du répertoire courant du serveur.
BYE : Termine la connexion entre le client et le serveur. 
