---
layout: default
title: Serveur
nav_order: 3
has_children: true
---

# Serveur

## Architecture

[architecture serveur](/assets/mindmap/Serveur_data.jpg)

![architecture serveur](/assets/mindmap/Serveur_data.jpg)


## Prérequis

Installer Docker et Docker-compose

```
sudo apt-get update
sudo apt-get install curl 
curl -fsSL https://get.docker.com/ | sh
sudo systemctl enable docker
sudo service docker start
sudo groupadd docker
sudo usermod -aG docker $USER

sudo apt-get install python-pip
sudo pip install docker-compose
````


## Installation des services PHYSALIA server

```
git clone https://github.com/jancelin/physalia.git
cd physalia/server
docker-compose build
docker-compose up -d
```

## Ports

* Postgresql/postgis : 5433
* Pgadmin4: 5051
* grafana: 3000
* buoy GNSS data tracking: 8090

## Login et mot de passe:

https://github.com/jancelin/physalia/blob/main/server/.env


![Grafana](/assets/grafana/grafana2_small.png)

