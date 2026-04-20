---
layout: default
title: Serveur
nav_order: 3
has_children: true
---

# Serveur
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

sudo apt-get install python3-pip
sudo pip3 install docker-compose
````

## Installation des services PHYSALIA server

```
git clone https://github.com/jancelin/physalia.git
cd physalia/server
docker-compose build
docker-compose up -d
```
