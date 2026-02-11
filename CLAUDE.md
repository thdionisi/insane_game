# Snake Bizarre

Jeu de type snake en C avec OpenGL/GLUT et DevIL pour le chargement d'images.

## Compilation

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
Images optionnelles : `enemy_midboss`, `enemy_boss` (sinon enemy_normal est utilise).
Tous les sons sont optionnels (le jeu fonctionne sans).

### Difficulte (`-d`)

- `-d 1` Facile : ennemis lents
- `-d 2` Normal (defaut) : vitesse equilibree
- `-d 3` Difficile : ennemis rapides, la nourriture apparait moins souvent, et les ennemis peuvent la manger (ils grossissent et accelerent)

La vitesse du snake est la meme dans tous les modes (4), ajustable en jeu avec `v`/`b`.

### Types d'ennemis

- **Normal** : petit, lent (tous les ennemis standard)
- **Mid-boss** : moyen, vitesse intermediaire (tous les 8 ennemis)
- **Boss** : gros, rapide (tous les 16 ennemis)

Chaque type peut avoir sa propre texture via le fichier de config.

### Son

7 pistes de musique de fond, de plus en plus dramatiques. La musique change automatiquement tous les 5 points. Effets sonores pour : manger, game over, orbe, collision entre ennemis.

Formats supportes :
- Musique : `.ogg`, `.wav`, `.mp3`, `.flac`, `.mod`
- Effets : `.wav`, `.ogg`

## Controles

- Fleches directionnelles : deplacer le snake
- `v` / `b` : augmenter / diminuer la vitesse du snake
- `F1` / `F2` : augmenter / diminuer la vitesse des ennemis
- `p` : pause
- `r` : reset
- `q` : quitter
