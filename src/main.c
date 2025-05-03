// *****************************************************************************
// Scroll shooter example
//
// Written by werton playskin 05/2025
// GFX artwork created by Luis Zuno (@ansimuz)
// Moon surface artwork and GFX created by mattwalkden.itch.io
// SFX created by JDSherbert jdsherbert.itch.io
// *****************************************************************************

#include <genesis.h>
#include <maths.h>
#include "defs.h"
#include "game_object.h"
#include "game.h"
#include "player.h"
#include "globals.h"
#include "enemy.h"
#include "game_types.h"
#include "enemy_type.h"
#include "explosion.h"
#include "resources.h"


// =============================================
// Main Game Loop
// =============================================

void Game_MainLoop();

int main(bool hardReset) {
    if (!hardReset)
        SYS_hardReset();

    Game_Init();
    Game_MainLoop();
    
    return 0;
}

