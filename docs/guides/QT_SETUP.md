# AutoTasks — Mise en place de Qt 6

Guide complet : installation, intégration CMake, pièges Windows, et ce qu'il reste à construire vue par vue.

> **Ordre dans la roadmap.** Qt est prévu en phase 5, après le moteur (phases 1-4). Poser la coquille UI maintenant est utile pour dérisquer l'installation Qt/MSVC, mais les vues resteront vides tant que le moteur et le store n'existent pas. Ce guide livre **la coquille**, pas l'application finie.

---

## 1. Installer Qt — l'étape qui décide de tout

### Le seul choix qui compte : MSVC, pas MinGW

Ton projet compile avec **MSVC**. Qt doit donc être la build **MSVC 2022 64-bit**. Une build MinGW ne se lie pas à du code MSVC : tu obtiendrais des erreurs de symboles non résolus totalement opaques, sans indice sur la cause réelle.

C'est l'erreur la plus fréquente et la plus coûteuse en temps sur ce chemin.

### Quelle version

Qt 6.11 est la version courante (mars 2026). Qt 6.8 est la LTS actuelle, supportée jusqu'en octobre 2029.

**Pour un projet open source, prends simplement la dernière stable.** Les correctifs LTS sont réservés aux licences commerciales : côté open source, une version LTS n'apporte rien de plus qu'une version normale. L'étiquette LTS n'a d'intérêt que si tu passes un jour sous licence commerciale.

### Procédure

1. Télécharger le **Qt Online Installer** sur `qt.io/download-qt-installer`
2. Créer un compte Qt (obligatoire, gratuit)
3. Choisir la licence **open source** (LGPL/GPL)
4. Dans la sélection des composants, développer la dernière version de Qt 6 et cocher :
   - ☑ **MSVC 2022 64-bit** ← le composant critique
   - ☑ **Qt Debug Information Files** (facultatif, mais très utile pour déboguer)
   - ☐ MinGW — **ne pas cocher**
   - ☐ Qt Creator — inutile, tu travailles sous Visual Studio
5. Sous *Developer and Designer Tools*, tu peux tout décocher : CMake et Ninja viennent déjà de Visual Studio

L'installation atterrit typiquement dans `C:\Qt\<version>\msvc2022_64`. **Note ce chemin exact**, il sert à l'étape suivante.

### Vérifier

```powershell
Get-ChildItem C:\Qt -Directory
Get-ChildItem C:\Qt\6.11.1 -Directory    # doit contenir msvc2022_64
```

---

## 2. Indiquer à CMake où se trouve Qt

Le chemin d'installation dépend de la machine — il **ne doit pas** être écrit en dur dans `CMakePresets.json`, qui est versionné. Sinon le projet casse chez ton collaborateur.

CMake prévoit exactement ce cas : **`CMakeUserPresets.json`**, local et déjà présent dans le `.gitignore`.

```powershell
Copy-Item CMakeUserPresets.json.example CMakeUserPresets.json
```

Puis ajuste le chemin dans le fichier copié :

```json
"CMAKE_PREFIX_PATH": "C:/Qt/6.11.1/msvc2022_64"
```

Attention aux **slashs avant** (`/`) dans le JSON, pas des antislashs.

> ⚠️ **Piège JSON.** Le format n'accepte pas de commentaires, et CMake **refuse tout champ inconnu à la racine** du fichier de presets. Un `"_comment": [...]` en haut du fichier fait échouer *toutes* les commandes `cmake --preset` avec `Invalid extra field "_comment" in root object` — y compris le preset `engine`, qui n'a pourtant rien à voir avec Qt. Le seul endroit prévu pour du texte libre est l'objet `"vendor"`.

**Alternative** — une variable d'environnement `QTDIR`, que le `CMakeLists.txt` sait lire :

```powershell
[Environment]::SetEnvironmentVariable("QTDIR", "C:\Qt\6.11.1\msvc2022_64", "User")
```

Il faut rouvrir le terminal (et Visual Studio) pour qu'elle soit prise en compte.

---

## 3. Ce que fait le CMakeLists

Le bloc Qt est déjà en place. Les points à comprendre :

```cmake
find_package(Qt6 6.5 COMPONENTS Widgets QUIET)
```

