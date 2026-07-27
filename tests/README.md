# `tests/` — suite de tests GoogleTest

**Vide pour l'instant. Phases 2-3.**

## Pourquoi pas tout de suite

Il n'y a rien à tester d'utile en phase 0 : le stub est déterministe par construction et le squelette Win32 ne fait rien. Les premiers tests qui valent le coup arrivent avec du code qui a de vraies règles :

| Phase | Ce qui devient testable |
|---|---|
| 2 | Sérialisation JSON — écrire un script, le relire, vérifier qu'il est identique |
| 3 | Seuils de confiance — `confidenceThreshold` et `reviewThreshold` produisent-ils le bon `StepOutcome` ? |
| 4 | Store — écrire un run, le relire, vérifier les migrations de schéma |

Le seuil de confiance est le meilleur premier test du projet : c'est de la logique pure, sans souris ni écran, et c'est exactement là qu'un bug serait invisible à l'œil.

## Structure prévue

```
tests/
├── CMakeLists.txt        récupère GoogleTest via FetchContent
├── test_engine.cpp
└── test_store.cpp
```

`autotasks_core` étant une bibliothèque, les tests s'y lient exactement comme les deux exécutables — c'est l'autre bénéfice du découpage en bibliothèque.

## Activation

Décommenter dans le `CMakeLists.txt` racine :

```cmake
# if(AUTOTASKS_BUILD_TESTS)
#     enable_testing()
#     add_subdirectory(tests)
# endif()
```

Puis :

```powershell
ctest --preset dev
```
