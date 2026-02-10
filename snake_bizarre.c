#include <stdio.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <GL/glut.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <IL/il.h>

/* ========== Constants ========== */

#define MAX_ENEMIES 100
#define START_ENEMIES 1
#define SMALL_SIZE 0.05f
#define BIG_SIZE 0.1f

#define DIR_UP    0
#define DIR_DOWN  1
#define DIR_LEFT  2
#define DIR_RIGHT 3

/* ========== Types ========== */

typedef struct {
	float xleft;
	float xright;
	float ydown;
	float yup;
	float speed;
} Rect;

/* ========== Screen dimensions (set at runtime) ========== */

int screen_width;
int screen_height;

/* ========== Textures ========== */

GLuint tex_enemy[MAX_ENEMIES];
GLuint tex_food[MAX_ENEMIES];
GLuint tex_snake;

/* ========== Game state ========== */

int window;
int num_enemies = START_ENEMIES;
int direction = DIR_UP;
int paused = 0;
int game_over = 0;
int food_eaten = 0;
int draw_collision_effect = 0;
int collision_timer = 0;
int respawn_food = 1;
int food_cycle = 0;
int food_x = 1000;
int food_y = 1000;
int points = 0;
int snake_length = 1;
int food_timer = 600;

/* ========== Movement / speed ========== */

float snake_x = 0;
float snake_y = 0;
float snake_speed = 7;
float slow_enemy_speed = 3;
float fast_enemy_speed = 6;
float enemy_gap;

/* ========== Per-enemy arrays ========== */

float enemy_dist_y[MAX_ENEMIES];
float enemy_dist_x[MAX_ENEMIES];
float last_dy[MAX_ENEMIES];
float last_dx[MAX_ENEMIES];
float enemy_size[MAX_ENEMIES];
float enemy_speed[MAX_ENEMIES];
int   enemy_dir_y[MAX_ENEMIES];
int   enemy_dir_x[MAX_ENEMIES];

/* ========== Entity rects ========== */

Rect snake[100];
Rect enemy[MAX_ENEMIES];

/* ========== Utility functions ========== */

double rand_range(double a, double b)
{
	return (rand() / (double)RAND_MAX) * (b - a) + a;
}

void bitmap_text(int x, int y, char *string, void *font)
{
	int len, i;
	glRasterPos2f(x, y);
	len = (int)strlen(string);
	for (i = 0; i < len; i++)
		glutBitmapCharacter(font, string[i]);
}

/* ========== Texture loading ========== */

void bind_texture(GLuint *tex)
{
	glGenTextures(1, tex);
	glBindTexture(GL_TEXTURE_2D, *tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, ilGetInteger(IL_IMAGE_BPP),
		ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT),
		0, ilGetInteger(IL_IMAGE_FORMAT), GL_UNSIGNED_BYTE, ilGetData());
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
}

int load_image(char *filename)
{
	ILuint image;
	ILboolean success;

	ilGenImages(1, &image);
	ilBindImage(image);

	if ((success = ilLoadImage(filename))) {
		success = ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
		if (!success)
			return -1;
	} else {
		return -1;
	}
	return image;
}

/* ========== Collision ========== */

int collision(Rect a, Rect b)
{
	if (a.xright < b.xleft) return 0;
	if (b.xright < a.xleft) return 0;
	if (a.yup < b.ydown) return 0;
	if (b.yup < a.ydown) return 0;
	return 1;
}

float distance(Rect a, Rect b)
{
	float dx = a.xright - b.xright;
	float dy = a.yup - b.yup;
	return sqrtf(dx * dx + dy * dy);
}

/* ========== Enemy management ========== */

void set_enemy_size_and_speed(int i)
{
	if (i % 8 == 0 && i != 0) {
		enemy_size[i] = BIG_SIZE;
		enemy_speed[i] = fast_enemy_speed;
	} else {
		enemy_size[i] = SMALL_SIZE;
		enemy_speed[i] = slow_enemy_speed;
	}
}

