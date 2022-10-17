
# PHYSALIA  ![logo](https://github.com/jancelin/physalia/blob/main/docs/assets/Illustration_sans_titre.jpg?raw=true)

[![DOI](https://zenodo.org/badge/375017002.svg)](https://zenodo.org/badge/latestdoi/375017002)

## Plateforme HYdrographique pour la Surveillance Altimétrique du LIttoral 

La surveillance du niveau de la mer et des marais est un enjeu majeur dans la compréhension des dynamiques côtières et hydrologiques. Plusieurs études ont montré l’efficacité des bouées GNSS appliquées à la mesure du niveau de la mer. Toutefois, leur usage est généralement limité par le coût élevé d’un tel
dispositif.

Trois prototypes de bouées GNSS-RTK ont été développés afin de répondre à des problématiques liées à la surveillance en temps réel des variations du niveau de la mer et des marais littoraux. La réalisation des bouées est caractérisée par un besoin de réduction importante des coûts et s’élève à environ 600€.
Les bouées ont été construites autour d’une structure centrale imprimée en 3D et sont équipées de trois flotteurs montés sur des bras en acier inoxydable. La réception des signaux GNSS est assurée par un récepteur bi-fréquences Ublox-F9P, utilisable avec les constellations GPS, GLONASS, Galileo et Beidou. Les corrections RTK sont obtenues par le réseau de base, [CentipedeRTK](https://docs.centipede.fr/), et traitées en temps réel par un Raspberry-Pi. La liaison est assurée par un module 4G et deux antennes externes.

![buoy1_small](https://user-images.githubusercontent.com/6421175/121193428-20f7ca80-c86e-11eb-9353-d99511f2edb5.png)

Des essais réalisés sur l’île d’Aix (France) ont permis d’évaluer la précision des bouées en comparant les données obtenues avec celles d’un marégraphe radar. La comparaison est réalisée avec les données corrigées en temps réel et celles corrigées en post-traitement. Les résultats obtenus montrent une erreur quadratique moyenne (RMSE) comprise entre 1,93 et 3,03 cm validant ainsi l’utilisation des bouées GNSS-RTK développée de manière économique pour un usage marégraphique.
[
![mission Thalia 10/22](/assets/mission_thalia_10-22.gif)](https://github.com/jancelin/physalia/blob/main/docs/assets/mission_thalia_10-22.gif)
![VID_20210526_131756](https://user-images.githubusercontent.com/6421175/123070138-c9cc2b00-d413-11eb-9d8e-3b214ef0bcf7.gif)
![monitoring_small](https://user-images.githubusercontent.com/6421175/121193450-281ed880-c86e-11eb-855a-5cee7dc6ea80.png)
![partenaires](https://user-images.githubusercontent.com/6421175/121195306-c6f80480-c86f-11eb-92a4-7ca271be2f45.png)


