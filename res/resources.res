IMAGE bg_title "gfx/bg_title.png"
IMAGE bg_opciones "gfx/bg_opciones.png"
IMAGE bg_game_over "gfx/bg_game_over.png"
IMAGE bg_fin_juego "gfx/bg_fin_juego.png"
IMAGE bg_continue "gfx/bg_continue.png"
IMAGE bg_intro_bosque "gfx/bg_intro_bosque.png"
IMAGE bg_intro_volcan "gfx/bg_intro_volcan.png"
IMAGE bg_intro_acuatico "gfx/bg_intro_acuatico.png"
IMAGE bg_intro_celeste "gfx/bg_intro_celeste.png"
IMAGE bg_como_jugar "gfx/bg_como_jugar.png"
IMAGE bg_cazador_ganador "gfx/bg_cazador_ganador.png"
IMAGE bg_pajaro_ganador "gfx/bg_pajaro_ganador.png"
IMAGE bg_versus "gfx/bg_versus.png"
IMAGE bg_idioma "gfx/bg_idioma.png"
IMAGE bg_hist1 "gfx/bg_hist1.png"
IMAGE bg_hist2 "gfx/bg_hist2.png"
IMAGE bg_hist3 "gfx/bg_hist3.png"
IMAGE bg_hist4 "gfx/bg_hist4.png"
IMAGE bg_opciones_en "gfx/bg_opciones_en.png"
IMAGE bg_como_jugar_en "gfx/bg_como_jugar_en.png"
IMAGE bg_intro_bosque_en "gfx/bg_intro_bosque_en.png"
IMAGE bg_intro_volcan_en "gfx/bg_intro_volcan_en.png"
IMAGE bg_intro_acuatico_en "gfx/bg_intro_acuatico_en.png"
IMAGE bg_intro_celeste_en "gfx/bg_intro_celeste_en.png"
IMAGE bg_fin_juego_en "gfx/bg_fin_juego_en.png"
IMAGE bg_pajaro_ganador_en "gfx/bg_pajaro_ganador_en.png"
IMAGE bg_cazador_ganador_en "gfx/bg_cazador_ganador_en.png"

// ---- Rediseño 16 bits (generado por tools/art/gen_assets.py) ----
PALETTE pal_bosque_maze "gen/bosque_maze.png"
TILESET ts_bosque_maze "gen/bosque_maze.png" NONE NONE
PALETTE pal_bosque_floor "gen/bosque_floor.png"
TILESET ts_bosque_floor "gen/bosque_floor.png" NONE NONE
PALETTE pal_items "gen/items.png"
TILESET ts_items "gen/items.png" NONE NONE
TILESET ts_hud "gen/hud.png" NONE NONE
TILESET ts_font "gen/font.png" NONE NONE
SPRITE hero_sprite "gen/hero.png" 2 2 FAST 0
SPRITE hero_grande_sprite "gen/hero_grande.png" 3 3 FAST 0
SPRITE chick_sprite "gen/chick.png" 2 2 FAST 8
SPRITE hunter_sprite "gen/hunter.png" 3 3 FAST 0
TILESET ts_chick_anim "gen/chick_tiles.png" NONE NONE
SPRITE spark_sprite "gen/spark.png" 2 2 FAST 4
TILESET ts_font_menu "gen/font_menu.png" NONE NONE
PALETTE pal_volcan_maze "gen/volcan_maze.png"
TILESET ts_volcan_maze "gen/volcan_maze.png" NONE NONE
PALETTE pal_volcan_floor "gen/volcan_floor.png"
TILESET ts_volcan_floor "gen/volcan_floor.png" NONE NONE
PALETTE pal_marino_maze "gen/marino_maze.png"
TILESET ts_marino_maze "gen/marino_maze.png" NONE NONE
PALETTE pal_marino_floor "gen/marino_floor.png"
TILESET ts_marino_floor "gen/marino_floor.png" NONE NONE
PALETTE pal_celeste_maze "gen/celeste_maze.png"
TILESET ts_celeste_maze "gen/celeste_maze.png" NONE NONE
PALETTE pal_celeste_floor "gen/celeste_floor.png"
TILESET ts_celeste_floor "gen/celeste_floor.png" NONE NONE
SPRITE flame_sprite "gen/flame.png" 3 3 FAST 0
SPRITE shark_sprite "gen/shark.png" 3 3 FAST 0
SPRITE imp_sprite "gen/imp.png" 3 3 FAST 0

// ---- Fase 5: sonido (tools/sound) ----
XGM2 musica "snd/titulo.vgm" "snd/menu.vgm" "snd/bosque.vgm" "snd/volcan.vgm" "snd/marino.vgm" "snd/celeste.vgm" "snd/intro.vgm" "snd/superada.vgm" "snd/gameover.vgm" "snd/victoria.vgm" "snd/continuar.vgm" "snd/jefe.vgm"
WAV sfx_click "snd/sfx_click.wav" XGM2
WAV sfx_pio "snd/sfx_pio.wav" XGM2
WAV sfx_nido0 "snd/sfx_nido0.wav" XGM2
WAV sfx_nido1 "snd/sfx_nido1.wav" XGM2
WAV sfx_nido2 "snd/sfx_nido2.wav" XGM2
WAV sfx_nido3 "snd/sfx_nido3.wav" XGM2
WAV sfx_nido4 "snd/sfx_nido4.wav" XGM2
WAV sfx_gema "snd/sfx_gema.wav" XGM2
WAV sfx_reloj "snd/sfx_reloj.wav" XGM2
WAV sfx_rayo "snd/sfx_rayo.wav" XGM2
WAV sfx_meta "snd/sfx_meta.wav" XGM2
WAV sfx_cierre "snd/sfx_cierre.wav" XGM2
WAV sfx_alerta "snd/sfx_alerta.wav" XGM2
WAV sfx_panico "snd/sfx_panico.wav" XGM2
WAV sfx_atrapado "snd/sfx_atrapado.wav" XGM2
WAV sfx_rabia "snd/sfx_rabia.wav" XGM2

// ---- Ajustes finales: huevos, efectos, jefes ----
SPRITE huevo_sprite "gen/huevo.png" 2 2 FAST 3
SPRITE estrellas_sprite "gen/estrellas.png" 3 1 FAST 0
SPRITE popup_sprite "gen/popup.png" 4 1 FAST 0
SPRITE polvo_sprite "gen/polvo.png" 2 2 FAST 4
SPRITE proyectil_sprite "gen/proyectil.png" 2 2 FAST 6
SPRITE ambiente_sprite "gen/ambiente.png" 2 2 FAST 6
SPRITE recarga_sprite "gen/recarga.png" 2 1 FAST 0
SPRITE jefe_cazador "gen/jefe_cazador.png" 6 6 FAST 0
SPRITE jefe_llama "gen/jefe_llama.png" 6 6 FAST 0
SPRITE jefe_tiburon "gen/jefe_tiburon.png" 6 6 FAST 0
SPRITE jefe_diablillo "gen/jefe_diablillo.png" 6 6 FAST 0
WAV sfx_lanzar "snd/sfx_lanzar.wav" XGM2
WAV sfx_chof "snd/sfx_chof.wav" XGM2
WAV sfx_aturdido "snd/sfx_aturdido.wav" XGM2
WAV sfx_golpe "snd/sfx_golpe.wav" XGM2
WAV sfx_explosion "snd/sfx_explosion.wav" XGM2
WAV sfx_unavida "snd/sfx_unavida.wav" XGM2
WAV sfx_teleport "snd/sfx_teleport.wav" XGM2
WAV sfx_red "snd/sfx_red.wav" XGM2
