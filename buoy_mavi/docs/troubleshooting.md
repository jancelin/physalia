Dépannage
=========

Les pannes identifiées du système ont été séparées en deux catégories :

- Les pannes au démarrage
- Les pannes en fonctionnement

La procédure de dépannage à adopter est déterminée par les arbres décisonnels suivants.

**Attention, il existe de nombreux modes défaillance possibles du système, ce pourquoi ce document n'est pas exhaustif. Il est donc possible que la panne que vous rencontrez n'y soit pas répertoriée. Le cas échéant, merci d'en faire part à votre administrateur.**

Arbes décisionnels
-------------------

### Lecture

Les arbres décisionnels permettent d'identifier les causes probables d'une panne, et les actions à mettre en oeuvre pour la résoudre. Ils sont contitués de branches, débutant par une origine (à l'extrème gauche) et terminant par des feuilles (à l'extrème droite). Chaque arbre concerne une panne particulière.

Ils présentent deux sens de lecture :

- De gauche à droite :<br>
  Ce sens de lecture part de la panne, jusqu'à identifier la cause précise et sa solution.<br>
  A l'origine, en couleur, la panne rencontrée est décrite. Lui susccèdent, en couleur claire, des constats permettent d'orienter la recherche de cause. Ensuite, en couleur sombre, les causes, de plus en pus précises qu'elles progressent vers les feuilles de la branche. Finalement, les feuilles de la branche proposent les solution qui permettraient de résoudre la panne.

- De haut en bas :<br>
  Les constats et causes ont été organisés par probabilité d'apparition. Sur la même ligne verticale, les plus probables se situent en haut, et les moins probables en bas.

### Utilisation

1. Trouver l'arbre correspondant à la panne (origine en couleur de l'arbre).
2. Identifier le constat correspondant à la situation (couleur claire).
3. Pour ce constat, éliminer les causes en les parcourant de la plus à la moins probable.
4. A la première cause non éliminée, appliquer la solution correspondante.
5. Si la panne n'est pas résolue, continuer de parcourir l'arbre en éliminant les causes.

### Exemple d'utilisation

**Panne :** Interruption du flux de données sortant de la bouée

**Etat du système :** 

- Aucun des composants électroniques n'est allumé.
- Le contrôleur de charge est opérationnel.
- Le contrôleur de charge indique que la batterie est chargée.

**Dépannage :**

1. L'arbre correspondant est le premier des "*Pannes en fonctionnement*".
2. Le constat "*Aucun composant allumé*" correspond à la situation.
3. La branche "*Défaillance batterie*" peut être éliminée car :
	- Le contrôleur de charge indique que la batterie n'est pas déchargée.
	- L'état de charge de la batterie sur le contrôleur de charge montre qu'elle y est bien connectée.
	- La batterie ne semble pas endommagée.<br>
  Dans la branche "*Défaillance contrôleur de charge*" :
	- Le voyant de sortie de contrôleur de charge est éteint, sa sortie n'est pas active.
4. L'activation de la sortie du contrôleur de charge solutionne le problème.
5. Problème solutionné.

Pannes au démarrage
------------


Pannes en fonctionnement
-----------------

![](assets/troubleshooting/tbshooting_operation.png)

