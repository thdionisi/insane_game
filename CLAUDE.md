# Snake Bizarre

Jeu de type snake en C avec OpenGL/GLUT et DevIL pour le chargement d'images.

## Compilation

```bash
gcc snake_bizarre.c -o snake_bizarre -lGL -lGLU -lglut -lIL -lm
```

### Dependances (Ubuntu/Debian)

```bash
sudo apt-get install libgl-dev libglu1-mesa-dev freeglut3-dev libdevil-dev
```

## Utilisation

```bash
./snake_bizarre <image_snake> <image_ennemi> <image_nourriture>
```

Les 3 arguments sont des fichiers image (PNG, JPG...) charges via DevIL :
1. Image du snake (le personnage)
2. Image de l'ennemi (les monstres)
3. Image de la nourriture (ce que le snake doit manger)

Au lancement, un menu dans le terminal propose 3 niveaux de difficulte (Facile, Normal, Difficile) qui ajustent la vitesse du snake et des ennemis. La fenetre s'ouvre en plein ecran.

## Controles

- Fleches directionnelles : deplacer le snake
- `v` / `b` : augmenter / diminuer la vitesse du snake
- `F1` / `F2` : augmenter / diminuer la vitesse des ennemis
- `p` : pause
- `r` : reset
- `q` : quitter
