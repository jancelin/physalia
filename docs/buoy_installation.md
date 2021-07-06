---
layout: default
title: Installation
parent: Bouée
nav_order: 2
---

## Installation

> Installation avec Raspberry Pi [zero](https://www.kubii.fr/cartes-raspberry-pi/1851-raspberry-pi-zero-w-kubii-3272496006997.html?search_query=zero&results=86)

### 1. mettre à jour le firmware de votre module GNSS F9P

Dans un premier temps il est nécessaire de mettre à jour le [firmware](https://fr.wikipedia.org/wiki/Firmware) de votre module GNSS F9P :

* [Install U-center](https://www.u-blox.com/en/product/u-center) (Windows seulement)

* Update F9P firmware 1.13:
  * [Télécharger le fichier de mise à jour](https://www.u-blox.com/en/ubx-viewer/view/UBX_F9_100_HPG_113_ZED_F9P.7e6e899c5597acddf2f5f2f70fdf5fbe.bin?url=https%3A%2F%2Fwww.u-blox.com%2Fsites%2Fdefault%2Ffiles%2FUBX_F9_100_HPG_113_ZED_F9P.7e6e899c5597acddf2f5f2f70fdf5fbe.bin)
  * Connecter en USB le récepteur F9P au PC
  * Ouvrir U-center en mode administrateur (click droit **Executer en mode administrateur**)
  * Connecter le recepteur (bouton en haut à gauche) au bon port COM
![firware](https://gblobscdn.gitbook.com/assets%2F-LYSZeu4HjB-NrVI4riL%2F-LYbICDde_PqBQRMcCsl%2F-LYbIddBqnC-aXKJ1bxh%2FSans-titre-1.png?alt=media&token=240244db-09d5-40e8-9735-869651b9198e)
  * Vérifier que le récepteur est bien connecté
![firware](https://gblobscdn.gitbook.com/assets%2F-LYSZeu4HjB-NrVI4riL%2F-LYbGvHfj8nIN6gywxBz%2F-LYbHSKTiJZ0j0qAf-5e%2Ficon_blink.png?alt=media&token=0f35cbc4-ce5a-4d3b-90f4-ecadc5a36821)
  * Cliquer sur **Tools** > **Firmware upade...**
    * Choisir le .bin précédement téléchargé dans **Firmware image**
    * cocher **Use this baudrate for update** et choisir 9600
    * Décocher les 4 autres ( Enter safeboot, ...)
  * Cliquer sur **GO** (en bas à gauche de la fenêtre)
![firware](https://gblobscdn.gitbook.com/assets%2F-LYSZeu4HjB-NrVI4riL%2F-LZ5-tu1J0X8sog9Xvkf%2F-LZ527USiWMS3Pjo5SXY%2Fstep4.png?alt=media&token=2e76981e-8874-4151-9c48-f5fa07cdcd69)
  * Attendre la fin de la procédure de mise à jour
![firware](https://gblobscdn.gitbook.com/assets%2F-LYSZeu4HjB-NrVI4riL%2F-LZ52KPCRzypMK4cqtQW%2F-LZ52Z_bl9GHQP8dz7By%2Fstep6.png?alt=media&token=f8f7240b-79b4-4856-87ea-26e12c1aac36)

#### 2. Injection des paramètres

L'injection permet de paramétrer le récepteur F9P pour une utilisation avec RTKlib et plus particulièrement RTKGPS+ sur Android

* Télécharger le [fichier de configuration](https://github.com/jancelin/rtkbase/releases/download/RTKroverGNSS_1_0_0/f9p_ubx_1hz_usb_uart1.txt)

![arduino](https://jancelin.github.io/docs-centipedeRTK/assets/images/montage_rover/u-center.gif)

* Aller dans **Tools** > **Receiver Configuration**
* Sélectionner u-blox Generation 9
* Sélectionner le fichier précédement téléchargé
* Cliquer sur **Transfert file -> GNSS** et attendre que le transfert se réalise

Par mesure de prudence, s'assurer que la configuration est bien enregistrée :
* Cliquer sur **View** > **Configuration View**
* Cliquer sur **CFG (Configaration)**
* Cliquer sur **Send**
* Cliquer sur **Disconnect**
* Débrancher le récepteur

### 3. Télécharger l'image pour Raspberry Pi

Télécharger l'image pour Raspberry Pi de Base RTK sur votre ordinateur (~ 560 Mo) : [BaseGNSS-RPi-2.2.0](https://github.com/jancelin/rtkbase/releases/download/RTKroverGNSS_1_0_0/RTKroverGNSS_1_0_0.zip). 

**Attention**, ne pas copier directement l'image RTKroverGNSS_X.X.X.zip sur la carte micro SD !!!

### 4. Télécharger et installer ETCHER 

Télécharger et installer ETCHER sur votre ordinateur (windows, linux, mac). Ce programme va permettre d'installer correctement l'image BaseRTK téléchargée dans la carte micro SD : <https://etcher.io/>.

### 5. Insérer la carte Micro SD dans l'ordinateur 

   ![SD](https://encrypted-tbn0.gstatic.com/images?q=tbn:ANd9GcRrqS8MhQYdjrRmaYZS-RCtgLIrhB8gdLaxUmAfey96t6YpopQr)

### 6. Flasher la carte SD avec l'image

Démarrer Etcher, choisir l'image téléchargée, la carte SD (normalement déjà sélectionnée) et flasher la carte :

   ![etcher](https://jancelin.github.io/docs-centipedeRTK/assets/images/install/etcher.png)

### 7. Retirer ensuite la carte SD du PC

Vous pouvez retirer la carte SD du PC et l'insérer maintenant dans le Raspberry Pi.

----

### 8. Assemblage et premier démarrage

1. Connecter en UART1 le récepteur F9P sur le TX RX 5v et GND du Raspberry Pi
2. Insérer la carte micro SD dans le Raspberry Pi et le mettre sous tension. 

    > Il est possible de connecter un écran en HDMI sur le raspberry pour visualiser le déroulement de l'installation. 

3. Les leds du raspberry s'allument et/ou clignotent pendant ce premier démarrage (démarrage des services). Quand l'une d'elles s'éteint définitivement (attention à ne pas confondre avec certaines petites coupures) l'installation est terminée (~ 3 min ou plus).
4. Créer un point d'accès Wifi avec votre smartphone comportant le nom **BUOY** et le mot de passe wpa2 **12345678**

5. sur un PC connecté au même réseau wifi ou avec le smartphone, ouvrir un navigateur Internet et accéder à l'interface de la base RTK via l'URL : **<http://basegnss.local>** ou avec l'ip du Raspberry Pi si vous êtes sur un réseau.

![base gnss](https://jancelin.github.io/docs-centipedeRTK/assets/images/basegnss/basegnss.gif)

En cas de problème
{: .label .label-yellow }

Si vous ne voyez pas de position ou de barre sur le graphique c'est que l'initialisation de votre module GNSS s'est mal déroulé, il est conseillé de vérifier les branchements, flasher de nouveau la carte SD et recommencer l'installation.


#### Connexion en ssh pour les développeurs ou le débugage

* identifiant : `ssh basegnss@basegnss.local`
* mot de passe : `basegnss!`