void add_enemy(void)
{
	int i = num_enemies - 1;
	float dist = 0;

	set_enemy_size_and_speed(i);

	while (dist < screen_width / 5) {
		enemy_dist_y[i] = rand_range(1, screen_height);
		enemy_dist_x[i] = rand_range(1, screen_width);

		float xleft = -screen_width / 2 + enemy_dist_x[i] - enemy_size[i] * screen_height;
		float xright = -screen_width / 2 + enemy_dist_x[i];
		float ydown = enemy_dist_y[i] - (screen_height / 2 - enemy_size[i] * screen_height);
		float yup = enemy_dist_y[i] - (screen_height / 2 - 2 * enemy_size[i] * screen_height);

		enemy[i].xleft = xleft;
		enemy[i].xright = xright;
		enemy[i].ydown = ydown;
		enemy[i].yup = yup;
		dist = distance(enemy[i], snake[0]);
	}

	enemy_dir_y[i] = (rand_range(1, 2) == 1) ? 1 : -1;
	enemy_dir_x[i] = (rand_range(1, 2) == 1) ? 1 : -1;
}

/* ========== Reset ========== */

void reset(void)
{
	int i;

	snake_x = 0;
	snake_y = 0;
	snake_speed = 7;
	num_enemies = START_ENEMIES;
	enemy_gap = screen_width / 5;

	for (i = 0; i < num_enemies; i++) {
		enemy_dist_y[i] = i * enemy_gap;
		enemy_dist_x[i] = i * enemy_gap;
		enemy_dir_y[i] = 1;
		enemy_dir_x[i] = 1;
		enemy_speed[i] = 3;
		enemy_size[i] = SMALL_SIZE;
	}
}

/* ========== Drawing helpers ========== */

static void draw_quad(float x1, float y1, float x2, float y2)
{
	glVertex2d(x1, y2);
	glVertex2d(x2, y2);
	glVertex2d(x2, y1);
	glVertex2d(x1, y1);
}

static void draw_textured_quad(GLuint tex, float x1, float y1, float x2, float y2)
{
	glEnable(GL_TEXTURE_2D);
	glMatrixMode(GL_MODELVIEW);
	glBindTexture(GL_TEXTURE_2D, tex);
	glBegin(GL_QUADS);
	glTexCoord2i(0, 0); glVertex2d(x1, y2);
	glTexCoord2i(1, 0); glVertex2d(x2, y2);
	glTexCoord2i(1, 1); glVertex2d(x2, y1);
	glTexCoord2i(0, 1); glVertex2d(x1, y1);
	glEnd();
	glDisable(GL_TEXTURE_2D);
}

/* ========== Display callback ========== */

