# Flicki's Escape (SEGA Mega Drive / Genesis)

[![Platform: Sega Mega Drive](https://img.shields.io/badge/Platform-Sega%20Mega%20Drive-blue.svg)](https://en.wikipedia.org/wiki/Sega_Genesis)
[![Language: C](https://img.shields.io/badge/Language-C-green.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Play on itch.io](https://img.shields.io/badge/Play-itch.io-fa5c5c.svg)](https://yupikaie.itch.io/flickies-escape)
[![Blog](https://img.shields.io/badge/Blog-RetroPlayingBCN-1a1a2e.svg)](https://www.retroplayingbcn.es/2026/07/flickis-escape-mega-drive-ia.html)

**Flicki's Escape** es un videojuego homebrew completamente original desarrollado para la legendaria consola de 16 bits **SEGA Mega Drive / Genesis**.

El proyecto nace como un cariñoso tributo que fusiona las mecánicas de recolección de *Flicky* con la tensión laberíntica de persecución de *Pac-Man*. Lo más destacado de este desarrollo es que **el 100% del código fuente en C ha sido estructurado, depurado y optimizado mediante la colaboración directa con modelos de Inteligencia Artificial**.

📝 Puedes leer el making-of completo del proyecto en **[RetroPlayingBCN](https://www.retroplayingbcn.es/2026/07/flickis-escape-mega-drive-ia.html)**, y jugar la última build directamente en **[itch.io](https://yupikaie.itch.io/flickies-escape)**.

---

## 🤖 El Hito Tecnológico: Desarrollo Guiado por IA

Programar para hardware de 1989 (procesador Motorola 68000) impone severas restricciones: gestión manual de memoria RAM, paletas de color limitadas y gestión estricta de sprites para evitar parpadeos.

Este repositorio demuestra cómo una IA puede comprender arquitecturas de hardware retro y diseñar código optimizado en C bajo el kit **SGDK**:

*   **Físicas de colisión por tiles:** Rutinas de colisión píxel a píxel adaptadas a una rejilla de metatiles de 16x16.
*   **Algoritmo BFS (Breadth-First Search):** mapa de distancias por búsqueda en anchura para que el enemigo persiga al jugador; cada enemigo tiene su personalidad (la llama corre más, el tiburón embiste en línea recta y el diablillo se teletransporta).
*   **Generación Procedimental:** Creación aleatoria de mapas garantizando que todos los elementos sean alcanzables mediante análisis topológico en tiempo real.

---

## 🎮 Características y Modos de Juego

*   **Aspecto de 16 bits:**
    *   Setos, rocas volcánicas, arrecifes y nubes con autotiles.
    *   Personajes chibi originales con animación.
    *   Título, menús, intros y finales en pixel art.
*   **Intro tipo arcade:** una pequeña historia en cuatro viñetas que da contexto al juego. Se repite si dejas el título quieto.
*   **Castellano e inglés:**
    *   El idioma se elige al arrancar; también se cambia con MODE en el menú (mando de 6 botones).
    *   El idioma y el récord se guardan en la SRAM del cartucho.
*   **Música FM y efectos PCM** originales, con el driver XGM2 de SGDK.
*   **Efectos:** fundidos, marcador fijo con scroll por líneas, olas en el mar, lava y estrellas con ciclo de paleta, estrellitas al recibir un golpe, polluelos que salen volando, puntos flotantes, polvo y un nido que se va llenando.

*   **Modo Historia:** 4 mundos temáticos (Bosque, Volcán, Mar y Cielo) de 6 fases, cada uno con su escenario y su enemigo: cazador, llama, tiburón y diablillo.
*   **Jefes:** la sexta fase de cada mundo es un combate contra el enemigo gigante, con barra de vida, un ataque propio y música de jefe. Se le vence a huevazos.
*   **Huevos:** con A o B lanzas un huevo que deja aturdido al enemigo 3 segundos y le corta la rabia. Llevas hasta 3 y aparecen por el mapa.
*   **Vidas:** cada fase superada da una vida, y también cada 5000 puntos.
*   **Modo Libre:** Generación procedural de laberintos jugables sobre la marcha.
*   **Modo Versus (2 Jugadores):** Acción multijugador local en el que el segundo jugador controla directamente al cazador.
*   **Mecánica de Dispersión:** Al recibir un golpe llevando polluelos, estos se esparcirán por el mapa y ganarás unos segundos de inmunidad para recuperarlos.
*   **Modo Rabia:** El cazador duplica su velocidad durante 8 segundos y anula tu protección por polluelos. ¡Un solo toque es muerte instantánea!
*   **Power-Ups:** Relojes de ralentización, rayos de velocidad y gemas de puntuación para conseguir vidas extra.

---

## 🔗 Enlaces

*   🎮 **Jugar / descargar:** [itch.io](https://yupikaie.itch.io/flickies-escape)
*   📝 **Artículo del desarrollo:** [RetroPlayingBCN](https://www.retroplayingbcn.es/2026/07/flickis-escape-mega-drive-ia.html)
*   🌐 **Más contenido retro:** [retroplayingbcn.es](https://www.retroplayingbcn.es)

---

## 🛠️ Cómo Compilar el Proyecto

Para compilar el código fuente y generar tu propia ROM ejecutable (`.bin`), necesitarás tener configurado el **Sega Genesis Development Kit (SGDK)** de Stephane Dallongeville.

1. Clona este repositorio en tu máquina local:

```bash
   git clone https://github.com/Retroplayingbcn/flickis-escape.git
```
Abre tu terminal de comandos en la carpeta raíz del proyecto.

Ejecuta la herramienta de compilación de SGDK:

```Bash
%GDK%\bin\make -f %GDK%\makefile.gen
```
Encontrarás la ROM final lista para emular en out/rom.bin.

### Regenerar gráficos y sonido

Todo el arte y el audio salen de scripts, así que se pueden retocar sin editores externos:

```bash
python3 tools/art/gen_assets.py      # laberintos, personajes, objetos, marcador y fuentes -> res/gen/
python3 tools/art/screens.py         # título, menús, historia, intros y finales -> res/gfx/
python3 tools/sound/samples.py       # batería y efectos de sonido -> res/snd/, tools/sound/pcm/
MMLC=/ruta/a/mmlc tools/sound/build_music.sh   # música: compose.py -> MML -> VGM (ctrmml)
```

Si compilas con el `m68k-linux-gnu-gcc` de Linux en lugar del toolchain de SGDK, añade `-mstrict-align -fno-store-merging`. Sin esas opciones, GCC genera accesos de 16/32 bits a direcciones impares, que un 68000 real no admite.