`QUIET` et non `REQUIRED` : **si Qt est absent, seul le moteur est construit**, avec un avertissement clair. Un collaborateur sans Qt peut donc cloner et compiler sans rien casser. C'est délibéré.

`Widgets` seulement pour l'instant. Le composant `Sql` s'ajoutera à la phase 4, quand le store existera : exiger un module qu'on n'utilise pas ne sert qu'à faire échouer la détection sur une installation Qt partielle, avec un message qui laisse croire que Qt est absent.

```cmake
qt_standard_project_setup()
```

Active AUTOMOC, AUTOUIC et AUTORCC. Sans cela, la macro `Q_OBJECT` ne génère rien et tu obtiens des erreurs de type « undefined reference to vtable » — un message qui n'évoque absolument pas la vraie cause.

```cmake
qt_add_executable(autotasks_app ...)
```

À préférer à `add_executable` : gère le déploiement, les plugins et les spécificités de plateforme.

```cmake
set_target_properties(autotasks_app PROPERTIES WIN32_EXECUTABLE OFF)
```

`OFF` garde la console visible, pratique pour lire `qDebug()` pendant le développement. Passe à `ON` quand tu voudras une vraie application GUI sans fenêtre noire.

---

## 4. Le piège des DLL (le même que celui d'ASan)

Lancer l'exe hors de l'IDE échoue avec « Qt6Widgets.dll introuvable ». Visual Studio ajoute le dossier `bin` de Qt au PATH pendant le débogage F5, mais pas pour un lancement direct — exactement le mécanisme que tu as rencontré avec `clang_rt.asan_dynamic-x86_64.dll`.

Le `CMakeLists.txt` règle le problème en lançant **`windeployqt`** après chaque build : l'outil analyse l'exe et copie à côté toutes les DLL Qt nécessaires, plus les plugins de plateforme.

Si le message `windeployqt introuvable` apparaît au configure, ajoute le dossier `bin` de Qt au PATH :

```powershell
$env:PATH += ";C:\Qt\6.11.1\msvc2022_64\bin"
```

---

## 5. Premier build

```powershell
cmake --preset dev-qt
cmake --build --preset dev-qt
.\build\dev\bin\autotasks_app.exe
```

Vérifie la ligne du résumé de configuration :

```
  Application Qt    : ON
```

et le message `AutoTasks: application Qt activée (Qt 6.11.x)`. Si tu vois l'avertissement « Qt 6 introuvable », c'est que `CMAKE_PREFIX_PATH` ne pointe pas au bon endroit — reviens à l'étape 2.

Tu dois obtenir une fenêtre avec une navigation latérale à six entrées et des pages vides annotées de leur phase.

---

## 6. Visual Studio

Rien de particulier : **Fichier → Ouvrir → Dossier** sur la racine du dépôt. VS lit `CMakePresets.json` **et** `CMakeUserPresets.json`, donc le preset `dev-qt` apparaît dans la liste déroulante.

Puis **Projet → Supprimer le cache et reconfigurer**, et sélectionne `autotasks_app.exe` comme élément de démarrage.

**L'extension Qt Visual Studio Tools ne t'est pas nécessaire.** Elle sert aux projets pilotés par qmake ou `.vcxproj`. Ton projet est piloté par CMake, qui gère déjà moc, uic et rcc via `qt_standard_project_setup()`. C'est une source de confusion en moins — et c'est le blocage que tu avais rencontré en essayant de configurer qmake dans l'extension.

Utile en revanche : **Outils → Options → Débogage → Général → activer les visualiseurs natifs**, pour voir le contenu réel des `QString` dans le débogueur au lieu d'un pointeur brut.

---

## 7. clang-tidy et le code généré

moc et uic produisent du C++ qui n'est pas le tien et qui déclencherait des centaines d'avertissements. Le `CMakeLists.txt` écrit un `.clang-tidy` neutre à la racine du dossier de build :

```
Checks: '-*'
```

Les outils clang remontent l'arborescence depuis chaque fichier et trouvent celui-ci en premier pour tout ce qui est sous `build/`. Ton code source, lui, reste analysé normalement.

Si `/W4 /WX` bloque sur du code généré par Qt — c'est rare, la sortie de moc est propre — retire `autotasks_warnings` de `target_link_libraries` pour la cible Qt uniquement.

---

## 8. La règle d'architecture à ne pas enfreindre

