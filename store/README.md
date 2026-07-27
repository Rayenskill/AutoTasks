# `store/` — couche de persistance SQLite

**Vide pour l'instant. Phase 4.**

Ce dossier accueillera la source de vérité unique du projet : les scripts, leurs versions, l'historique des runs, les événements par étape, et les planifications.

## Structure prévue

```
store/
├── include/store/     en-têtes publics — le contrat, comme engine/include/engine/
├── src/               implémentation
├── schema/            fichiers .sql de création et de migration
└── CMakeLists.txt     à ajouter quand le dossier sera peuplé
```

## Décision à prendre AVANT d'écrire du code ici

SQLite est au milieu des deux moitiés du projet : le moteur y écrit les événements, l'UI les lit. Deux options :

1. **Le moteur possède le store** — cohérent, c'est lui qui produit les données ; le frontend consomme en lecture seule.
2. **Une troisième cible `autotasks_store`** — l'un l'écrit, l'autre le consomme via une interface, exactement le schéma déjà utilisé pour le moteur.

La seconde est plus propre et réutilise un découpage que vous maîtriserez déjà à ce moment-là. Trancher **avant** d'y arriver, pas pendant — voir `docs/guides/TEAM_WORKFLOW.md` §5.

## Activation

Décommenter dans le `CMakeLists.txt` racine :

```cmake
# ---- Phase 4 : store ----
# add_subdirectory(store)
```

> Rappel `.gitignore` : les fichiers `.db` produits à l'exécution ne sont **jamais** versionnés. Ils contiennent l'historique de vos runs et des captures de votre écran.
