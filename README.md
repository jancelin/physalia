# PHYSALIA

## Plateforme HYbride pour la Surveillance Altimétrique du LIttoral
![buoy1_small](https://user-images.githubusercontent.com/6421175/121193428-20f7ca80-c86e-11eb-9353-d99511f2edb5.png)

La surveillance du niveau de la mer et des marais est un enjeu majeur dans la compréhension des dynamiques côtières et hydrologiques. Plusieurs études ont montré l’efficacité des bouées GNSS appliquées à la mesure du niveau de la mer. Toutefois, leur usage est généralement limité par le coût élevé d’un tel
dispositif.

Trois prototypes de bouées GNSS-RTK ont été développés afin de répondre à des problématiques liées à la surveillance en temps réel des variations du niveau de la mer et des marais littoraux. La réalisation des bouées est caractérisée par un besoin de réduction importante des coûts et s’élève à environ 600€.
Les bouées ont été construites autour d’une structure centrale imprimée en 3D et sont équipées de trois flotteurs montés sur des bras en acier inoxydable. La réception des signaux GNSS est assurée par un récepteur bi-fréquences Ublox-F9P, utilisable avec les constellations GPS, GLONASS, Galileo et Beidou. Les corrections RTK sont obtenues par le réseau de base, [CentipedeRTK](https://docs.centipede.fr/), et traitées en temps réel par un Raspberry-Pi. La liaison est assurée par un module 4G et deux antennes externes.

Des essais réalisés sur l’île d’Aix (France) ont permis d’évaluer la précision des bouées en comparant les données obtenues avec celles d’un marégraphe radar. La comparaison est réalisée avec les données corrigées en temps réel et celles corrigées en post-traitement. Les résultats obtenus montrent une erreur quadratique moyenne (RMSE) comprise entre 1,93 et 3,03 cm validant ainsi l’utilisation des bouées GNSS-RTK développée de manière économique pour un usage marégraphique.

![buoy_small](https://user-images.githubusercontent.com/6421175/121193440-25bc7e80-c86e-11eb-95a2-a34d9fa0d071.png)
![monitoring_small](https://user-images.githubusercontent.com/6421175/121193450-281ed880-c86e-11eb-855a-5cee7dc6ea80.png)
![partenaires](https://user-images.githubusercontent.com/6421175/121195306-c6f80480-c86f-11eb-92a4-7ca271be2f45.png)