> **L'interface ne synthétise jamais d'entrée.** Elle commande le moteur et lit le store.

Concrètement : aucun appel à `SendInput` depuis `app/`. La fenêtre demande au moteur de rejouer un script ; le moteur émet des événements ; l'orchestrateur les écrit dans le store ; les vues lisent le store.

C'est ce qui permet aux logs en direct, à l'historique et à la file de revue de venir d'une source unique et cohérente — et ce qui laisse la porte ouverte à un mode sans interface plus tard.

---

## 9. La vraie difficulté de la phase 5 : ne pas geler l'UI

Un rejeu dure plusieurs secondes ou minutes. Exécuté sur le thread principal, il fige la fenêtre : plus de rafraîchissement, plus de bouton d'arrêt, et Windows affiche « ne répond pas ».

**C'est déjà réglé.** `ReplayController` (`app/include/app/ReplayController.h`) lance `Engine::replay()` sur un `QThread` et convertit le callback du moteur en signaux Qt, que Qt livre tout seul sur le thread UI.

Il suffit de s'y connecter :

```cpp
m_replay = new ReplayController(this);
connect(m_replay, &ReplayController::runStarted,   this, &MaVue::onRunStarted);
connect(m_replay, &ReplayController::stepReported, this, &MaVue::onStep);
connect(m_replay, &ReplayController::runFinished,  this, &MaVue::onRunFinished);

m_replay->start(/*useStub=*/true);
```

`app/src/LibraryPage.cpp` en est l'exemple complet et fonctionnel.

Deux règles absolues restent à respecter dans tout ce que vous ajouterez : **ne jamais toucher un widget depuis un thread secondaire**, et faire circuler l'information uniquement par signaux.

Le moteur bouchon attend 400 ms par étape précisément pour rendre l'erreur visible : si la fenêtre se fige pendant un rejeu simulé, c'est que le travail a atterri sur le thread UI.

---

## 10. Ce qu'il reste à construire, vue par vue

Les six vues sont **dessinées** : chacune a son formulaire dans `app/ui/`, sa classe dans `app/src/` et `app/include/app/`. Ce qui manque, ce sont les **données** — les modèles qui les remplissent.

| Vue | Phase | Ce qu'il reste à brancher |
|---|---|---|
| Library | 5 | ✅ le rejeu marche (bouchon). Reste : `QTableView` + `QSqlTableModel` sur `scripts`, recherche via `QSortFilterProxyModel` |
| Editor | 6 | Modèle sur `Script::steps` pour `stepsView`, glisser-déposer, chargement/écriture des champs du panneau |
| Recorder | 6 | Appel au recorder du moteur, remplissage de `capturedView` au fil des actions |
| History | 7 | Trois modèles liés : `runs` → `run_events` → capture d'écran |
| Schedules | 8 | Modèle sur `schedules`, `QTimer` de déclenchement, calcul du prochain run |
| Review | 9 | Modèle sur les étapes en confiance basse, `QPixmap` attendu/observé, actions de décision |

Chaque `.cpp` contient un `TODO` à l'endroit exact où brancher le modèle.

L'ordre compte : **Library d'abord**, car elle valide toute la chaîne UI → store → moteur sur le cas le plus simple.

---

## 11. Dessiner une vue dans Qt Designer

**Toutes les vues sont dessinées, pas codées.** Pour changer l'apparence d'une page, on ouvre son `.ui` — jamais le C++.

**Designer est installé mais sans intégration Visual Studio** — le lancer directement (faites-vous un raccourci) :

```powershell
C:\Qt\6.11.1\msvc2022_64\bin\designer.exe
```

### Modifier une vue existante

Ouvrir le `.ui`, modifier, enregistrer, reconstruire. Rien d'autre à toucher — sauf si vous **ajoutez ou renommez** un widget, auquel cas le `.cpp` doit suivre.

| Vue | Formulaire |
|---|---|
| Library | `app/ui/LibraryPage.ui` |
| Editor | `app/ui/EditorPage.ui` |
| Recorder | `app/ui/RecorderPage.ui` |
| Schedules | `app/ui/SchedulesPage.ui` |
| History | `app/ui/HistoryPage.ui` |
| Review | `app/ui/ReviewPage.ui` |

