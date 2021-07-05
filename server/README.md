# Design

![Serveur_data](https://user-images.githubusercontent.com/6421175/124486146-95f6eb00-ddad-11eb-9f05-691d740e1e80.jpg)


# prérequis

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


# Installation des services PHYSALIA server

```
git clone https://github.com/jancelin/physalia.git
cd physalia/server
docker-compose build
docker-compose up -d
```

# Ports

* Postgresql/postgis : 5433
* Pgadmin4: 5051
* grafana: 3000
* buoy GNSS data tracking: 8090

# Login et mot de passe:

https://github.com/jancelin/physalia/blob/main/server/.env


![Screenshot 2021-06-04 at 17-22-32 Bouee GNSS LIENSS INRAE - Grafana](https://user-images.githubusercontent.com/6421175/122247286-87f92d00-cec7-11eb-961d-744e1f8a4def.png)
