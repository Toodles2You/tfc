About
======================

### This is work-in-progress! Use at your own discretion.

Half-Life SDK with fixes & new features geared toward online play.  
This SDK is derived from [Half-Life Updated](https://github.com/SamVanheer/halflife-updated) with bits from the [Half-Life Unified SDK](https://github.com/SamVanheer/halflife-unified-sdk).

Features
======================

- Networked player & weapon data processed through class methods
- Predicted data passed to the HUD (No more laggy ammo counter!)
- Visual effects handled client-side
    - Gibs won't cause server crashes anymore :)
- Real HUD scaling!
    - Supports forced 4:3 mode, see `hud_widescreen`
    - Fallback for Software renderer
- Improved weapon prediction
    - Weapon events sync up with where you're actually aiming
    - Millisecond integer timers
    - Crosshair compensates for recoil (& third-person) (& concussions)
- Bot system with dummy bots
- Removed Half-Life single-player content
    - Monster code removed (Seriously, *all* of it! No stub classes.)
    - Saves & node graphs are still in-tact
- Improved combat feedback
    - Hit sounds & crosshair hit marker
        - Pitch variance depending upon damage
        - Unique sounds for frags & headshots
    - Death notice
        - Headshot indicator
        - Highlight frags involving the client
- Damage flags to prevent non-sensical headshots
- Spawn teleport effect
- New HL2-esque player movement interface
- C++-ified a lot of C-isms
- Clean project structure
- Team Fortress style grenade priming
- CMake options for toggling features
- Other stuff I'm probably forgetting

Building this SDK
======================

See [BUILDING.md](BUILDING.md)

Half-Life SDK License
======================

See [LICENSE](LICENSE)