> ⚠️ **Après avoir modifié un `.ui`, si l'application ne change pas visiblement**, c'est que Ninja n'a pas recompilé les `.obj` qui dépendent du header régénéré. Forcer :
> ```powershell
> cmake --build --preset dev-qt --clean-first
> ```

### Ajouter une nouvelle vue

Prendre `LibraryPage` comme patron — le trio `.ui` / `.h` / `.cpp` est là pour être recopié.

1. **Designer → Widget** (pas *Main Window* : la page ira dans le `QStackedWidget`, une `QMainWindow` ne s'imbrique pas).
2. Sélectionner le widget racine, `objectName = MaPage`. Nommer chaque widget utile (`confirmButton`, pas `pushButton_3`) : ces noms deviennent les accesseurs C++.
3. Enregistrer sous `app/ui/MaPage.ui`.
4. Ajouter les **trois** fichiers dans `qt_add_executable` du `CMakeLists.txt`.
5. Écrire le `.h` et le `.cpp` sur le modèle de `LibraryPage`.
6. `addPage(QStringLiteral("Ma page"), new MaPage(this));` dans `MainWindow::buildUi()`.
7. `cmake --build --preset dev-qt`.

Le header généré doit apparaître dans `build/dev/autotasks_app_autogen/include/ui_MaPage.h`.

### Ce qui ne se met PAS dans un `.ui`

- **Le contenu dynamique.** Le `.ui` contient un `QTableView` **vide** ; les lignes viennent d'un modèle à l'exécution. On ne dessine jamais « une ligne ».
- **Les facteurs d'étirement d'un `QSplitter`** : `setStretchFactor()` n'existe pas dans le format `.ui`. Il est appelé dans le constructeur de chaque page.
- **`MainWindow`** reste en code : elle n'assemble que la barre latérale et la pile.

### Les quatre pièges

| Piège | Symptôme | Pourquoi |
|---|---|---|
| Nom de fichier ≠ objectName | `Ui::MaPage` introuvable | Le **fichier** donne `ui_MaPage.h`, l'**objectName** donne la classe. Gardez-les identiques |
| `.ui` hors de `app/ui/` | `ui_MaPage.h: No such file` | AUTOUIC cherche à côté du `.cpp` ; `AUTOUIC_SEARCH_PATHS` couvre `app/ui/` seulement |
| `~MaPage() = default;` dans le `.h` | `invalid application of 'sizeof' to an incomplete type` | `unique_ptr<Ui::X>` exige le type complet pour détruire → destructeur dans le `.cpp` |
| Slots auto-connectés `on_xxx_clicked()` | Le slot n'est jamais appelé, **sans erreur de compilation** | Designer connecte par nom, en silence. `connect()` explicite échoue à la compilation |

---

## 12. Dépannage

| Symptôme | Cause | Correctif |
|---|---|---|
| `Qt6 introuvable` au configure | `CMAKE_PREFIX_PATH` absent ou faux | Étape 2 ; vérifier le nom exact du dossier |
| Symboles non résolus au link | Qt MinGW avec compilateur MSVC | Réinstaller le composant MSVC 2022 64-bit |
| `undefined reference to vtable` | moc n'a pas tourné | `qt_standard_project_setup()` présent ? Le `.h` avec `Q_OBJECT` est-il listé dans `qt_add_executable` ? |
| `Qt6Widgets.dll introuvable` | DLL non déployées | `windeployqt` (étape 4) ou Qt `bin` dans le PATH |
| `Could not find the Qt platform plugin "windows"` | Plugins manquants | `windeployqt` copie aussi les plugins — vérifier `platforms/qwindows.dll` à côté de l'exe |
| Fenêtre figée pendant un rejeu | Travail long sur le thread UI | Section 9 — passer par `ReplayController` |
| `ui_XXX.h: No such file or directory` | AUTOUIC n'a pas trouvé le `.ui` | Section 11 — nom du fichier, et `.ui` bien dans `app/ui/` |
| Un `.ui` modifié ne change rien à l'écran | Dépendance mal suivie par Ninja | `cmake --build --preset dev-qt --clean-first` |
| L'app ne se reconfigure pas dans VS | Cache périmé | Projet → Supprimer le cache et reconfigurer |

---

*Documents liés : `README.md` · `TOOLING.md` · `Architecture_and_Roadmap.md`*
