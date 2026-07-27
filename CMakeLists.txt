cmake_minimum_required(VERSION 3.21)   # 3.21+ requis par CMakePresets v3

project(AutoTasks
    VERSION     0.1.0
    DESCRIPTION "Desktop GUI automation: record, replay, review"
    LANGUAGES   CXX)                   # CXX seulement — pas de compilateur C inutile

# =========================================================================
# L'IDEE CENTRALE DE CE FICHIER
#
# Le moteur est une BIBLIOTHEQUE (autotasks_core), pas un executable. Deux
# points d'entree la consomment :
#
#         engine/include/engine/Engine.h   <- LE CONTRAT
#                        |
#               autotasks_core (STATIC)    <- le moteur, sans main()
#                    +----+----+
#                    |         |
#         autotasks_engine   autotasks_app
#              (CLI)             (Qt)
#           dev MOTEUR        dev FRONTEND
#
# C'est ce decoupage qui permet a deux personnes de travailler en parallele :
# le dev moteur n'installe pas Qt (preset `engine`), le dev frontend ne touche
# jamais au Win32. Voir docs/guides/TEAM_WORKFLOW.md.
# =========================================================================

# =========================================================================
# MSVC hot reload / Edit and Continue  (repris du template VS)
# Doit venir avant la creation des cibles.
# =========================================================================
if(POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
        "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

# =========================================================================
# Standard du langage  —  le "target" + "strict" du projet.
# =========================================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)   # echouer franchement plutot que retomber en silence
set(CMAKE_CXX_EXTENSIONS OFF)         # C++20 ISO, pas les dialectes MS/GNU

# Necessaire a clang-tidy, clangd et a l'outillage des editeurs.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# =========================================================================
# Emplacement des binaires — identique quel que soit le generateur.
# Les generateurs multi-config (Visual Studio) ajoutent sinon /Debug, /Release,
# et les chemins de la doc ne marchent plus qu'avec Ninja.
# =========================================================================
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

foreach(cfg IN ITEMS DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${cfg} ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${cfg} ${CMAKE_BINARY_DIR}/bin)
endforeach()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# =========================================================================
# Options
# =========================================================================
option(AUTOTASKS_WARNINGS_AS_ERRORS "Traiter les avertissements comme des erreurs" ON)
option(AUTOTASKS_ENABLE_CLANG_TIDY  "Lancer clang-tidy pendant le build"           OFF)
option(AUTOTASKS_ENABLE_SANITIZERS  "Activer AddressSanitizer (builds Debug)"      OFF)
option(AUTOTASKS_BUILD_TESTS        "Construire la suite de tests (phase 2-3)"     ON)
option(AUTOTASKS_BUILD_APP          "Construire l'application Qt (necessite Qt 6)" ON)

# =========================================================================
# Avertissements  —  c'est ici que vit le "mode strict".
# Bibliotheque INTERFACE : on la lie et la cible herite des flags. Mieux que
# des flags globaux, car le code tiers ne doit pas etre juge par nos regles.
# =========================================================================
add_library(autotasks_warnings INTERFACE)

if(MSVC)
    target_compile_options(autotasks_warnings INTERFACE
        /W4              # niveau eleve (/Wall est inutilisable sous MSVC)
        /permissive-     # <- le vrai "strict" : desactive les extensions MS
        /w14242          # conversion avec perte possible de donnees
        /w14263          # une methode masque une virtuelle (souvent une faute de frappe)
        /w14265          # fonctions virtuelles mais destructeur non virtuel
        /w14555          # expression sans effet
        /w14640          # initialisation de static locale non thread-safe
        /w14826          # conversion avec extension de signe, surprise possible
        /utf-8           # source ET execution en UTF-8
        /Zc:__cplusplus  # renvoyer la vraie valeur de __cplusplus (MSVC ment par defaut)
    )
    if(AUTOTASKS_WARNINGS_AS_ERRORS)
        target_compile_options(autotasks_warnings INTERFACE /WX)
    endif()
else()
    target_compile_options(autotasks_warnings INTERFACE
        -Wall -Wextra -Wpedantic
        -Wshadow              # variable locale masquant un nom externe — bug classique
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wcast-align
        -Wunused
        -Woverloaded-virtual
        -Wconversion
        -Wsign-conversion
        -Wnull-dereference
        -Wdouble-promotion
    )
    if(AUTOTASKS_WARNINGS_AS_ERRORS)
        target_compile_options(autotasks_warnings INTERFACE -Werror)
    endif()
endif()

# =========================================================================
# Sanitizers  —  verifications a l'execution (use-after-free, debordements).
#
# Sous MSVC, l'exe produit depend de clang_rt.asan_dynamic-x86_64.dll. Visual
# Studio ajoute le chemin au PATH pour le debogage F5, mais PAS pour un
# lancement direct : d'ou l'erreur systeme "DLL introuvable". La fonction
# autotasks_deploy_asan() plus bas copie cette DLL a cote de l'exe, ce qui rend
# le preset asan reellement utilisable.
#
# Prerequis : composant "C++ AddressSanitizer" dans l'installeur Visual Studio.
# Cout : environ 2x plus lent. A reserver a la chasse aux bugs memoire.
# =========================================================================
add_library(autotasks_sanitizers INTERFACE)

if(AUTOTASKS_ENABLE_SANITIZERS)
    if(MSVC)
        target_compile_options(autotasks_sanitizers INTERFACE /fsanitize=address)
        target_link_options(autotasks_sanitizers INTERFACE /INCREMENTAL:NO)

        # /RTC1 est incompatible avec ASan : on le retire des flags Debug.
        string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
        string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")

        # Les DLL runtime vivent a cote de cl.exe. Selon le runtime C utilise
        # (/MDd en Debug, /MD en Release), MSVC lie une variante differente :
        # clang_rt.asan_dynamic-* ou clang_rt.asan_dbg_dynamic-*. On copie donc
        # toutes celles qui sont presentes plutot que d'en deviner une seule.
        get_filename_component(_msvc_bin_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
        file(GLOB ASAN_RUNTIME_DLLS "${_msvc_bin_dir}/clang_rt.asan*dynamic*-x86_64.dll")

        if(ASAN_RUNTIME_DLLS)
            message(STATUS "AutoTasks: ASan actif — runtime copie a cote des exes")
        else()
            message(WARNING
                "AutoTasks: ASan actif mais aucun runtime clang_rt.asan*.dll trouve dans\n"
                "           ${_msvc_bin_dir}\n"
                "           Installer le composant 'C++ AddressSanitizer' :\n"
                "           Installeur Visual Studio > Modifier > Composants individuels.")
        endif()
    else()
        target_compile_options(autotasks_sanitizers INTERFACE
            -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(autotasks_sanitizers INTERFACE
            -fsanitize=address,undefined)
        message(STATUS "AutoTasks: ASan + UBSan actifs")
    endif()
endif()

# Copie la DLL runtime ASan a cote de l'executable, sinon il ne se lance pas
# hors de l'IDE. Sans effet quand les sanitizers sont desactives.
function(autotasks_deploy_asan target)
    if(AUTOTASKS_ENABLE_SANITIZERS AND MSVC AND ASAN_RUNTIME_DLLS)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${ASAN_RUNTIME_DLLS} "$<TARGET_FILE_DIR:${target}>"
            COMMENT "Copie du runtime ASan a cote de ${target}"
            VERBATIM)
    endif()
endfunction()

# =========================================================================
# clang-tidy  —  DOIT etre defini avant la creation de toute cible.
# Les regles sont dans .clang-tidy a la racine.
# =========================================================================
if(AUTOTASKS_ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy)
    if(CLANG_TIDY_EXE)
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
        message(STATUS "AutoTasks: clang-tidy active -> ${CLANG_TIDY_EXE}")
    else()
        message(WARNING "AutoTasks: clang-tidy demande mais introuvable dans le PATH")
    endif()
endif()

# =========================================================================
# autotasks_core — LA BIBLIOTHEQUE PARTAGEE
#
# Tout le moteur vit ici, SANS main(). C'est le changement qui rend le reste
# possible : un executable ne peut pas etre lie depuis une autre cible, une
# bibliotheque si.
#
# Aucun header Qt ne doit jamais etre inclus depuis engine/.
# =========================================================================
add_library(autotasks_core STATIC
    engine/src/StubEngine.cpp
    engine/src/Win32Engine.cpp
    engine/include/engine/Engine.h    # liste pour l'affichage dans les IDE
)

# PUBLIC : toute cible qui lie autotasks_core recupere automatiquement le
# chemin d'include, sans avoir a le redeclarer. C'est la difference essentielle
# avec PRIVATE — et la raison pour laquelle autotasks_app trouve Engine.h.
target_include_directories(autotasks_core PUBLIC engine/include)

target_link_libraries(autotasks_core PRIVATE
    autotasks_warnings
    autotasks_sanitizers
)

if(WIN32)
    # SendInput / SetWindowsHookEx (phases 1-2).
    target_link_libraries(autotasks_core PRIVATE user32)
endif()

# =========================================================================
# autotasks_engine — CLI, banc d'essai du moteur
# Se compile SANS Qt : le developpeur moteur n'a pas besoin de l'installer.
#     cmake --preset engine && cmake --build --preset engine
# =========================================================================
add_executable(autotasks_engine engine/src/main.cpp)

target_link_libraries(autotasks_engine PRIVATE
    autotasks_core
    autotasks_warnings
    autotasks_sanitizers
)

autotasks_deploy_asan(autotasks_engine)

# ---- Phase 3 : OpenCV ---------------------------------------------------
# A ajouter sur autotasks_core, pas sur les executables.
# find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs)
# target_include_directories(autotasks_core SYSTEM PRIVATE ${OpenCV_INCLUDE_DIRS})
# target_link_libraries(autotasks_core PRIVATE ${OpenCV_LIBS})

# ---- Phase 4 : store ----------------------------------------------------
# add_subdirectory(store)

# =========================================================================
# autotasks_app — APPLICATION Qt (phase 5)
#
# Cible OPTIONNELLE : si Qt 6 est introuvable, seul le moteur est construit et
# la configuration reussit quand meme. Un collaborateur sans Qt peut donc
# cloner et compiler sans rien installer.
#
# Indiquer l'emplacement de Qt (voir docs/guides/QT_SETUP.md) :
#   - CMakeUserPresets.json  (recommande, non versionne, machine par machine)
#   - variable d'environnement QTDIR
#   - -D CMAKE_PREFIX_PATH="C:/Qt/6.11.0/msvc2022_64"
# =========================================================================
if(NOT CMAKE_PREFIX_PATH AND DEFINED ENV{QTDIR})
    list(APPEND CMAKE_PREFIX_PATH "$ENV{QTDIR}")
    message(STATUS "AutoTasks: CMAKE_PREFIX_PATH pris depuis QTDIR = $ENV{QTDIR}")
endif()

if(AUTOTASKS_BUILD_APP)
    # Widgets seulement pour l'instant. Ajouter Sql a la phase 4, quand le
    # store existera — exiger un module inutilise ne sert qu'a faire echouer
    # la detection sur une installation Qt partielle.
    find_package(Qt6 6.5 COMPONENTS Widgets QUIET)

    if(Qt6_FOUND)
        qt_standard_project_setup()      # active AUTOMOC / AUTOUIC / AUTORCC

        # AUTOMOC est indispensable : sans lui, la macro Q_OBJECT de
        # MainWindow.h et ReplayController.h produit des erreurs de link
        # "unresolved external symbol ... staticMetaObject".
        qt_add_executable(autotasks_app
            app/src/main.cpp
            app/src/MainWindow.cpp
            app/src/ReplayController.cpp
            app/include/app/MainWindow.h
            app/include/app/ReplayController.h
        )

        target_include_directories(autotasks_app PRIVATE app/include)

        # autotasks_core apporte engine/include automatiquement (PUBLIC).
        target_link_libraries(autotasks_app PRIVATE
            Qt6::Widgets
            autotasks_core
            autotasks_warnings      # si moc genere du code qui declenche /WX,
            autotasks_sanitizers    # retirer autotasks_warnings D'ICI seulement
        )

        # OFF = la console reste visible (pratique pour qDebug en developpement).
        # Passer a ON pour une vraie application GUI sans fenetre console.
        set_target_properties(autotasks_app PROPERTIES WIN32_EXECUTABLE OFF)

        autotasks_deploy_asan(autotasks_app)

        # clang-tidy ne doit pas analyser le code genere par moc/uic/rcc.
        # Un .clang-tidy neutre a la racine du dossier de build suffit : les
        # outils clang remontent l'arborescence et trouvent celui-ci en premier
        # pour tout ce qui est sous build/.
        file(WRITE "${CMAKE_BINARY_DIR}/.clang-tidy" "Checks: '-*'\n")

        # Copie les DLL Qt a cote de l'exe apres chaque build. Sans cela,
        # lancer l'exe hors de l'IDE echoue sur "Qt6Widgets.dll introuvable".
        if(WIN32)
            get_target_property(_qt_qmake Qt6::qmake IMPORTED_LOCATION)
            get_filename_component(_qt_bin "${_qt_qmake}" DIRECTORY)
            find_program(WINDEPLOYQT_EXE windeployqt HINTS "${_qt_bin}")

            if(WINDEPLOYQT_EXE)
                add_custom_command(TARGET autotasks_app POST_BUILD
                    COMMAND "${WINDEPLOYQT_EXE}"
                            --no-translations
                            --no-compiler-runtime
                            "$<TARGET_FILE:autotasks_app>"
                    COMMENT "windeployqt : copie des DLL Qt a cote de l'exe"
                    VERBATIM)
            else()
                message(WARNING "AutoTasks: windeployqt introuvable — ajouter ${_qt_bin} au PATH")
            endif()
        endif()

        message(STATUS "AutoTasks: application Qt activee (Qt ${Qt6_VERSION})")
    else()
        message(WARNING
            "AutoTasks: Qt 6 introuvable — la cible autotasks_app est desactivee.\n"
            "           Le moteur se compile normalement, rien n'est casse.\n"
            "           Pour activer l'app : copier CMakeUserPresets.json.example en\n"
            "           CMakeUserPresets.json et y mettre le chemin de Qt.\n"
            "           Voir docs/guides/QT_SETUP.md.")
    endif()
endif()

# ---- Tests (GoogleTest, ~phase 2-3) -------------------------------------
# if(AUTOTASKS_BUILD_TESTS)
#     enable_testing()
#     add_subdirectory(tests)
# endif()

# =========================================================================
# Cible pratique :  cmake --build --preset format
# =========================================================================
find_program(CLANG_FORMAT_EXE NAMES clang-format)
if(CLANG_FORMAT_EXE)
    file(GLOB_RECURSE AUTOTASKS_ALL_SOURCES
        ${CMAKE_SOURCE_DIR}/engine/*.cpp ${CMAKE_SOURCE_DIR}/engine/*.h
        ${CMAKE_SOURCE_DIR}/store/*.cpp  ${CMAKE_SOURCE_DIR}/store/*.h
        ${CMAKE_SOURCE_DIR}/app/*.cpp    ${CMAKE_SOURCE_DIR}/app/*.h
        ${CMAKE_SOURCE_DIR}/tests/*.cpp)
    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXE} -i --style=file ${AUTOTASKS_ALL_SOURCES}
        COMMENT "Formatage des sources avec clang-format"
        VERBATIM)
endif()

# =========================================================================
# Resume de configuration — lire ces lignes quand quelque chose surprend.
# =========================================================================
message(STATUS "-----------------------------------------------------------")
message(STATUS "AutoTasks ${PROJECT_VERSION}")
message(STATUS "  Generateur        : ${CMAKE_GENERATOR}")
message(STATUS "  Type de build     : ${CMAKE_BUILD_TYPE}")
message(STATUS "  Standard          : C++${CMAKE_CXX_STANDARD}")
message(STATUS "  Warnings = erreurs: ${AUTOTASKS_WARNINGS_AS_ERRORS}")
message(STATUS "  clang-tidy        : ${AUTOTASKS_ENABLE_CLANG_TIDY}")
message(STATUS "  Sanitizers (ASan) : ${AUTOTASKS_ENABLE_SANITIZERS}")
message(STATUS "  Application Qt    : ${AUTOTASKS_BUILD_APP}")
message(STATUS "  Binaires          : ${CMAKE_BINARY_DIR}/bin")
message(STATUS "-----------------------------------------------------------")
