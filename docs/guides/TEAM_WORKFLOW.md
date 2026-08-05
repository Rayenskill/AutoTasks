# AutoTasks — Travailler à deux

Comment le projet est découpé pour que deux personnes avancent en parallèle sans se bloquer ni se marcher dessus.

---

## 1. Le principe

Le moteur est une **bibliothèque**, pas un exécutable. Deux points d'entrée la consomment.

```
              engine/include/engine/Engine.h
                      LE CONTRAT
                           │
            ┌──────────────┴──────────────┐
            │                             │
    autotasks_core                        │
  (bibliothèque statique)                 │
   Win32Engine.cpp  ← dev MOTEUR          │
   StubEngine.cpp   ← partagé             │
            │                             │
    ┌───────┴────────┐                    │
    │                │                    │
autotasks_engine   autotasks_app ─────────┘
     (CLI)            (Qt)
  dev MOTEUR       dev FRONTEND
```

Pourquoi ça compte : un exécutable ne peut pas être lié depuis une autre cible — sous Windows un `.exe` ne produit pas de bibliothèque d'import. Tant que le moteur était un `add_executable`, le front Qt ne pouvait pas l'appeler du tout.

Conséquences concrètes :

- Le dev **moteur** n'installe pas Qt. Il compile avec le preset `engine` et teste avec le CLI.
- Le dev **frontend** ne touche jamais au Win32. Il code contre `Engine.h` et développe avec `createStubEngine()`.
- Ni l'un ni l'autre n'attend l'autre pour commencer.

---

## 2. Le contrat, seul point de contact

`engine/include/engine/Engine.h` déclare ce que le moteur offre : `Step`, `Script`, `StepResult`, `Engine::replay()`, `Engine::abort()`.

**Règle :** ce fichier ne se modifie pas unilatéralement. Un changement casse l'autre côté. Toute évolution se discute d'abord, puis se fait en une seule fois avec les deux implémentations mises à jour ensemble.

Deux interdits qui gardent la séparation propre :

- Aucun `#include <Q...>` dans `engine/`
- Aucun `SendInput`, aucun `<windows.h>` dans `app/`

Vérifiable en une commande — ça ne doit rien retourner :

```powershell
Select-String -Path engine\*,app\* -Recurse -Pattern "include <Q","SendInput","windows\.h"
```

---

## 3. Le moteur bouchon, ce qui débloque vraiment

Le vrai moteur, c'est les phases 1 à 3 : `SendInput`, hooks bas niveau, `matchTemplate` OpenCV. Plusieurs semaines. Le front Qt, lui, a besoin **dès aujourd'hui** d'un moteur qui répond.

Sans bouchon, il n'y a que trois issues, toutes mauvaises :

1. le frontend attend le moteur — une personne à l'arrêt,
2. il code à l'aveugle contre un fantôme — et découvre à l'intégration que son threading est faux,
3. il écrit du faux moteur *dans* l'UI — qu'il faudra arracher plus tard.

`createStubEngine()` implémente **la même interface `Engine`** que le vrai moteur, sans toucher la souris ni le clavier :

| Ce qu'il fait | Pourquoi c'est là |
|---|---|
| 400 ms par étape | Un stub instantané masquerait un gel d'interface. Avec le délai, l'erreur saute aux yeux dès le premier essai. |
| 1 étape sur 4 en confiance basse (0.68) | Permet de construire la **file de revue** — la fonctionnalité qui différencie le projet — avant que le moteur sache cliquer. |
| 1 étape sur 7 en échec | Permet de construire la gestion d'erreur et les captures d'échec. |
| Remplit `confidence` et `screenshotPath` | Le frontend affiche déjà les champs que la phase 3 remplira pour de vrai. |
| Déterministe, jamais aléatoire | Un bug d'affichage doit être reproductible à la demande. |

Le frontend peut donc construire et tester dès maintenant : l'affichage de progression en direct, le comportement des boutons pendant un rejeu, le threading, et la file de revue.

**Ce n'est pas du code jetable.** Après l'intégration, il reste le moyen d'exercer l'UI sans bouger la vraie souris, et la référence pour le dev moteur de ce à quoi ressemble une implémentation qui se comporte bien.

Le jour de l'intégration, il n'y a pas de « fusion des deux moitiés » : le frontend appelle `createEngine()` au lieu de `createStubEngine()`. Une ligne.

---

## 4. Propriété des fichiers

| Dossier / fichier | Propriétaire | Règle |
|---|---|---|
| `engine/include/engine/Engine.h` | **partagé** | modification concertée uniquement |
| `engine/src/Win32Engine.cpp` | moteur | le frontend n'y touche pas |
| `engine/src/win32/` | moteur | tout le code spécifique OS (à créer phase 1) |
| `engine/src/main.cpp` | moteur | banc d'essai CLI |
| `engine/src/StubEngine.cpp` | moteur | le frontend peut demander des cas de test |
| `app/` | frontend | toute l'UI Qt, y compris `ReplayController` |
| `store/` | à attribuer | phase 4, voir plus bas |
| `CMakeLists.txt` | **partagé** | source n°1 de conflits, voir §6 |
| `docs/` | partagé | |

