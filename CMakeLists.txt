#===============================================================
# Half-Life Mod CMake Project
#===============================================================

cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_SKIP_BUILD_RPATH ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(halflife
    VERSION 1.0.0
    DESCRIPTION "Half-Life Mod"
    LANGUAGES CXX
)

set(HALFLIFE_GAMEDESC "Team Fortress" CACHE STRING "Server browser game & default scoreboard title")
option(HALFLIFE_SAVERESTORE "Enable singleplayer game save system" OFF)
option(HALFLIFE_NODEGRAPH "Enable node graphs for monster AI" OFF)
option(HALFLIFE_BOTS "Enable multiplayer bots" ON)
option(HALFLIFE_JOYSTICK "Enable joystick support" OFF)
option(HALFLIFE_TANKCONTROL "Half-Life player tank control" OFF)
option(HALFLIFE_TRAINCONTROL "Half-Life player train control" OFF)
option(HALFLIFE_GRENADES "Team Fortress style grenade priming" ON)

set(HL_SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(SHARED_SRC_DIR ${HL_SRC_DIR}/shared)
set(SERVER_SRC_DIR ${HL_SRC_DIR}/server)
set(CLIENT_SRC_DIR ${HL_SRC_DIR}/client)

set(SHARED_SRC
    ${SHARED_SRC_DIR}/entities/animating.cpp
    ${SHARED_SRC_DIR}/entities/player_shared.cpp
    ${SHARED_SRC_DIR}/entities/tf_axe.cpp
    ${SHARED_SRC_DIR}/entities/tf_pipebomb.cpp
    ${SHARED_SRC_DIR}/entities/tf_sniper.cpp
    ${SHARED_SRC_DIR}/entities/tf_weapons.cpp
    ${SHARED_SRC_DIR}/entities/weapons_shared.cpp

    ${SHARED_SRC_DIR}/movement/gamemovement_climb.cpp
    ${SHARED_SRC_DIR}/movement/gamemovement_duck.cpp
    ${SHARED_SRC_DIR}/movement/gamemovement_swim.cpp
    ${SHARED_SRC_DIR}/movement/gamemovement_walk.cpp
    ${SHARED_SRC_DIR}/movement/gamemovement.cpp
    ${SHARED_SRC_DIR}/movement/pm_debug.cpp
    ${SHARED_SRC_DIR}/movement/pm_math.cpp
    ${SHARED_SRC_DIR}/movement/pm_shared.cpp

    ${SHARED_SRC_DIR}/animation.cpp
    ${SHARED_SRC_DIR}/interface.cpp
    ${SHARED_SRC_DIR}/trace.cpp
    ${SHARED_SRC_DIR}/tf_defs.cpp

    ${SHARED_SRC_DIR}/utils/filesystem_utils.cpp
    ${SHARED_SRC_DIR}/utils/shared_util.cpp
)

set(SERVER_SRC
    ${SERVER_SRC_DIR}/entities/bmodels.cpp
    ${SERVER_SRC_DIR}/entities/buttons.cpp
    ${SERVER_SRC_DIR}/entities/cbase.cpp
    ${SERVER_SRC_DIR}/entities/combat.cpp
    ${SERVER_SRC_DIR}/entities/doors.cpp
    ${SERVER_SRC_DIR}/entities/effects.cpp
    ${SERVER_SRC_DIR}/entities/explode.cpp
    ${SERVER_SRC_DIR}/entities/func_break.cpp
    ${SERVER_SRC_DIR}/entities/func_tank.cpp
    ${SERVER_SRC_DIR}/entities/ggrenade.cpp
    ${SERVER_SRC_DIR}/entities/h_cycler.cpp
    ${SERVER_SRC_DIR}/entities/items.cpp
    ${SERVER_SRC_DIR}/entities/lights.cpp
    ${SERVER_SRC_DIR}/entities/maprules.cpp
    ${SERVER_SRC_DIR}/entities/monstermaker.cpp
    ${SERVER_SRC_DIR}/entities/mortar.cpp
    ${SERVER_SRC_DIR}/entities/nail.cpp
    ${SERVER_SRC_DIR}/entities/observer.cpp
    ${SERVER_SRC_DIR}/entities/pathcorner.cpp
    ${SERVER_SRC_DIR}/entities/pipebomb.cpp
    ${SERVER_SRC_DIR}/entities/plats.cpp
    ${SERVER_SRC_DIR}/entities/player.cpp
    ${SERVER_SRC_DIR}/entities/rocket.cpp
    ${SERVER_SRC_DIR}/entities/sound.cpp
    ${SERVER_SRC_DIR}/entities/spectator.cpp
    ${SERVER_SRC_DIR}/entities/subs.cpp
    ${SERVER_SRC_DIR}/entities/tf_detpack.cpp
    ${SERVER_SRC_DIR}/entities/tf_goal_item.cpp
    ${SERVER_SRC_DIR}/entities/tf_goal_timer.cpp
    ${SERVER_SRC_DIR}/entities/tf_goal.cpp
    ${SERVER_SRC_DIR}/entities/tf_map.cpp
    ${SERVER_SRC_DIR}/entities/triggers.cpp
    ${SERVER_SRC_DIR}/entities/weaponbox.cpp
    ${SERVER_SRC_DIR}/entities/weapons.cpp
    ${SERVER_SRC_DIR}/entities/world.cpp

    ${SERVER_SRC_DIR}/gamerules/gamerules.cpp
    ${SERVER_SRC_DIR}/gamerules/mapcycle.cpp
    ${SERVER_SRC_DIR}/gamerules/multiplay_gamerules.cpp
    ${SERVER_SRC_DIR}/gamerules/singleplay_gamerules.cpp
    ${SERVER_SRC_DIR}/gamerules/tf_gamerules.cpp

    ${SERVER_SRC_DIR}/client.cpp
    ${SERVER_SRC_DIR}/game.cpp
    ${SERVER_SRC_DIR}/h_export.cpp
    ${SERVER_SRC_DIR}/tent.cpp
    ${SERVER_SRC_DIR}/UserMessages.cpp
    ${SERVER_SRC_DIR}/util.cpp
    ${SERVER_SRC_DIR}/voice_gamemgr.cpp
    ${SERVER_SRC_DIR}/vote_manager.cpp
    ${SERVER_SRC_DIR}/vote_poll.cpp

    ${SHARED_SRC}
)

set(CLIENT_SRC
    ${CLIENT_SRC_DIR}/ui/hud/ammo_secondary.cpp
    ${CLIENT_SRC_DIR}/ui/hud/ammo.cpp
    ${CLIENT_SRC_DIR}/ui/hud/ammohistory.cpp
    ${CLIENT_SRC_DIR}/ui/hud/battery.cpp
    ${CLIENT_SRC_DIR}/ui/hud/death.cpp
    ${CLIENT_SRC_DIR}/ui/hud/flashlight.cpp
    ${CLIENT_SRC_DIR}/ui/hud/geiger.cpp
    ${CLIENT_SRC_DIR}/ui/hud/health.cpp
    ${CLIENT_SRC_DIR}/ui/hud/hud_msg.cpp
    ${CLIENT_SRC_DIR}/ui/hud/hud_redraw.cpp
    ${CLIENT_SRC_DIR}/ui/hud/hud_spectator.cpp
    ${CLIENT_SRC_DIR}/ui/hud/hud_update.cpp
    ${CLIENT_SRC_DIR}/ui/hud/hud.cpp
    ${CLIENT_SRC_DIR}/ui/hud/menu.cpp
    ${CLIENT_SRC_DIR}/ui/hud/message.cpp
    ${CLIENT_SRC_DIR}/ui/hud/saytext.cpp
    ${CLIENT_SRC_DIR}/ui/hud/status_icons.cpp
    ${CLIENT_SRC_DIR}/ui/hud/statusbar.cpp
    ${CLIENT_SRC_DIR}/ui/hud/text_message.cpp

    ${CLIENT_SRC_DIR}/ui/vgui/vgui_ClassMenu.cpp
    ${CLIENT_SRC_DIR}/ui/vgui/vgui_CustomObjects.cpp
    ${CLIENT_SRC_DIR}/ui/vgui/vgui_int.cpp
    ${CLIENT_SRC_DIR}/ui/vgui/vgui_MOTDWindow.cpp
    ${CLIENT_SRC_DIR}/ui/vgui/vgui_SchemeManager.cpp
    ${CLIENT_SRC_DIR}/ui/vgui/vgui_ScorePanel.cpp
    ${CLIENT_SRC_DIR}/ui/vgui/vgui_SpectatorPanel.cpp
    ${CLIENT_SRC_DIR}/ui/vgui/vgui_TeamFortressViewport.cpp
    ${CLIENT_SRC_DIR}/ui/vgui/vgui_teammenu.cpp

    ${CLIENT_SRC_DIR}/ui/vgui/utils/vgui_checkbutton2.cpp
    ${CLIENT_SRC_DIR}/ui/vgui/utils/vgui_grid.cpp
    ${CLIENT_SRC_DIR}/ui/vgui/utils/vgui_helpers.cpp
    ${CLIENT_SRC_DIR}/ui/vgui/utils/vgui_listbox.cpp
    ${CLIENT_SRC_DIR}/ui/vgui/utils/vgui_loadtga.cpp
    ${CLIENT_SRC_DIR}/ui/vgui/utils/vgui_scrollbar2.cpp
    ${CLIENT_SRC_DIR}/ui/vgui/utils/vgui_slider2.cpp

    ${CLIENT_SRC_DIR}/ui/voice_banmgr.cpp
    ${CLIENT_SRC_DIR}/ui/voice_status.cpp

    ${CLIENT_SRC_DIR}/entities/hl_baseentity.cpp
    ${CLIENT_SRC_DIR}/entities/hl_objects.cpp
    ${CLIENT_SRC_DIR}/entities/hl_weapons.cpp
    ${CLIENT_SRC_DIR}/entities/player_client.cpp
    ${CLIENT_SRC_DIR}/entities/weapons_client.cpp

    ${CLIENT_SRC_DIR}/cdll_int.cpp
    ${CLIENT_SRC_DIR}/com_weapons.cpp
    ${CLIENT_SRC_DIR}/demo.cpp
    ${CLIENT_SRC_DIR}/entity.cpp
    ${CLIENT_SRC_DIR}/ev_common.cpp
    ${CLIENT_SRC_DIR}/ev_hldm.cpp
    ${CLIENT_SRC_DIR}/interpolation.cpp
    ${CLIENT_SRC_DIR}/parsemsg.cpp
    ${CLIENT_SRC_DIR}/studio_util.cpp
    ${CLIENT_SRC_DIR}/util.cpp
    ${CLIENT_SRC_DIR}/view_spec.cpp
    ${CLIENT_SRC_DIR}/view.cpp

    ${CLIENT_SRC_DIR}/rendering/GameStudioModelRenderer.cpp
    ${CLIENT_SRC_DIR}/rendering/StudioModelRenderer.cpp
    ${CLIENT_SRC_DIR}/rendering/tri.cpp

    ${CLIENT_SRC_DIR}/rendering/particleman/CBaseParticle.cpp
    ${CLIENT_SRC_DIR}/rendering/particleman/CFrustum.cpp
    ${CLIENT_SRC_DIR}/rendering/particleman/CMiniMem.cpp
    ${CLIENT_SRC_DIR}/rendering/particleman/IParticleMan_Active.cpp

    ${CLIENT_SRC_DIR}/input/in_camera.cpp
    ${CLIENT_SRC_DIR}/input/in_keyboard.cpp
    ${CLIENT_SRC_DIR}/input/in_mouse.cpp

    ${SHARED_SRC}
)

set(HL_COMPILE_DEFS HALFLIFE_GAMEDESC="${HALFLIFE_GAMEDESC}")
set(HL_COMPILE_OPTIONS)
set(HL_LINK_OPTIONS)

if(HALFLIFE_BOTS)

    list(APPEND HL_COMPILE_DEFS HALFLIFE_BOTS)

    list(APPEND SERVER_SRC
        ${SERVER_SRC_DIR}/bot/bot_manager.cpp
        ${SERVER_SRC_DIR}/bot/bot_profile.cpp
        ${SERVER_SRC_DIR}/bot/bot_util.cpp
        ${SERVER_SRC_DIR}/bot/bot.cpp
        ${SERVER_SRC_DIR}/bot/hl_bot_manager.cpp
        ${SERVER_SRC_DIR}/bot/hl_bot.cpp
        ${SERVER_SRC_DIR}/bot/nav_area.cpp
        ${SERVER_SRC_DIR}/bot/nav_file.cpp
        ${SERVER_SRC_DIR}/bot/nav_node.cpp
        ${SERVER_SRC_DIR}/bot/nav_path.cpp
    )

endif()

if(HALFLIFE_SAVERESTORE)

    list(APPEND HL_COMPILE_DEFS HALFLIFE_SAVERESTORE)

    list(APPEND SERVER_SRC
        ${SERVER_SRC_DIR}/saverestore.cpp
    )

endif()

if(HALFLIFE_NODEGRAPH)

    list(APPEND HL_COMPILE_DEFS HALFLIFE_NODEGRAPH)

    list(APPEND SERVER_SRC
        ${SERVER_SRC_DIR}/nodes.cpp
    )

endif()

if(HALFLIFE_JOYSTICK)

    list(APPEND HL_COMPILE_DEFS HALFLIFE_JOYSTICK)

    list(APPEND CLIENT_SRC
        ${CLIENT_SRC_DIR}/input/in_joystick.cpp
    )

endif()

if(HALFLIFE_TANKCONTROL)

    list(APPEND HL_COMPILE_DEFS HALFLIFE_TANKCONTROL)

endif()

if(HALFLIFE_TRAINCONTROL)

    list(APPEND CLIENT_SRC
        ${CLIENT_SRC_DIR}/ui/hud/train.cpp
    )

    list(APPEND HL_COMPILE_DEFS HALFLIFE_TRAINCONTROL)

endif()

if(HALFLIFE_GRENADES)

    list(APPEND HL_COMPILE_DEFS HALFLIFE_GRENADES)

endif()

if(UNIX)

    list(APPEND HL_COMPILE_DEFS
        POSIX
        LINUX
    )

    list(APPEND HL_COMPILE_OPTIONS
        -m32
        -march=pentium-m
        -mfpmath=387
        -mno-sse
        
        -fpermissive
        -fno-strict-aliasing
        -fno-exceptions
        -fexpensive-optimizations
        -Werror=return-type
        -w
        
        -flifetime-dse=1
    )

    list(APPEND HL_LINK_OPTIONS
        -m32
        -static-libstdc++
        -static-libgcc
    )

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")

        list(APPEND HL_COMPILE_OPTIONS
            -gdwarf-2
            -g2
        )

        list(APPEND HL_LINK_OPTIONS
            -gdwarf-2
            -g2
        )

    elseif()

        list(APPEND HL_COMPILE_OPTIONS
            -O3
        )

    endif()

elseif(WIN32)

    list(APPEND HL_COMPILE_DEFS
        WIN32
    )

endif()

set(HL_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(HL_LIBRARY_DIR ${CMAKE_SOURCE_DIR}/external)

set(SHARED_INCLUDE_DIRS
    ${HL_INCLUDE_DIR}
    ${HL_INCLUDE_DIR}/common
    ${HL_INCLUDE_DIR}/engine
    ${HL_INCLUDE_DIR}/public
    ${SHARED_SRC_DIR}
    ${SERVER_SRC_DIR}/entities
    ${SHARED_SRC_DIR}/utils
    ${SHARED_SRC_DIR}/movement
)

set(SERVER_INCLUDE_DIRS
    ${SERVER_SRC_DIR}
    ${SERVER_SRC_DIR}/entities
    ${SERVER_SRC_DIR}/gamerules
    ${SERVER_SRC_DIR}/bot
    ${SERVER_SRC_DIR}/bot/utils
    ${SHARED_INCLUDE_DIRS}
)

set(CLIENT_INCLUDE_DIRS
    ${CLIENT_SRC_DIR}
    ${CLIENT_SRC_DIR}/input
    ${CLIENT_SRC_DIR}/rendering
    ${CLIENT_SRC_DIR}/rendering/particleman
    ${CLIENT_SRC_DIR}/ui
    ${CLIENT_SRC_DIR}/ui/hud
    ${CLIENT_SRC_DIR}/ui/vgui
    ${CLIENT_SRC_DIR}/ui/vgui/utils
    ${SERVER_SRC_DIR}
    ${SERVER_SRC_DIR}/entities
    ${SERVER_SRC_DIR}/gamerules
    ${HL_LIBRARY_DIR}/SDL2/include
    ${HL_LIBRARY_DIR}/vgui/include
    ${SHARED_INCLUDE_DIRS}
)

set(HL_LIBRARIES
    stdc++
    dl
)

#===============================================================
# Server
#===============================================================

add_library(server SHARED ${SERVER_SRC})

target_compile_definitions(server PRIVATE ${HL_COMPILE_DEFS} GAME_DLL)
target_compile_options(server PRIVATE ${HL_COMPILE_OPTIONS})

target_include_directories(server BEFORE PRIVATE ${SERVER_INCLUDE_DIRS})

target_link_options(server PRIVATE ${HL_LINK_OPTIONS})
target_link_libraries(server ${HL_LIBRARIES})

install(TARGETS server DESTINATION ${CMAKE_INSTALL_PREFIX}/dlls)

#===============================================================
# Client
#===============================================================

if(UNIX)

    # Copy libraries provided by the game to the build directory so we can point the compiler and linker to the files.

    configure_file(
        ${HL_LIBRARY_DIR}/vgui/lib/vgui${CMAKE_SHARED_LIBRARY_SUFFIX}
        ${CMAKE_CURRENT_BINARY_DIR}/vgui${CMAKE_SHARED_LIBRARY_SUFFIX}
        COPYONLY
    )

    configure_file(
        ${HL_LIBRARY_DIR}/SDL2/lib/libSDL2-2.0${CMAKE_SHARED_LIBRARY_SUFFIX}.0
        ${CMAKE_CURRENT_BINARY_DIR}/libSDL2-2.0${CMAKE_SHARED_LIBRARY_SUFFIX}.0
        COPYONLY
    )

endif()

#===============================
# VGUI
#===============================

add_library(vgui SHARED IMPORTED)

if(UNIX)

    set_target_properties(vgui
        PROPERTIES
            IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/vgui${CMAKE_SHARED_LIBRARY_SUFFIX}"
    )

elseif(WIN32)

    set_target_properties(vgui
        PROPERTIES
            IMPORTED_IMPLIB "${HL_LIBRARY_DIR}/SDL2/lib/vgui.lib"
    )

endif()

target_include_directories(vgui INTERFACE ${HL_LIBRARY_DIR}/vgui/include)

#================================
# SDL
#================================

add_library(SDL2 SHARED IMPORTED)

if(UNIX)

    set_target_properties(SDL2
        PROPERTIES
            IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/libSDL2-2.0${CMAKE_SHARED_LIBRARY_SUFFIX}.0"
)

elseif(WIN32)

    set_target_properties(SDL2
        PROPERTIES
            IMPORTED_IMPLIB "${HL_LIBRARY_DIR}/SDL2/lib/SDL2.lib"
    )

endif()

target_include_directories(SDL2 INTERFACE ${HL_LIBRARY_DIR}/SDL2/include)

#===============================
# Client
#===============================

add_library(client SHARED ${CLIENT_SRC})

target_compile_definitions(client PRIVATE ${HL_COMPILE_DEFS} CLIENT_DLL)
target_compile_options(client PRIVATE ${HL_COMPILE_OPTIONS})

target_include_directories(client BEFORE PRIVATE ${CLIENT_INCLUDE_DIRS})

target_link_directories(client BEFORE PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_options(client PRIVATE ${HL_LINK_OPTIONS})
target_link_libraries(client ${HL_LIBRARIES} vgui SDL2)

set_target_properties(server client
    PROPERTIES
        PREFIX ""
)

install(TARGETS client DESTINATION ${CMAKE_INSTALL_PREFIX}/cl_dlls)
