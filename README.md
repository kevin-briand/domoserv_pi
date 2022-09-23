# domoserv_pi
Gestionnaire chauffage pour raspberry pi

## Cablage
![Domoserv_pi Wiring](https://i.postimg.cc/rpbB0h7N/domoserv-pi-wiring.png)

## INSTALL
Connectez-vous à votre raspberry et entrez les commandes suivantes.
À la fin de l'installation le raspberry redémarrera automatiquement.
```
sudo apt-get install git
git clone https://github.com/firedream89/domoserv_pi.git
cd domoserv_pi
chmod +x INSTALL
sudo ./INSTALL
```

## Configuration
La configuration s'effectue par un import/export du fichier de configuration
Par défaut le gestionnaire chauffage est inactif
L'application peut-être lancé sans argument, cela affichera un menu

Création du fichier de configuration(Config.txt)
```
cd $HOME/domoserv_pi/build
sudo ./domoserv_pi -export
```

Importation du fichier de configuration
```
sudo ./domoserv_pi -import
```
