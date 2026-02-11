#include <stdio.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <GL/glut.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <IL/il.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

/* ========== Constants ========== */

#define MAX_ENEMIES 100
#define START_ENEMIES 1
#define SMALL_SIZE 0.05f
#define MIDBOSS_SIZE 0.08f
#define BOSS_SIZE 0.12f
#define MAX_ENEMY_SIZE 0.25f

#define DIR_UP    0
#define DIR_DOWN  1
#define DIR_LEFT  2
#define DIR_RIGHT 3

#define DIFF_EASY   1
#define DIFF_NORMAL 2
#define DIFF_HARD   3

#define ENEMY_NORMAL  0
#define ENEMY_MIDBOSS 1
#define ENEMY_BOSS    2

#define ORB_RADIUS 30.0f
#define ORB_PUSH_FORCE 300.0f
#define ORB_ANIM_DURATION 400

#define MAX_MUSIC 7
#define MAX_PATH_LEN 512
#define SCORE_PER_MUSIC 11

/* ========== Types ========== */

typedef struct {
	float xleft, xright, ydown, yup, speed;
} Rect;

typedef struct {
	float x, y;
	int active, timer;
	float pulse;
} Orb;

typedef struct {
	char img_snake[MAX_PATH_LEN];
	char img_food[MAX_PATH_LEN];
	char img_enemy_normal[MAX_PATH_LEN];
	char img_enemy_midboss[MAX_PATH_LEN];
	char img_enemy_boss[MAX_PATH_LEN];
	char music[MAX_MUSIC][MAX_PATH_LEN];
	char sfx_eat[MAX_PATH_LEN];
	char sfx_gameover[MAX_PATH_LEN];
	char sfx_orb[MAX_PATH_LEN];
	char sfx_collision[MAX_PATH_LEN];
} Config;

/* ========== Screen ========== */

int screen_width, screen_height;

/* ========== Config ========== */

Config config;

/* ========== Textures ========== */

GLuint tex_snake;
GLuint tex_food;
GLuint tex_enemy_normal;
GLuint tex_enemy_midboss;
GLuint tex_enemy_boss;
int has_tex_midboss = 0;
int has_tex_boss = 0;

/* ========== Sound ========== */

Mix_Music *music_tracks[MAX_MUSIC];
Mix_Chunk *sfx_eat_sound = NULL;
Mix_Chunk *sfx_gameover_sound = NULL;
Mix_Chunk *sfx_orb_sound = NULL;
Mix_Chunk *sfx_collision_sound = NULL;
int current_music_tier = -1;
int sound_initialized = 0;

/* ========== Game state ========== */

int window;
int difficulty = DIFF_NORMAL;
int num_enemies = START_ENEMIES;
int direction = DIR_UP;
int paused = 0;
int game_over = 0;
int food_eaten = 0;
int respawn_food = 1;
int food_cycle = 0;
int food_x = 1000, food_y = 1000;
int points = 0;
int snake_length = 1;
int food_timer = 600;
char game_over_msg[256];

Orb orb = {0, 0, 0, 0, 0.0f};

/* ========== Movement ========== */

float snake_x = 0, snake_y = 0;
float snake_speed = 4;
float slow_enemy_speed = 3;
float fast_enemy_speed = 6;
float enemy_gap;

/* ========== Per-enemy arrays ========== */

float enemy_dist_y[MAX_ENEMIES], enemy_dist_x[MAX_ENEMIES];
float last_dy[MAX_ENEMIES], last_dx[MAX_ENEMIES];
float enemy_size[MAX_ENEMIES];
float enemy_speed[MAX_ENEMIES];
int   enemy_dir_y[MAX_ENEMIES], enemy_dir_x[MAX_ENEMIES];
int   enemy_type[MAX_ENEMIES];

/* ========== Entity rects ========== */

Rect snake[100];
Rect enemy[MAX_ENEMIES];
Rect food_rect;

