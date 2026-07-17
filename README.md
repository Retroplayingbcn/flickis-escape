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

*   **Físicas de colisión por tiles:** Rutinas de colisión píxel a píxel adaptadas a rejillas de 8x8.
*   **Algoritmo BFS (Breadth-First Search):** Implementación de una búsqueda en anchura optimizada en memoria para que la CPU del cazador persiga al jugador de forma inteligente.
*   **Generación Procedimental:** Creación aleatoria de mapas garantizando que todos los elementos sean alcanzables mediante análisis topológico en tiempo real.

---

## 🎮 Características y Modos de Juego

*   **Modo Historia:** 4 mundos temáticos (Bosque, Volcán, Escenario Marino y Celeste) con sus respectivos enemigos y paletas personalizadas.
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
