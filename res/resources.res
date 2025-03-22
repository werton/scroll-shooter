
//------------------------------ Background palette ------------------------------------------------
PALETTE bg_pal "map.png"


//------------------------------ Tileset for background --------------------------------------------
TILESET bg_tileset        "map.png"      BEST    ALL


//------------------------------ Background map -----------------------------------------------------
TILEMAP map_def "map.png" bg_tileset  FAST 0
IMAGE mapImage "map.png"  NONE ALL

SPRITE player_sprite      "player.png"        4 4     0 0
SPRITE enemy_sprite       "player.png"        4 4     0 0
SPRITE bullet_sprite      "player.png"        4 4     0 0



//------------------------------ Background map -----------------------------------------------------
WAV    xpcm_shoot         "sounds/shoot.wav"  XGM