/* ========== Utility ========== */

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

static char *strip(char *s)
{
	while (*s == ' ' || *s == '\t') s++;
	char *end = s + strlen(s) - 1;
	while (end > s && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r'))
		*end-- = '\0';
	return s;
}

/* ========== Config parsing ========== */

static int load_config(const char *path)
{
	FILE *f = fopen(path, "r");
	if (!f) return -1;

	char line[1024];
	memset(&config, 0, sizeof(Config));

	while (fgets(line, sizeof(line), f)) {
		char *s = strip(line);
		if (*s == '#' || *s == '[' || *s == '\0')
			continue;

		char *eq = strchr(s, '=');
		if (!eq) continue;

		*eq = '\0';
		char *key = strip(s);
		char *val = strip(eq + 1);

		if (strcmp(key, "snake") == 0)
			strncpy(config.img_snake, val, MAX_PATH_LEN - 1);
		else if (strcmp(key, "food") == 0)
			strncpy(config.img_food, val, MAX_PATH_LEN - 1);
		else if (strcmp(key, "enemy_normal") == 0)
			strncpy(config.img_enemy_normal, val, MAX_PATH_LEN - 1);
		else if (strcmp(key, "enemy_midboss") == 0)
			strncpy(config.img_enemy_midboss, val, MAX_PATH_LEN - 1);
		else if (strcmp(key, "enemy_boss") == 0)
			strncpy(config.img_enemy_boss, val, MAX_PATH_LEN - 1);
		else if (strcmp(key, "music_1") == 0)
			strncpy(config.music[0], val, MAX_PATH_LEN - 1);
		else if (strcmp(key, "music_2") == 0)
			strncpy(config.music[1], val, MAX_PATH_LEN - 1);
		else if (strcmp(key, "music_3") == 0)
			strncpy(config.music[2], val, MAX_PATH_LEN - 1);
		else if (strcmp(key, "music_4") == 0)
			strncpy(config.music[3], val, MAX_PATH_LEN - 1);
		else if (strcmp(key, "music_5") == 0)
			strncpy(config.music[4], val, MAX_PATH_LEN - 1);
		else if (strcmp(key, "music_6") == 0)
			strncpy(config.music[5], val, MAX_PATH_LEN - 1);
		else if (strcmp(key, "music_7") == 0)
			strncpy(config.music[6], val, MAX_PATH_LEN - 1);
		else if (strcmp(key, "sfx_eat") == 0)
			strncpy(config.sfx_eat, val, MAX_PATH_LEN - 1);
		else if (strcmp(key, "sfx_gameover") == 0)
			strncpy(config.sfx_gameover, val, MAX_PATH_LEN - 1);
		else if (strcmp(key, "sfx_orb") == 0)
			strncpy(config.sfx_orb, val, MAX_PATH_LEN - 1);
		else if (strcmp(key, "sfx_collision") == 0)
			strncpy(config.sfx_collision, val, MAX_PATH_LEN - 1);
	}
	fclose(f);
	return 0;
}

/* ========== Sound ========== */

static void play_sfx(Mix_Chunk *sfx)
{
	if (sound_initialized && sfx)
		Mix_PlayChannel(-1, sfx, 0);
}

static void update_music(void)
{
	if (!sound_initialized) return;

	int tier = points / SCORE_PER_MUSIC;
	if (tier >= MAX_MUSIC) tier = MAX_MUSIC - 1;

	if (tier != current_music_tier && music_tracks[tier]) {
		Mix_FadeInMusic(music_tracks[tier], -1, 1000);
		current_music_tier = tier;
	}
}

static void init_sound(void)
{
	int i;

	if (SDL_Init(SDL_INIT_AUDIO) < 0) {
		fprintf(stderr, "SDL audio: %s\n", SDL_GetError());
		return;
	}
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
		fprintf(stderr, "SDL_mixer: %s\n", Mix_GetError());
		return;
	}
	sound_initialized = 1;

	for (i = 0; i < MAX_MUSIC; i++) {
		if (config.music[i][0])
			music_tracks[i] = Mix_LoadMUS(config.music[i]);
	}

	if (config.sfx_eat[0])
		sfx_eat_sound = Mix_LoadWAV(config.sfx_eat);
		if (sfx_eat_sound)
			Mix_VolumeChunk(sfx_eat_sound, MIX_MAX_VOLUME / 4);
	if (config.sfx_gameover[0])
		sfx_gameover_sound = Mix_LoadWAV(config.sfx_gameover);
	if (config.sfx_orb[0])
		sfx_orb_sound = Mix_LoadWAV(config.sfx_orb);
	if (config.sfx_collision[0]) {
		sfx_collision_sound = Mix_LoadWAV(config.sfx_collision);
		if (sfx_collision_sound)
			Mix_VolumeChunk(sfx_collision_sound, MIX_MAX_VOLUME / 4);
	}
}

