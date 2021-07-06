---
layout: default
title: Utilisation
parent: Bouée
nav_order: 4
---

# Utilisation

## Prérequis

* Charger la batterie
* Insérer une carte SIM
* Fermer la boite et vérifier l'étanchéité
* Monter les bras flotteurs et la quille
* Vérifier que le Dashboard [Grafana](https://grafana.centipede.fr/d/V35NQ1rGk/bouee-gnss-lienss-inrae?orgId=1) est accessible et fonctionnel

## Démarage de la bouée

* Retirer l'aimant sur le côté de la boite afin de mettre sous tension.
* attendre 3-4 min le temps que les services demarrent.
* Vérifier sur [Grafana](https://grafana.centipede.fr/d/V35NQ1rGk/bouee-gnss-lienss-inrae?orgId=1) que les données s'affichent.

## Si pas de données

* Configurer et activer son smartphone en [point d'accès Wifi](https://support.google.com/android/answer/9059108?hl=fr) pour accéder à l'interface web de paramétrage de la bouée
  * Nom du point d'accès: **BUOY**
  * Sécurité: **WPA2 PSK**
  * Mot de passe du point d'accès: **12345678**
  * Activer
  * La bouée va se connecter au wifi de votre smartphone
  * activer **options avancées** du module **point d'accès wifi**
  * Cliquer sur **utilisateurs connectés**
  * Retenir l'adresse IP de la bouée (de type 192.168.43.15), si plusieurs bouées alors plusieurs IP.
* Ouvrir un navigateur internet sur votre smartphone (chrome, firefox,...)
* Rentrer l'adresse IP ou ```http://basegnss.local``` (si une seule bouée)
* Vérifier:
  * Qu'il ait des barres sur le graphique et une position dans la carte, dans ce cas le module GNSS fonctionne.
  * Que le statut soit **float** ou **fix** et que le l'**age** soit supérieur à 0, dans ce cas la connexion WEB via la carte SIM est OK.
  * Dans l'onglet **Settings**, vérifier les **Options** de connection du **Send service** sont bon et correspondent bien au serveur central
  * Enfin arrêter, attendre quelques secondes et redémarer le **Send service** ([bug](https://github.com/jancelin/physalia/issues/1))
  * Vérifier enfin sur [Grafana](https://grafana.centipede.fr/d/V35NQ1rGk/bouee-gnss-lienss-inrae?orgId=1) que les données arrivent normalement (rafraichissement toute les 5 secondes).
