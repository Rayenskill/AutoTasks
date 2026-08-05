# Documentation AutoTasks

Deux familles de documents : ce qu'on construit, et comment on travaille.

## `guides/` — comment travailler

À lire en premier, dans cet ordre.

| Document | Pour qui | Ce qu'il couvre |
|---|---|---|
| [`guides/TEAM_WORKFLOW.md`](guides/TEAM_WORKFLOW.md) | **les deux** | Le découpage à deux devs, le contrat, le moteur bouchon, la propriété des fichiers, Git |
| [`guides/QT_SETUP.md`](guides/QT_SETUP.md) | dev frontend | Installer Qt, le configurer par machine, les pièges de déploiement |
| [`guides/TOOLING.md`](guides/TOOLING.md) | les deux | clang-format, clang-tidy, sanitizers, les commandes et les gotchas |

## `planning/` — ce qu'on construit

| Document | Ce qu'il couvre |
|---|---|
| [`planning/Pitch.md`](planning/Pitch.md) | Pourquoi le projet existe et ce qui le distingue |
| [`planning/SRS.md`](planning/SRS.md) | Spécification complète — chaque fonctionnalité, testable |
| [`planning/Architecture_and_Roadmap.md`](planning/Architecture_and_Roadmap.md) | Conception des composants, choix techniques, ordre de construction |
| [`planning/Learning_Path.md`](planning/Learning_Path.md) | Ressources par technologie, séquencées par phase |

---

## Note sur le nom

Les documents de `planning/` ont été écrits sous le nom de code **« Reprise »**, avant que le projet soit renommé **AutoTasks**. Le contenu reste valable ; le renommage est à faire en une passe, quand vous le déciderez — mieux vaut un commit dédié qu'un remplacement au fil de l'eau.