void display(void)
{
	int i;
	float W = screen_width;
	float H = screen_height;
	float cell = SMALL_SIZE * H;

	if (paused)
		return;

	glClearColor(0.5f, 0.0f, 0.4f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/* Bottom band */
	glBegin(GL_QUADS);
	glColor3f(0.75f, 0.75f, 0.15f);
	draw_quad(-W / 2, -H / 2, W / 2, -(H / 2 - cell));
	glEnd();

	/* Middle band */
	glBegin(GL_QUADS);
	glColor3f(0.50f, 0.70f, 0.90f);
	draw_quad(-W / 2, -(H / 2 - cell), W / 2, -(H / 2 - 2 * cell));
	glEnd();

	/* Top band */
	glBegin(GL_QUADS);
	glColor3f(0.35f, 0.15f, 0.75f);
	draw_quad(-W / 2, -(H / 2 - 2 * cell), W / 2, -(H / 2 - 3 * cell));
	glEnd();

	/* Clamp snake position */
	if (snake_x <= -(W - cell) / 2) snake_x = -(W - cell) / 2;
	if (snake_x >= (W - cell) / 2)  snake_x = (W - cell) / 2;
	if (snake_y <= 0)               snake_y = 0;
	if (snake_y >= H - cell)        snake_y = H - cell;

	float sx1 = -cell / 2 + snake_x;
	float sx2 = cell / 2 + snake_x;
	float sy1 = -H / 2 + snake_y;
	float sy2 = -H / 2 + cell + snake_y;

	snake[0].xleft = sx1;
	snake[0].xright = sx2;
	snake[0].ydown = sy1;
	snake[0].yup = sy2;

	/* Draw snake */
	draw_textured_quad(tex_snake, sx1, sy1, sx2, sy2);

	/* Food position */
	int fx, fy;
	if (respawn_food) {
		fx = rand_range(cell, W - cell);
		fy = rand_range(1, H - cell);
		food_x = fx;
		food_y = fy;
	} else {
		fx = food_x;
		fy = food_y;
	}

	float fl = -W / 2 + fx - cell;
	float fr = -W / 2 + fx;
	float fu = fy - H / 2 + cell;
	float fd = fy - H / 2;
	if (fu >= H / 2) fu = H / 2;

	Rect food;
	food.xleft = fl;
	food.xright = fr;
	food.ydown = fd;
	food.yup = fu;

	/* Check food collision before drawing */
	if (collision(snake[0], food)) {
		snake_length++;
		food_eaten = 1;
	}

	/* Draw food */
	draw_textured_quad(tex_food[0], fl, fd, fr, fu);

	/* Enemies */
	for (i = 0; i < num_enemies; i++) {
		float sz = enemy_size[i] * H;

		if (enemy_dist_y[i] >= H - 2 * enemy_size[i] * H)
			enemy_dist_y[i] = H - 2 * enemy_size[i] * H;
		if (enemy_dist_y[i] <= -enemy_size[i] * H)
			enemy_dist_y[i] = -enemy_size[i] * H;
		if (enemy_dist_x[i] >= W)
			enemy_dist_x[i] = W;
		if (enemy_dist_x[i] <= enemy_size[i] * H)
			enemy_dist_x[i] = enemy_size[i] * H;

		float ex1 = -W / 2 + enemy_dist_x[i] - sz;
		float ex2 = -W / 2 + enemy_dist_x[i];
		float ey1 = enemy_dist_y[i] - (H / 2 - sz);
		float ey2 = enemy_dist_y[i] - (H / 2 - 2 * sz);

		if (ex1 <= -W / 2) enemy_dir_x[i] = 1;
		if (ex2 >= W / 2)  enemy_dir_x[i] = -1;
		if (ey1 <= -H / 2) enemy_dir_y[i] = 1;
		if (ey2 >= H / 2)  enemy_dir_y[i] = -1;

		enemy[i].xleft = ex1;
		enemy[i].xright = ex2;
		enemy[i].ydown = ey1;
		enemy[i].yup = ey2;

		draw_textured_quad(tex_enemy[0], ex1, ey1, ex2, ey2);

		/* Enemy-enemy collision */
		int j;
		for (j = 0; j < i; j++) {
			if (collision(enemy[j], enemy[i])) {
				if (sx1 >= ex1 && direction != DIR_RIGHT) {
					enemy_dir_x[i] = -1;
					enemy_dir_x[j] = 1;
					draw_collision_effect = 1;
				}
				if (sy1 >= ey1 && direction != DIR_UP) {
					enemy_dir_y[i] = -1;
					enemy_dir_y[j] = 1;
					draw_collision_effect = 1;
				}
				if (sy2 <= ey2 && direction != DIR_DOWN) {
					enemy_dir_y[i] = 1;
					enemy_dir_y[j] = -1;
					draw_collision_effect = 1;
				}
				if (sx2 <= ex2 && direction != DIR_LEFT) {
					enemy_dir_x[i] = 1;
					enemy_dir_x[j] = -1;
					draw_collision_effect = 1;
				}
			}
		}

		/* Collision effect flash */
		if (draw_collision_effect) {
			glBegin(GL_QUADS);
			glColor3f(0.65f, 0.1f, 0.76f);
			glVertex2d(-20, -20);
			glVertex2d(20, 20);
			glVertex2d(-20, 20);
			glVertex2d(20, -20);
			glEnd();
		}

		/* Snake eats food */
		if (collision(snake[0], food))
			food_eaten = 1;

		/* Enemy kills snake */
		if (collision(enemy[i], snake[0])) {
			char buffer[256] = {'\0'};
			sprintf(buffer, "%d", points);

			if (points < 5)
				strcat(buffer, " points... C'est lamentable, affligeant, pitoyable...");
			else if (points < 10)
				strcat(buffer, " points... Tu devrais essayer la bataille ou les petits chevaux.");
			else if (points < 15)
				strcat(buffer, " points... Essaye encore !");
			else if (points < 20)
				strcat(buffer, " points. L'Histoire ne retiendra malheureusement pas cette partie.");
			else if (points < 25)
				strcat(buffer, " points. Pas mal, mais il en faut plus pour m'impressionner !");
			else if (points < 30)
				strcat(buffer, " points ! Tu as un certain talent !");
			else
				strcat(buffer, " points ! Quel talent inoui ! Tu deviendras un grand...");

			glColor3f(0.95f, 0.0f, 0.0f);
			bitmap_text(-250, 20, buffer, GLUT_BITMAP_TIMES_ROMAN_24);
			game_over = 1;
		}
	}

	glutSwapBuffers();
}

/* ========== Idle callback ========== */

void idle(void)
{
	int i;

	if (paused)
		return;

	if (food_eaten) {
		num_enemies++;
		food_eaten = 0;
		points++;
		food_cycle = food_timer - 1;
		add_enemy();
	}

	for (i = 0; i < num_enemies; i++) {
		if (food_cycle % 10 == 0) {
			int prev_y = enemy_dist_y[i];
			int prev_x = enemy_dist_x[i];
			enemy_dist_y[i] += enemy_dir_y[i] * rand_range(enemy_speed[i] / 3, enemy_speed[i]);
			enemy_dist_x[i] += enemy_dir_x[i] * rand_range(enemy_speed[i] / 3, enemy_speed[i]);
			last_dy[i] = enemy_dist_y[i] - prev_y;
			last_dx[i] = enemy_dist_x[i] - prev_x;
		} else {
			enemy_dist_y[i] += last_dy[i];
			enemy_dist_x[i] += last_dx[i];
		}
	}

	if (direction == DIR_UP)    snake_y += snake_speed;
	if (direction == DIR_DOWN)  snake_y -= snake_speed;
	if (direction == DIR_LEFT)  snake_x -= snake_speed;
	if (direction == DIR_RIGHT) snake_x += snake_speed;

	food_cycle++;
	if (food_cycle == food_timer) {
		food_cycle = 0;
		respawn_food = 1;
	} else {
		respawn_food = 0;
	}

	if (draw_collision_effect)
		collision_timer++;
	if (collision_timer == 100) {
		collision_timer = 0;
		draw_collision_effect = 0;
	}

	glutPostRedisplay();

	if (game_over) {
		sleep(2);
		glFinish();
		glutDestroyWindow(window);
	}
}

/* ========== Reshape callback ========== */

void reshape(int w, int h)
{
	screen_width = w;
	screen_height = h;

	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(-w / 2.0, w / 2.0, -h / 2.0, h / 2.0);
	glMatrixMode(GL_MODELVIEW);
	glutPostRedisplay();
}

/* ========== Keyboard callbacks ========== */

void keypress(unsigned char key, int x, int y)
{
	(void)x; (void)y;
	switch (key) {
	case 'q':
		glFinish();
		glutDestroyWindow(window);
		break;
	case 'r':
		reset();
		break;
	case 'v':
		snake_speed *= 1.3f;
		break;
	case 'b':
		snake_speed /= 1.3f;
		break;
	case 'p':
		paused = !paused;
		break;
	}
}

void special_keys(int key, int x, int y)
{
	(void)x; (void)y;
	int i;
	switch (key) {
	case GLUT_KEY_UP:    direction = DIR_UP;    break;
	case GLUT_KEY_DOWN:  direction = DIR_DOWN;  break;
	case GLUT_KEY_LEFT:  direction = DIR_LEFT;  break;
	case GLUT_KEY_RIGHT: direction = DIR_RIGHT; break;
	case GLUT_KEY_F1:
		slow_enemy_speed *= 1.3f;
		fast_enemy_speed *= 1.3f;
		for (i = 0; i < num_enemies; i++)
			set_enemy_size_and_speed(i);
		break;
	case GLUT_KEY_F2:
		slow_enemy_speed /= 1.3f;
		fast_enemy_speed /= 1.3f;
		for (i = 0; i < num_enemies; i++)
			set_enemy_size_and_speed(i);
		break;
	}
}

/* ========== Difficulty selection ========== */

void select_difficulty(void)
{
	int choice;

	printf("\n=== SNAKE BIZARRE ===\n");
	printf("Choisis la difficulte :\n");
	printf("  1 - Facile\n");
	printf("  2 - Normal\n");
	printf("  3 - Difficile\n");
	printf("Ton choix : ");

	if (scanf("%d", &choice) != 1)
		choice = 2;

	switch (choice) {
	case 1:
		snake_speed = 4;
		slow_enemy_speed = 2;
		fast_enemy_speed = 4;
		printf("Mode Facile selectionne.\n\n");
		break;
	case 3:
		snake_speed = 11;
		slow_enemy_speed = 5;
		fast_enemy_speed = 9;
		printf("Mode Difficile selectionne.\n\n");
		break;
	default:
		snake_speed = 7;
		slow_enemy_speed = 3;
		fast_enemy_speed = 6;
		printf("Mode Normal selectionne.\n\n");
		break;
	}
}

/* ========== Main ========== */

int main(int argc, char *argv[])
{
	int img_snake, img_enemy, img_food;

	srand(time(NULL));

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);

	/* Get screen size and go fullscreen */
	screen_width = glutGet(GLUT_SCREEN_WIDTH);
	screen_height = glutGet(GLUT_SCREEN_HEIGHT);
	if (screen_width == 0) screen_width = 1920;
	if (screen_height == 0) screen_height = 1080;

	reset();
	select_difficulty();

	window = glutCreateWindow("Snake Bizarre");
	glutFullScreen();

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);

	if (ilGetInteger(IL_VERSION_NUM) < IL_VERSION) {
		printf("Wrong DevIL version\n");
		return -1;
	}
	ilInit();

	/* Load textures: argv[1]=snake, argv[2]=enemy, argv[3]=food */
	if (argc < 4) {
		printf("Usage: %s <image_snake> <image_ennemi> <image_nourriture>\n", argv[0]);
		return -1;
	}

	img_snake = load_image(argv[1]);
	bind_texture(&tex_snake);
	img_enemy = load_image(argv[2]);
	bind_texture(&tex_enemy[0]);
	img_food = load_image(argv[3]);
	bind_texture(&tex_food[0]);

	if (img_snake == -1 || img_enemy == -1 || img_food == -1) {
		printf("Erreur de chargement des images.\n");
		printf("Usage: %s <image_snake> <image_ennemi> <image_nourriture>\n", argv[0]);
		return -1;
	}

	glutIdleFunc(idle);
	glutKeyboardFunc(keypress);
	glutSpecialFunc(special_keys);
	glutMainLoop();

	return 0;
}
