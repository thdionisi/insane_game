# Snake Bizarre

Jeu de type snake en C avec OpenGL/GLUT et DevIL pour le chargement d'images.

## Compilation

```bash
make
```

Ou manuellement :

```bash
gcc snake_bizarre.c -o snake_bizarre -lGL -lGLU -lglut -lIL -lm $(sdl2-config --cflags --libs) -lSDL2_mixer
```

### Dependances (Ubuntu/Debian)

```bash
sudo apt-get install libgl-dev libglu1-mesa-dev freeglut3-dev libdevil-dev libsdl2-mixer-dev
```

## Utilisation

```bash
./snake_bizarre [-d 1|2|3] [-c config.cfg]
```

Le jeu lit un fichier de configuration (`snake_bizarre.cfg` par defaut) qui contient les chemins vers les images et les sons. Voir `snake_bizarre.cfg` pour le format.

Images obligatoires dans le config : `snake`, `food`, `enemy_normal`.
Images optionnelles : `enemy_midboss`, `enemy_boss`, `blackhole` (sinon textures generees).
Tous les sons sont optionnels (le jeu fonctionne sans).

### Difficulte (`-d`)

- `-d 1` Facile : ennemis lents
- `-d 2` Normal (defaut) : vitesse equilibree
- `-d 3` Difficile : ennemis rapides, la nourriture apparait moins souvent, et les ennemis peuvent la manger (ils grossissent et accelerent)

La vitesse du snake est la meme dans tous les modes (4), ajustable en jeu avec `v`/`b`.

### Types d'ennemis

- **Normal** : petit, lent (tous les ennemis standard)
- **Mid-boss** : moyen, vitesse intermediaire (tous les 4 ennemis)
- **Boss** : gros, rapide (tous les 10 ennemis)

Chaque type peut avoir sa propre texture via le fichier de config.

### Orbe

Apparait lors des collisions entre ennemis. Projette le snake ou un ennemi dans une direction avec un boost de vitesse progressivement decroissant.

### Power-ups

Apparaissent regulierement apres 3 points (40% de chance a chaque nourriture mangee). Le type (bouclier ou trou noir) est choisi aleatoirement et chacun a un visuel distinct sur la carte. Les ennemis peuvent aussi manger les power-ups. Ramassez-les pour stocker un pouvoir, puis activez-le avec `c` :

- **Bouclier** (3s) : aura verte autour du snake. Repousse les ennemis et les blesse (retrecit + ralentit). Les plus petits meurent.
- **Trou noir** : pose un vortex fixe a la position actuelle du snake (10s). Attire les ennemis proches, tue les petits non-nourris (avec animation de mort), blesse les autres. Attention : le snake meurt aussi s'il repasse dessus.

### Son

7 pistes de musique de fond, de plus en plus dramatiques. La musique change automatiquement tous les 11 points. Effets sonores pour : manger, game over, orbe, power-up, bouclier, trou noir.

Formats supportes :
- Musique : `.ogg`, `.wav`, `.mp3`, `.flac`, `.mod`
- Effets : `.wav`, `.ogg`, `.mp3`

## Controles

- Fleches directionnelles : deplacer le snake
- `v` / `b` : augmenter / diminuer la vitesse du snake
- `F1` / `F2` : augmenter / diminuer la vitesse des ennemis
- `c` : activer le power-up stocke
- `p` : pause
- `r` : reset
- `q` : quitter