---

## 5. Répartition des tâches

### Dev moteur — chemin critique

| Phase | Tâche | Livrable vérifiable |
|---|---|---|
| 1 | `SendInput` : déplacer, cliquer, taper | `autotasks_engine` reproduit une séquence codée en dur |
| 2 | Hooks bas niveau + sérialisation JSON | enregistrer → sauver → rejouer depuis un fichier |
| 3 | OpenCV : `matchTemplate`, seuils de confiance, attente d'image | une macro survit au déplacement de la fenêtre cible |
| — | remplir `StepResult::confidence` et `screenshotPath` | le frontend affiche déjà ces champs |

C'est le chemin critique du projet : sans lui, l'application n'a rien à piloter. Les `TODO` sont déjà posés phase par phase dans `Win32Engine.cpp`.

### Dev frontend — peut tout faire en parallèle

| Phase | Tâche | Dépend de |
|---|---|---|
| 5 | **Brancher `ReplayController` dans `MainWindow`** — bouton Run, bouton Abort, case « moteur simulé », journal | rien (bouchon) |
| 5 | Vue Library : liste des scripts, exécuter, progression | rien (bouchon) |
| 6 | Éditeur d'étapes : liste réordonnable, insertion, suppression | `Step` du contrat |
| 7 | Historique des runs, détail par étape, captures | store (phase 4) |
| 9 | File de revue : attendu / observé, confirmer ou corriger | `StepOutcome::LowConfidence` (déjà fourni par le bouchon) |

La première ligne est le point de départ : `ReplayController` existe et fonctionne, mais `MainWindow` ne l'utilise pas encore. Le mode d'emploi est en commentaire en haut de `app/include/app/ReplayController.h`.

### Phase 4 — le store, à décider ensemble

SQLite est au milieu : le moteur y écrit les événements, l'UI les lit. Deux options :

- **Le moteur le fait** — cohérent, il produit les données. Le frontend consomme.
- **Une troisième cible `autotasks_store`** — l'un l'écrit, l'autre le consomme via une interface, exactement comme pour le moteur.

La seconde est plus propre et reproduit un schéma que vous maîtriserez déjà. Décidez avant d'y arriver, pas pendant.

---

## 6. Git à deux

**Une branche par tâche**, jamais de commit direct sur `main` :

```
phase-1-sendinput          (moteur)
phase-5-library-view       (frontend)
fix/asan-runtime-deploy
```

**Pull Request pour fusionner**, même en solo. Ça donne un endroit pour relire le diff de l'autre et une trace lisible.

**Avant de créer une branche, partir d'un `main` à jour :**

```powershell
git switch main
git pull
git switch -c ma-branche
```

**`CMakeLists.txt` est le fichier qui va poser problème.** Les deux devs vont vouloir y ajouter des sources. Trois moyens de limiter les dégâts :

1. Ajouter vos fichiers dans des blocs distincts et éloignés (le bloc `autotasks_core` contre le bloc `qt_add_executable`)
2. Fusionner `main` dans votre branche souvent, plutôt que d'attendre la fin
3. À la phase 4, découper en `engine/CMakeLists.txt` et `app/CMakeLists.txt` — chacun son fichier, plus de conflit

**Fins de ligne :** `.gitattributes` normalise tout en LF. Si vos deux postes n'ont pas le même `core.autocrlf`, c'est ce qui évite les diffs où « tout le fichier a changé ».

---

## 7. Commandes par rôle

**Dev moteur** (Qt non requis) :

```powershell
cmake --preset engine
cmake --build --preset engine
.\build\engine\bin\autotasks_engine.exe          # vrai moteur
.\build\engine\bin\autotasks_engine.exe --stub   # moteur simulé
```

**Dev frontend** (Qt requis, voir `QT_SETUP.md`) :

```powershell
cmake --preset dev-qt
cmake --build --preset dev-qt
.\build\dev\bin\autotasks_app.exe
```

**Les deux, avant de pousser :**

```powershell
cmake --build --preset format   # clang-format
cmake --preset lint             # clang-tidy
cmake --build --preset lint
```

Toutes ces commandes doivent être lancées depuis une **Developer PowerShell for VS**, sinon le compilateur n'est pas dans le PATH.

---

## 8. Le point de synchronisation

Le moment où les deux moitiés se rejoignent : quand `createEngine()` renvoie un moteur qui fonctionne réellement, et que décocher « moteur simulé » dans l'UI produit un vrai rejeu.

Tout ce qui précède peut se faire séparément. **Visez ce moment tôt**, avec une seule étape fonctionnelle — un simple clic réel piloté depuis la fenêtre Qt vaut mieux que deux moitiés parfaites qui ne se sont jamais parlé.

---

*Documents liés : [`../../README.md`](../../README.md) · [`QT_SETUP.md`](QT_SETUP.md) · [`TOOLING.md`](TOOLING.md) · [`../planning/Architecture_and_Roadmap.md`](../planning/Architecture_and_Roadmap.md)*