static void cleanup_sound(void)
{
	int i;
	if (!sound_initialized) return;

	Mix_HaltMusic();
	for (i = 0; i < MAX_MUSIC; i++)
		if (music_tracks[i]) Mix_FreeMusic(music_tracks[i]);
	if (sfx_eat_sound) Mix_FreeChunk(sfx_eat_sound);
	if (sfx_gameover_sound) Mix_FreeChunk(sfx_gameover_sound);
	if (sfx_orb_sound) Mix_FreeChunk(sfx_orb_sound);
	if (sfx_collision_sound) Mix_FreeChunk(sfx_collision_sound);

	Mix_CloseAudio();
	SDL_Quit();
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

static int load_and_bind(char *path, GLuint *tex)
{
	if (!path[0]) return 0;
	int img = load_image(path);
	if (img == -1) {
		fprintf(stderr, "Impossible de charger: %s\n", path);
		return -1;
	}
	bind_texture(tex);
	return 1;
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

float rect_distance(Rect a, Rect b)
{
	float dx = a.xright - b.xright;
	float dy = a.yup - b.yup;
	return sqrtf(dx * dx + dy * dy);
}

static float rect_cx(Rect r) { return (r.xleft + r.xright) / 2.0f; }
static float rect_cy(Rect r) { return (r.ydown + r.yup) / 2.0f; }

/* ========== Orb ========== */

static void spawn_orb(void)
{
	float W = screen_width, H = screen_height;
	orb.x = rand_range(-W / 2 + 50, W / 2 - 50);
	orb.y = rand_range(-H / 2 + 50, H / 2 - 50);
	orb.active = 1;
	orb.timer = ORB_ANIM_DURATION;
	orb.pulse = 0.0f;
}

static int point_in_orb(float px, float py)
{
	float dx = px - orb.x, dy = py - orb.y;
	float r2 = (ORB_RADIUS * 2) * (ORB_RADIUS * 2);
	return (dx * dx + dy * dy) < r2;
}

static void push_from_orb(float *ox, float *oy)
{
	float dx = *ox - orb.x, dy = *oy - orb.y;
	float len = sqrtf(dx * dx + dy * dy);
	if (len < 1.0f) len = 1.0f;
	*ox += (dx / len) * ORB_PUSH_FORCE;
	*oy += (dy / len) * ORB_PUSH_FORCE;
}

static void draw_orb(void)
{
	if (!orb.active) return;

	float scale = 1.0f + 0.3f * sinf(orb.pulse);
	float r = ORB_RADIUS * scale;
	float alpha = (float)orb.timer / ORB_ANIM_DURATION;
	int k, seg = 24;

	glColor4f(0.8f, 0.2f, 1.0f, alpha * 0.3f);
	glBegin(GL_TRIANGLE_FAN);
	glVertex2f(orb.x, orb.y);
	for (k = 0; k <= seg; k++) {
		float a = 2.0f * M_PI * k / seg;
		glVertex2f(orb.x + cosf(a) * r * 1.8f, orb.y + sinf(a) * r * 1.8f);
	}
	glEnd();

	glColor3f(0.9f, 0.3f, 1.0f);
	glBegin(GL_TRIANGLE_FAN);
	glVertex2f(orb.x, orb.y);
	for (k = 0; k <= seg; k++) {
		float a = 2.0f * M_PI * k / seg;
		glVertex2f(orb.x + cosf(a) * r, orb.y + sinf(a) * r);
	}
	glEnd();
}

/* ========== Enemy management ========== */

static void set_enemy_type_and_stats(int i)
{
	if (i > 0 && i % 10 == 0) {
		enemy_type[i] = ENEMY_BOSS;
		enemy_size[i] = BOSS_SIZE;
		enemy_speed[i] = fast_enemy_speed * 1.2f;
	} else if (i > 0 && i % 4 == 0) {
		enemy_type[i] = ENEMY_MIDBOSS;
		enemy_size[i] = MIDBOSS_SIZE;
		enemy_speed[i] = (slow_enemy_speed + fast_enemy_speed) / 2.0f;
	} else {
		enemy_type[i] = ENEMY_NORMAL;
		enemy_size[i] = SMALL_SIZE;
		enemy_speed[i] = slow_enemy_speed;
	}
}

static GLuint enemy_texture(int i)
{
	switch (enemy_type[i]) {
	case ENEMY_BOSS:    return has_tex_boss ? tex_enemy_boss : tex_enemy_normal;
	case ENEMY_MIDBOSS: return has_tex_midboss ? tex_enemy_midboss : tex_enemy_normal;
	default:            return tex_enemy_normal;
	}
}

void add_enemy(void)
{
	int i = num_enemies - 1;
	float dist = 0;

	set_enemy_type_and_stats(i);

	while (dist < screen_width / 5) {
		enemy_dist_y[i] = rand_range(1, screen_height);
		enemy_dist_x[i] = rand_range(1, screen_width);

		float sz = enemy_size[i] * screen_height;
		enemy[i].xleft = -screen_width / 2 + enemy_dist_x[i] - sz;
		enemy[i].xright = -screen_width / 2 + enemy_dist_x[i];
		enemy[i].ydown = enemy_dist_y[i] - (screen_height / 2 - sz);
		enemy[i].yup = enemy_dist_y[i] - (screen_height / 2 - 2 * sz);
		dist = rect_distance(enemy[i], snake[0]);
	}

	enemy_dir_y[i] = (rand_range(1, 2) == 1) ? 1 : -1;
	enemy_dir_x[i] = (rand_range(1, 2) == 1) ? 1 : -1;
}

static void enemy_eat_food(int i)
{
	if (enemy_size[i] < MAX_ENEMY_SIZE)
		enemy_size[i] *= 1.15f;
	/* Enemy gets faster */
  if(difficulty==DIFF_HARD)
    enemy_speed[i] *= 1.1f;
	/* Force food respawn */
  food_cycle = food_timer - 1;
	respawn_food = 1;
}

/* ========== Game control ========== */

void reset(void)
{
	int i;
	snake_x = 0;
	snake_y = 0;
	snake_speed = 4;
	num_enemies = START_ENEMIES;
	enemy_gap = screen_width / 5;
	game_over = 0;
	points = 0;
	snake_length = 1;
	food_cycle = 0;
	respawn_food = 1;
	food_eaten = 0;
	orb.active = 0;
	game_over_msg[0] = '\0';

	for (i = 0; i < num_enemies; i++) {
		enemy_dist_y[i] = i * enemy_gap;
		enemy_dist_x[i] = i * enemy_gap;
		enemy_dir_y[i] = 1;
		enemy_dir_x[i] = 1;
		enemy_type[i] = ENEMY_NORMAL;
		enemy_speed[i] = slow_enemy_speed;
		enemy_size[i] = SMALL_SIZE;
	}
}


/* ========== Difficulty setup ========== */

static void apply_difficulty(int level)
{
	difficulty = level;
	switch (level) {
	case DIFF_EASY:
		slow_enemy_speed = 1.5;
		fast_enemy_speed = 3;
		food_timer = 600;
		break;
	case DIFF_HARD:
		slow_enemy_speed = 2;
		fast_enemy_speed = 4;
		food_timer = 1800;
		break;
	default:
		difficulty = DIFF_NORMAL;
		slow_enemy_speed = 1.5;
		fast_enemy_speed = 3;
		food_timer = 1800;
		break;
	}
  snake_speed = 4;
}
static void start_game(void)
{
	reset();
	apply_difficulty(difficulty);
	current_music_tier = -1;
	update_music();
}

/* ========== Drawing helpers ========== */

static void draw_quad(float x1, float y1, float x2, float y2)
{
	glVertex2f(x1, y2);
	glVertex2f(x2, y2);
	glVertex2f(x2, y1);
	glVertex2f(x1, y1);
}

static void draw_textured_quad(GLuint tex, float x1, float y1, float x2, float y2)
{
	glEnable(GL_TEXTURE_2D);
	glMatrixMode(GL_MODELVIEW);
	glBindTexture(GL_TEXTURE_2D, tex);
	glBegin(GL_QUADS);
	glTexCoord2i(0, 0); glVertex2f(x1, y2);
	glTexCoord2i(1, 0); glVertex2f(x2, y2);
	glTexCoord2i(1, 1); glVertex2f(x2, y1);
	glTexCoord2i(0, 1); glVertex2f(x1, y1);
	glEnd();
	glDisable(GL_TEXTURE_2D);
}

static void build_game_over_msg(void)
{
	sprintf(game_over_msg, "%d", points);
	if (points < 5)
		strcat(game_over_msg, " points... C'est lamentable, affligeant, pitoyable...");
	else if (points < 10)
		strcat(game_over_msg, " points... Essaie la bataille ou les petits chevaux.");
	else if (points < 15)
		strcat(game_over_msg, " points... Bof bof !");
	else if (points < 20)
		strcat(game_over_msg, " points. L'Histoire ne retiendra pas cette partie.");
	else if (points < 25)
		strcat(game_over_msg, " points. Pas mal, c'est correct.");
	else if (points < 30)
		strcat(game_over_msg, " points ! Tu as un certain talent !");
	else
		strcat(game_over_msg, " points ! Du jamais vu ! Tu deviendras un grand de ce monde...");
}

/* ========== Display ========== */

void display(void)
{
	int i;
	float W = screen_width;
	float H = screen_height;
	float cell = SMALL_SIZE * H;

	if (paused && !game_over)
		return;

	glClearColor(0.5f, 0.0f, 0.4f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/* Background bands */
	glBegin(GL_QUADS);
	glColor3f(0.75f, 0.75f, 0.15f);
	draw_quad(-W / 2, -H / 2, W / 2, -(H / 2 - cell));
	glEnd();

	glBegin(GL_QUADS);
	glColor3f(0.50f, 0.70f, 0.90f);
	draw_quad(-W / 2, -(H / 2 - cell), W / 2, -(H / 2 - 2 * cell));
	glEnd();

	glBegin(GL_QUADS);
	glColor3f(0.35f, 0.15f, 0.75f);
	draw_quad(-W / 2, -(H / 2 - 2 * cell), W / 2, -(H / 2 - 3 * cell));
	glEnd();

	/* Clamp snake */
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

	draw_textured_quad(tex_snake, sx1, sy1, sx2, sy2);

	/* Food */
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

	food_rect.xleft = fl;
	food_rect.xright = fr;
	food_rect.ydown = fd;
	food_rect.yup = fu;

	if (!game_over && collision(snake[0], food_rect))
		food_eaten = 1;

	draw_textured_quad(tex_food, fl, fd, fr, fu);

	/* Orb */
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	draw_orb();
	glDisable(GL_BLEND);

	/* Score HUD */
	glColor3f(1.0f, 1.0f, 1.0f);
	char score_buf[64];
	sprintf(score_buf, "Score: %d", points);
	bitmap_text(-W / 2 + 10, H / 2 - 25, score_buf, GLUT_BITMAP_HELVETICA_18);

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

		draw_textured_quad(enemy_texture(i), ex1, ey1, ex2, ey2);

		/* Enemy-enemy collision */
		int j;
		for (j = 0; j < i; j++) {
			if (!collision(enemy[j], enemy[i]))
				continue;

			float dx = rect_cx(enemy[i]) - rect_cx(enemy[j]);
			float dy = rect_cy(enemy[i]) - rect_cy(enemy[j]);

			if (fabsf(dx) > fabsf(dy)) {
				enemy_dir_x[i] = (dx > 0) ? 1 : -1;
				enemy_dir_x[j] = (dx > 0) ? -1 : 1;
			} else {
				enemy_dir_y[i] = (dy > 0) ? 1 : -1;
				enemy_dir_y[j] = (dy > 0) ? -1 : 1;
			}


			if (!orb.active)
				spawn_orb();
		}

		/* Hard mode: enemy eats food */
		if (difficulty != DIFF_EASY && collision(enemy[i], food_rect))
			enemy_eat_food(i);

		/* Enemy kills snake */
		if (!game_over && collision(enemy[i], snake[0])) {
			build_game_over_msg();
			game_over = 1;
			play_sfx(sfx_collision_sound);
			if (sound_initialized) Mix_HaltMusic();
			play_sfx(sfx_gameover_sound);
		}
	}

	/* Game over overlay */
	if (game_over) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(0.0f, 0.0f, 0.0f, 0.6f);
		glBegin(GL_QUADS);
		draw_quad(-W / 2, -H / 2, W / 2, H / 2);
		glEnd();
		glDisable(GL_BLEND);

		glColor3f(0.95f, 0.1f, 0.1f);
		bitmap_text(-300, 40, game_over_msg, GLUT_BITMAP_TIMES_ROMAN_24);

		glColor3f(1.0f, 1.0f, 1.0f);
		bitmap_text(-180, -20, "R = Rejouer    Q = Quitter", GLUT_BITMAP_HELVETICA_18);
	}

	glutSwapBuffers();
}

/* ========== Idle ========== */

void idle(void)
{
	int i;

	if (paused) return;

	if (game_over) {
		glutPostRedisplay();
		return;
	}

	if (food_eaten) {
		num_enemies++;
		food_eaten = 0;
		points++;
		food_cycle = food_timer - 1;
		add_enemy();
		play_sfx(sfx_eat_sound);
		update_music();
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

	/* Orb interactions */
	if (orb.active) {
		orb.pulse += 0.15f;
		orb.timer--;

		float scx = rect_cx(snake[0]), scy = rect_cy(snake[0]);
		if (point_in_orb(scx, scy)) {
			push_from_orb(&snake_x, &snake_y);
			play_sfx(sfx_orb_sound);
			orb.active = 0;
		}

		for (i = 0; i < num_enemies; i++) {
			float ecx = rect_cx(enemy[i]), ecy = rect_cy(enemy[i]);
			if (point_in_orb(ecx, ecy)) {
				push_from_orb(&enemy_dist_x[i], &enemy_dist_y[i]);
				//play_sfx(sfx_orb_sound);
				orb.active = 0;
				break;
			}
		}

		if (orb.timer <= 0)
			orb.active = 0;
	}

	glutPostRedisplay();
}

/* ========== Reshape ========== */

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

/* ========== Keyboard ========== */

void keypress(unsigned char key, int x, int y)
{
	(void)x; (void)y;

	if (game_over) {
		if (key == 'r' || key == 'R') start_game();
		if (key == 'q' || key == 'Q') { cleanup_sound(); glFinish(); glutDestroyWindow(window); }
		return;
	}

	switch (key) {
	case 'q': case 'Q':
		cleanup_sound();
		glFinish();
		glutDestroyWindow(window);
		break;
	case 'r': case 'R':
		start_game();
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
	if (game_over) return;

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
			set_enemy_type_and_stats(i);
		break;
	case GLUT_KEY_F2:
		slow_enemy_speed /= 1.3f;
		fast_enemy_speed /= 1.3f;
		for (i = 0; i < num_enemies; i++)
			set_enemy_type_and_stats(i);
		break;
	}
}


/* ========== Usage ========== */

static void print_usage(const char *prog)
{
	printf("Usage: %s [-d 1|2|3] [-c config.cfg]\n\n", prog);
	printf("  -d 1  Facile    (ennemis lents, ne mangent pas la nourriture)\n");
	printf("  -d 2  Normal    (defaut)\n");
	printf("  -d 3  Difficile (ennemis plus rapides)\n");
	printf("  -c    Fichier de configuration (defaut: snake_bizarre.cfg)\n");
}

/* ========== Main ========== */

int main(int argc, char *argv[])
{
	int diff_level = DIFF_NORMAL;
	const char *config_path = "snake_bizarre.cfg";

	srand(time(NULL));
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);

	/* Parse CLI flags */
	int argi = 1;
	while (argi < argc) {
		if (strcmp(argv[argi], "-d") == 0 && argi + 1 < argc) {
			diff_level = atoi(argv[++argi]);
			if (diff_level < 1 || diff_level > 3) diff_level = DIFF_NORMAL;
		} else if (strcmp(argv[argi], "-c") == 0 && argi + 1 < argc) {
			config_path = argv[++argi];
		} else if (strcmp(argv[argi], "-h") == 0 || strcmp(argv[argi], "--help") == 0) {
			print_usage(argv[0]);
			return 0;
		}
		argi++;
	}

	/* Load config */
	if (load_config(config_path) != 0) {
		fprintf(stderr, "Impossible de lire le fichier de config: %s\n", config_path);
		print_usage(argv[0]);
		return -1;
	}

	/* Verify required images */
	if (!config.img_snake[0] || !config.img_food[0] || !config.img_enemy_normal[0]) {
		fprintf(stderr, "Le fichier de config doit contenir au minimum: snake, food, enemy_normal\n");
		return -1;
	}

	/* Screen */
	screen_width = glutGet(GLUT_SCREEN_WIDTH);
	screen_height = glutGet(GLUT_SCREEN_HEIGHT);
	if (screen_width == 0) screen_width = 1920;
	if (screen_height == 0) screen_height = 1080;

	reset();
	apply_difficulty(diff_level);

	window = glutCreateWindow("Snake Bizarre");
	glutFullScreen();
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);

	/* DevIL */
	if (ilGetInteger(IL_VERSION_NUM) < IL_VERSION) {
		fprintf(stderr, "Wrong DevIL version\n");
		return -1;
	}
	ilInit();

	/* Load textures */
	if (load_and_bind(config.img_snake, &tex_snake) < 0) return -1;
	if (load_and_bind(config.img_food, &tex_food) < 0) return -1;
	if (load_and_bind(config.img_enemy_normal, &tex_enemy_normal) < 0) return -1;

	if (config.img_enemy_midboss[0])
		has_tex_midboss = (load_and_bind(config.img_enemy_midboss, &tex_enemy_midboss) > 0);
	if (config.img_enemy_boss[0])
		has_tex_boss = (load_and_bind(config.img_enemy_boss, &tex_enemy_boss) > 0);

	/* Sound */
	init_sound();
	current_music_tier = -1;
	update_music();

	atexit(cleanup_sound);

	glutIdleFunc(idle);
	glutKeyboardFunc(keypress);
	glutSpecialFunc(special_keys);
	glutMainLoop();

	return 0;
}
