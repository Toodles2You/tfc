//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Whatever
//
// $NoKeywords: $
//=============================================================================


#include "tf_defs.h"


PCInfo sTFClassInfo[PC_LASTCLASS] =
{
    [PC_UNDEFINED] = {},

    [PC_SCOUT] = {
        .maxHealth        = 75,
        .maxSpeed         = 400,
        .maxArmor         = 50,
        .initArmor        = 25,
        .maxArmorType     = 0.3,
        .initArmorType    = 0.3,
        .armorClasses     = AT_SAVESHOT | AT_SAVENAIL,
        .initArmorClasses = 0,
        .weapons = {
            WEAPON_NAILGUN,
            WEAPON_TF_SHOTGUN,
            WEAPON_AXE,
            -1,
        },
        .grenades = {
            GRENADE_CALTROP,
            GRENADE_CONCUSSION,
        },
        .maxAmmo = {
            50,
            200,
            25,
            100,
            3,
            3,
        },
        .initAmmo = {
            25,
            100,
            0,
            50,
            2,
            3,
        },
        .model = "scout",
        .modelPath = "models/player/scout/scout.mdl",
        .colormap = {
            {153, 139},
            {255,  10},
            { 45,  35},
            {100,  90},
        },
    },

    [PC_SNIPER] = {
        .maxHealth        = 90,
        .maxSpeed         = 300,
        .maxArmor         = 50,
        .initArmor        = 25,
        .maxArmorType     = 0.3,
        .initArmorType    = 0.3,
        .armorClasses     = AT_SAVESHOT | AT_SAVENAIL,
        .initArmorClasses = 0,
        .weapons = {
            WEAPON_SNIPER_RIFLE,
            WEAPON_AUTO_RIFLE,
            WEAPON_NAILGUN,
            WEAPON_AXE,
        },
        .grenades = {
            GRENADE_NORMAL,
            -1,
        },
        .maxAmmo = {
            75,
            100,
            25,
            50,
            4,
            0,
        },
        .initAmmo = {
            60,
            50,
            0,
            0,
            2,
            0,
        },
        .model = "sniper",
        .modelPath = "models/player/sniper/sniper.mdl",
        .colormap = {
            {153, 145},
            {255,  10},
            { 45,  35},
            { 80,  90},
        },
    },
    
    [PC_SOLDIER] = {
        .maxHealth        = 100,
        .maxSpeed         = 240,
        .maxArmor         = 200,
        .initArmor        = 100,
        .maxArmorType     = 0.8,
        .initArmorType    = 0.8,
        .armorClasses     = AT_SAVESHOT | AT_SAVENAIL | AT_SAVEEXPLOSION | AT_SAVEELECTRICITY | AT_SAVEFIRE,
        .initArmorClasses = 0,
        .weapons = {
            WEAPON_ROCKET_LAUNCHER,
            WEAPON_SUPER_SHOTGUN,
            WEAPON_TF_SHOTGUN,
            WEAPON_AXE,
        },
        .grenades = {
            GRENADE_NORMAL,
            GRENADE_NAIL,
        },
        .maxAmmo = {
            100,
            100,
            50,
            50,
            4,
            2,
        },
        .initAmmo = {
            50,
            0,
            10,
            0,
            2,
            1,
        },
        .model = "soldier",
        .modelPath = "models/player/soldier/soldier.mdl",
        .colormap = {
            {153, 130},
            {250,  28},
            { 45,  35},
            {100,  40},
        },
    },

    [PC_DEMOMAN] = {
        .maxHealth        = 90,
        .maxSpeed         = 280,
        .maxArmor         = 120,
        .initArmor        = 50,
        .maxArmorType     = 0.6,
        .initArmorType    = 0.6,
        .armorClasses     = AT_SAVESHOT | AT_SAVENAIL | AT_SAVEEXPLOSION | AT_SAVEELECTRICITY | AT_SAVEFIRE,
        .initArmorClasses = 0,
        .weapons = {
            WEAPON_GRENADE_LAUNCHER,
            WEAPON_PIPEBOMB_LAUNCHER,
            WEAPON_TF_SHOTGUN,
            WEAPON_AXE,
        },
        .grenades = {
            GRENADE_NORMAL,
            GRENADE_MIRV,
        },
        .maxAmmo = {
            75,
            50,
            50,
            50,
            4,
            2,
        },
        .initAmmo = {
            30,
            0,
            20,
            0,
            2,
            2,
        },
        .model = "demo",
        .modelPath = "models/player/demo/demo.mdl",
        .colormap = {
            {153, 145},
            {255,  20},
            { 45,  35},
            {100,  90},
        },
    },
    
    [PC_MEDIC] = {
        .maxHealth        = 90,
        .maxSpeed         = 320,
        .maxArmor         = 100,
        .initArmor        = 50,
        .maxArmorType     = 0.6,
        .initArmorType    = 0.3,
        .armorClasses     = AT_SAVESHOT | AT_SAVENAIL | AT_SAVEELECTRICITY | AT_SAVEFIRE,
        .initArmorClasses = 0,
        .weapons = {
            WEAPON_MEDIKIT,
            WEAPON_SUPER_NAILGUN,
            WEAPON_SUPER_SHOTGUN,
            WEAPON_TF_SHOTGUN,
        },
        .grenades = {
            GRENADE_NORMAL,
            GRENADE_CONCUSSION,
        },
        .maxAmmo = {
            75,
            150,
            25,
            50,
            4,
            3,
        },
        .initAmmo = {
            50,
            50,
            0,
            0,
            2,
            2,
        },
        .model = "medic",
        .modelPath = "models/player/medic/medic.mdl",
        .colormap = {
            {153, 140},
            {255, 250},
            { 45,  35},
            {100,  90},
        },
    },

    [PC_HVYWEAP] = {
        .maxHealth        = 100,
        .maxSpeed         = 230,
        .maxArmor         = 300,
        .initArmor        = 150,
        .maxArmorType     = 0.8,
        .initArmorType    = 0.8,
        .armorClasses     = AT_SAVESHOT | AT_SAVENAIL | AT_SAVEEXPLOSION | AT_SAVEELECTRICITY | AT_SAVEFIRE,
        .initArmorClasses = 0,
        .weapons = {
            WEAPON_ASSAULT_CANNON,
            WEAPON_SUPER_SHOTGUN,
            WEAPON_TF_SHOTGUN,
            WEAPON_AXE,
        },
        .grenades = {
            GRENADE_NORMAL,
            GRENADE_MIRV,
        },
        .maxAmmo = {
            200,
            200,
            25,
            50,
            4,
            2,
        },
        .initAmmo = {
            200,
            0,
            0,
            30,
            2,
            1,
        },
        .model = "hvyweapon",
        .modelPath = "models/player/hvyweapon/hvyweapon.mdl",
        .colormap = {
            {148, 138},
            {255,  25},
            { 45,  40},
            {100,  90},
        },
    },

    [PC_PYRO] = {
        .maxHealth        = 100,
        .maxSpeed         = 300,
        .maxArmor         = 150,
        .initArmor        = 50,
        .maxArmorType     = 0.6,
        .initArmorType    = 0.6,
        .armorClasses     = AT_SAVESHOT | AT_SAVENAIL | AT_SAVEELECTRICITY | AT_SAVEFIRE,
        .initArmorClasses = AT_SAVEFIRE,
        .weapons = {
            WEAPON_FLAMETHROWER,
            WEAPON_INCENDIARY,
            WEAPON_TF_SHOTGUN,
            WEAPON_AXE,
        },
        .grenades = {
            GRENADE_NORMAL,
            GRENADE_NAPALM,
        },
        .maxAmmo = {
            40,
            50,
            20,
            200,
            4,
            4,
        },
        .initAmmo = {
            20,
            0,
            5,
            120,
            2,
            4,
        },
        .model = "pyro",
        .modelPath = "models/player/pyro/pyro.mdl",
        .colormap = {
            {140, 145},
            {250,  25},
            { 45,  35},
            {100,  50},
        },
    },

    [PC_SPY] = {
        .maxHealth        = 90,
        .maxSpeed         = 300,
        .maxArmor         = 100,
        .initArmor        = 25,
        .maxArmorType     = 0.6,
        .initArmorType    = 0.6,
        .armorClasses     = AT_SAVESHOT | AT_SAVENAIL | AT_SAVEELECTRICITY | AT_SAVEFIRE,
        .initArmorClasses = 0,
        .weapons = {
            WEAPON_KNIFE,
            WEAPON_SUPER_SHOTGUN,
            WEAPON_NAILGUN,
            WEAPON_TRANQ,
        },
        .grenades = {
            GRENADE_NORMAL,
            GRENADE_FLASH,
        },
        .maxAmmo = {
            40,
            100,
            15,
            30,
            4,
            4,
        },
        .initAmmo = {
            40,
            50,
            0,
            10,
            2,
            2,
        },
        .model = "spy",
        .modelPath = "models/player/spy/spy.mdl",
        .colormap = {
            {150, 145},
            {250, 240},
            { 45,  35},
            {100,  90},
        },
    },

    [PC_ENGINEER] = {
        .maxHealth        = 80,
        .maxSpeed         = 300,
        .maxArmor         = 50,
        .initArmor        = 25,
        .maxArmorType     = 0.6,
        .initArmorType    = 0.3,
        .armorClasses     = AT_SAVESHOT | AT_SAVENAIL | AT_SAVEEXPLOSION | AT_SAVEELECTRICITY | AT_SAVEFIRE,
        .initArmorClasses = 0,
        .weapons = {
            WEAPON_SPANNER,
            WEAPON_SUPER_SHOTGUN,
            WEAPON_LASER,
            -1,
        },
        .grenades = {
            GRENADE_NORMAL,
            GRENADE_EMP,
        },
        .maxAmmo = {
            50,
            50,
            30,
            200,
            2,
            4,
        },
        .initAmmo = {
            20,
            25,
            0,
            100,
            2,
            2,
        },
        .model = "engineer",
        .modelPath = "models/player/engineer/engineer.mdl",
        .colormap = {
            {140, 148},
            {  5, 250},
            { 45,  45},
            {100,  90},
        },
    },

    [PC_RANDOM] = {},

    [PC_CIVILIAN] = {
        .maxHealth        = 50,
        .maxSpeed         = 240,
        .maxArmor         = 0,
        .initArmor        = 0,
        .maxArmorType     = 0,
        .initArmorType    = 0,
        .armorClasses     = 0,
        .initArmorClasses = 0,
        .weapons = {
            WEAPON_UMBRELLA,
            -1,
            -1,
            -1,
        },
        .grenades = {
            -1,
            -1,
        },
        .maxAmmo = {
            0,
            0,
            0,
            0,
            0,
            0,
        },
        .initAmmo = {
            0,
            0,
            0,
            0,
            0,
            0,
        },
        .model = "civilian",
        .modelPath = "models/player/civilian/civilian.mdl",
        .colormap = {
            {150, 140},
            {250, 240},
            { 45,  35},
            {100,  90},
        },
    }
};

