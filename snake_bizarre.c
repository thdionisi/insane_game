#include <stdio.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <GL/glut.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <IL/il.h>
#define WIDTH 1840 // Taille de la fen�tre
#define HEIGHT 1080	
#define nVoitureMax 100 // Nombre de voitures dans une file
#define nVoitureStart 1
#define Pas 0.1 // Pourcentage de d�placement de la grenouille[0] par rapport � la taille de la fen�tre
#define fPasV (float)Pas*HEIGHT	// Pas vertical de d�placement de la grenouille[0]
#define fPasH (float)Pas*WIDTH	// Pas horizontal de d�placement de la grenouille[0]

#define up 0
#define down 1 
#define left 2 
#define right 3
// Variables du programme

GLuint monstre[nVoitureMax];
GLuint bouffe[nVoitureMax];
GLuint heros;

int nVoiture = nVoitureStart;
float sizecar=(float) 0.05; // Taille d'une ligne en pourcentage de la taille totale de la fen�tre
float bigsizecar=0.1;
// se d�termine par : 1/(4 (2 trottoirs et un pont de deux lignes) + nbr rivi�res + nbr routes)
float fDistanceUp[nVoitureMax];
float fDistanceRight[nVoitureMax];
float lastdistup[nVoitureMax];
float lastdistright[nVoitureMax];
float fLigne[nVoitureMax];
float fEcart;
float nVitesse[nVoitureMax];	// Vitesse dans la boucle Idle
float nVitesseGrenouille=5;	// Vitesse dans la boucle Idle
// D�placement de la grenouille[0]
float fHoriz=0;
float fVertic=0;
int dir=0;
int window;
int car_up[nVoitureMax];
int car_right[nVoitureMax];
int drawTrucmucheAuMilieu = 0;int cpt;
int changeTrucABouffer=1,cpt2=0,anctrucx=1000,anctrucy=1000;
int points=0;
// D�claration des fonctions
void vDisplay();
void vReshape();
void vIdle();
void vSpecial(int key, int x, int y);
void keypress(unsigned char key,int x,int y);
void vBitmapOutput(int x, int y, char *string, void *font);
void vStrokeOutput(GLfloat x, GLfloat y, char *string, void *font);
int leave = 0;
int miam_miam = 0;
int pos_in_queue,grenouille_size=1;
int timeTrucABouffer=600;
float fastSpeed = 6, slowSpeed = 3;
int paused=0;
typedef struct obj
{
	float xleft;
	float xright;
	float ydown;
	float yup;
	float vitesse;
} quad;

quad grenouille[100];
quad voiture[nVoitureMax];
double myrand(double a,double b) {

	return ( rand()/(double)RAND_MAX ) *( b-a)+a;

}
void vBitmapOutput(int x, int y, char *string, void *font)
{
	int len,i; // len donne la longueur de la cha�ne de caract�res

	glRasterPos2f(x,y); // Positionne le premier caract�re de la cha�ne
	len = (int) strlen(string); // Calcule la longueur de la cha�ne
	for (i = 0; i < len; i++) glutBitmapCharacter(font,string[i]); // Affiche chaque caract�re de la cha�ne
}

void vStrokeOutput(GLfloat x, GLfloat y, char *string, void *font)
{
	char *p;

	glPushMatrix();	// glPushMatrix et glPopMatrix sont utilis�es pour sauvegarder 
	// et restaurer les syst�mes de coordonn�es non translat�s
	glTranslatef(x, y, 0); // Positionne le premier caract�re de la cha�ne
	for (p = string; *p; p++) glutStrokeCharacter(font, *p); // Affiche chaque caract�re de la cha�ne
	glPopMatrix();
}


// MAIN

void reset()
{
	fHoriz=0;
	fVertic=0;
	nVitesseGrenouille=7;
	nVoiture = nVoitureStart;
	fEcart=WIDTH/5;
	int i;
	for (i=0;i<nVoiture;i++) 
	{
		fDistanceUp[i]=i*fEcart; // R�initialis� lorsque la voiture sort de l'�cran
		fDistanceRight[i]=i*fEcart; // R�initialis� lorsque la voiture sort de l'�cran
		car_up[i]=1;car_right[i]=1;
		nVitesse[i]=3;
		fLigne[i]=sizecar;
	}
}
void gentextures(GLuint *img)
{
	glGenTextures(1, img); /* Texture name generation */
	glBindTexture(GL_TEXTURE_2D, *img); /* Binding of texture name */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); /* We will use linear interpolation for magnification filter */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); /* We will use linear interpolation for minifying filter */
	glTexImage2D(GL_TEXTURE_2D, 0, ilGetInteger(IL_IMAGE_BPP), ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT), 
			0, ilGetInteger(IL_IMAGE_FORMAT), GL_UNSIGNED_BYTE, ilGetData());
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
}	

int collision(struct obj obj1, struct obj obj2)//float x1left,float x1right,float y1up,float y1down,float x2left,float x2right,float y2up,float y2down)
{
	if(obj1.xright < obj2.xleft) return 0;
	if(obj2.xright < obj1.xleft) return 0;
	if(obj1.yup < obj2.ydown) return 0;
	if(obj2.yup < obj1.ydown) return 0;
	return 1;
}
void keypress(unsigned char key,int x, int y)
{
	(void)x; (void)y;
	switch(key) {
		case 'q' :  glFinish();
					glutDestroyWindow(window);
					return; 
		case 'r' : reset();
				   break;

		case 'v' : nVitesseGrenouille*=1.3;
				   break;

		case 'b' : nVitesseGrenouille/=1.3;
				   break;
		case 'p' : if(paused==0) paused = 1;else if(paused==1) paused=0;
				   break;
	}
}
void vDisplay()
{

	//Variables locales	
	int i;

	if(paused == 1)return;

	glClearColor(0.5,0,4,0); // selectionne la couleur noire (qui est celle par d�faut)
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // efface le frame buffer


	// Trottoir de d�part
	glBegin(GL_QUADS);	//commence le dessin d'un quadrilat�re
	glColor3d(0.75,0.75,0.15); // trottoir de couleur grise
	glVertex2d(-WIDTH/2,-HEIGHT/2);	//coordonn�es du premier sommet
	glVertex2d(WIDTH/2,-HEIGHT/2); 
	glVertex2d(WIDTH/2,-(HEIGHT/2-sizecar*HEIGHT)); 
	glVertex2d(-WIDTH/2,-(HEIGHT/2-sizecar*HEIGHT));	// Ordre de sommets : sens trigonom�trique
	glEnd();

	// 1�re file de la route
	glBegin(GL_QUADS);	//commence le dessin d'un quadrilat�re
	glColor3d(0.50,0.70,0.90); // route de couleur grise
	glVertex2d(WIDTH/2,-(HEIGHT/2-sizecar*HEIGHT)); 
	glVertex2d(-WIDTH/2,-(HEIGHT/2-sizecar*HEIGHT));	
	glVertex2d(-WIDTH/2,-(HEIGHT/2-2*sizecar*HEIGHT));	
	glVertex2d(WIDTH/2,-(HEIGHT/2-2*sizecar*HEIGHT)); 
	glEnd();



	// Trottoir du milieu
	glBegin(GL_QUADS);	//commence le dessin d'un quadrilat�re
	glColor3d(0.35,0.15,0.75); // trottoir de couleur grise
	glVertex2d(WIDTH/2,-(HEIGHT/2-2*sizecar*HEIGHT)); 
	glVertex2d(-WIDTH/2,-(HEIGHT/2-2*sizecar*HEIGHT));	
	glVertex2d(-WIDTH/2,-(HEIGHT/2-3*sizecar*HEIGHT));	
	glVertex2d(WIDTH/2,-(HEIGHT/2-3*sizecar*HEIGHT));
	glEnd();


	// "Grenouille"
	// grenouille[0] de couleur verte

	if(fHoriz<=-(WIDTH-sizecar*HEIGHT)/2) fHoriz=-(WIDTH-sizecar*HEIGHT)/2;	//Grenouille bloqu�e � gauche	
	if(fHoriz>=(WIDTH-sizecar*HEIGHT)/2) fHoriz=(WIDTH-sizecar*HEIGHT)/2;	//Grenouille bloqu�e � droite	
	if(fVertic<=0) fVertic=0;	//Grenouille bloqu�e en bas	
	if(fVertic>=HEIGHT - sizecar*HEIGHT) fVertic=HEIGHT-sizecar*HEIGHT;	//Grenouille bloqu�e en haut	

	float xleftG=-sizecar*HEIGHT/2+fHoriz;
	float xrightG=sizecar*HEIGHT/2+fHoriz;
	float ydownG=-HEIGHT/2+fVertic;
	float yupG=-HEIGHT/2+sizecar*HEIGHT +fVertic;

	grenouille[0].xleft=xleftG;grenouille[0].xright=xrightG;grenouille[0].ydown=ydownG;grenouille[0].yup=yupG;


	glEnable(GL_TEXTURE_2D);
	glMatrixMode(GL_MODELVIEW);
	glBindTexture(GL_TEXTURE_2D,heros);
	//glColor3d(1.,0.2,0.2); 


	glBegin(GL_QUADS);	

	//glColor3d(0,0.46,0.25); 
	glClearColor(0.0,0,0,0); // selectionne la couleur noire (qui est celle par d�faut)
	glTexCoord2i(0, 0); 
	glVertex2d(xleftG,yupG);
	glTexCoord2i(1, 0); 
	glVertex2d(xrightG,yupG);
	glTexCoord2i(1, 1); 
	glVertex2d(xrightG,ydownG);
	glTexCoord2i(0, 1);
	glVertex2d(xleftG,ydownG);
	glEnd();
	glDisable(GL_TEXTURE_2D);

	grenouille[0].xleft=xleftG;grenouille[0].xright=xrightG;grenouille[0].ydown=ydownG;grenouille[0].yup=yupG;

	int truc_x,truc_y;	
	if(changeTrucABouffer==1) {

		truc_x = myrand(sizecar*HEIGHT,WIDTH-sizecar*HEIGHT);anctrucx=truc_x;
		truc_y = myrand(1,HEIGHT-sizecar*HEIGHT);anctrucy=truc_y;
	}
	else
	{
		truc_x=anctrucx;truc_y=anctrucy;
	}
	//truc_x=250;truc_y=200;
	float xleft=-WIDTH/2+truc_x-sizecar*HEIGHT;
	float xright=-WIDTH/2+truc_x;
	float yup=truc_y-HEIGHT/2+sizecar*HEIGHT;
	float ydown=truc_y-HEIGHT/2;
	if(yup>=HEIGHT/2) yup=HEIGHT/2;
	quad TrucABouffer;
	TrucABouffer.xleft=xleft;TrucABouffer.xright=xright;TrucABouffer.ydown=ydown;TrucABouffer.yup=yup;
	glEnable(GL_TEXTURE_2D);
	glMatrixMode(GL_MODELVIEW);
	glBindTexture(GL_TEXTURE_2D,bouffe[0]);
	glBegin(GL_QUADS);
	if (collision(grenouille[0],TrucABouffer))
	{
		//glColor3d(0.55,0.4,0.16);
		grenouille_size++;
	}

	//printf("x %f y %f %f %f\n",xleft,xright,yup,ydown);
	glClearColor(0.0,0,0,0); // selectionne la couleur noire (qui est celle par d�faut)
	glTexCoord2i(0, 0); 
	glVertex2d(xleft,yup);
	glTexCoord2i(1, 0); 
	glVertex2d(xright,yup);
	glTexCoord2i(1, 1); 
	glVertex2d(xright,ydown);
	glTexCoord2i(0, 1);
	glVertex2d(xleft,ydown);
	glEnd();
	glDisable(GL_TEXTURE_2D);
	// Boucle de cr�ation des voitures
	for (i=0;i<nVoiture;i++)
	{	
		// Voiture i

		//if(fDistanceUp[i]>=WIDTH+fEcart) {fDistanceUp[i]=0;fDistanceRight[i]=0;car_right[i]=1;car_up[i]=1;}
		//if(fDistanceRight[i]>=WIDTH+fEcart) {fDistanceRight[i]=0;fDistanceUp[i]=0;car_right[i]=1;car_up[i]=1;}
		if(fDistanceUp[i] >= HEIGHT-2*fLigne[i]*HEIGHT) fDistanceUp[i] = HEIGHT-2*fLigne[i]*HEIGHT;
		if(fDistanceUp[i] <= -fLigne[i]*HEIGHT) fDistanceUp[i] = -fLigne[i]*HEIGHT;
		if(fDistanceRight[i] >= WIDTH) fDistanceRight[i] = WIDTH;
		if(fDistanceRight[i] <= fLigne[i]*HEIGHT) fDistanceRight[i] = fLigne[i]*HEIGHT;

		float xleftV,xrightV,ydownV,yupV;
		xleftV = -WIDTH/2+fDistanceRight[i]-fLigne[i]*HEIGHT;
		xrightV = -WIDTH/2+fDistanceRight[i];
		ydownV = fDistanceUp[i]-(HEIGHT/2-fLigne[i]*HEIGHT);
		yupV = fDistanceUp[i]-(HEIGHT/2-2*fLigne[i]*HEIGHT);

		if(xleftV <= -WIDTH/2) car_right[i]=1;
		if(xrightV >= WIDTH/2) car_right[i]=-1;
		if(ydownV <= - HEIGHT/2) car_up[i] = 1;
		if(yupV >= HEIGHT/2) car_up[i] = -1;
		voiture[i].xleft=xleftV;voiture[i].xright=xrightV;voiture[i].ydown=ydownV;voiture[i].yup=yupV;
		glEnable(GL_TEXTURE_2D);

		// Operate on model-view matrix
		glMatrixMode(GL_MODELVIEW);
		glColor3d(1.,0.2,0.2); // voiture de couleur bleue
		glBindTexture(GL_TEXTURE_2D,monstre[0]);
		glBegin(GL_QUADS);	//commence le dessin d'un quadrilat�re

		glTexCoord2i(0, 0); 
		glVertex2d(xleftV,yupV);
		glTexCoord2i(1, 0); 
		glVertex2d(xrightV,yupV);
		glTexCoord2i(1, 1); 
		glVertex2d(xrightV,ydownV);
		glTexCoord2i(0, 1);
		glVertex2d(xleftV,ydownV);
		glEnd();
		glDisable(GL_TEXTURE_2D);
		int j;
		for(j=0;j<i;j++) {
			if (collision(voiture[j],voiture[i]))//xleftG,xrightG,yupG,ydownG,xleftV,xrightV,yupV,ydownV))
			{
				if(xleftG >= xleftV && dir != right) {
					car_right[i] = -1;
					car_right[j] = 1;
					drawTrucmucheAuMilieu = 1;
				}
				if(ydownG >=ydownV && dir !=up) {
					car_up[i] = -1;
					car_up[j] = 1;
					drawTrucmucheAuMilieu = 1;
				}
				if(yupG <= yupV && dir != down) {
					car_up[i] = 1;
					car_up[j] = -1;
					drawTrucmucheAuMilieu = 1;
				}
				if(xrightG <=xrightV && dir != left) {
					car_right[i] = 1;
					car_right[j] = -1;
					drawTrucmucheAuMilieu = 1;
				}
			}
		}	

		if(drawTrucmucheAuMilieu == 1) {
			glBegin(GL_QUADS);
			glColor3d(0.65,0.1,0.76);
			glVertex2d(-20,-20);
			glVertex2d(20,20);
			glVertex2d(-20,20);
			glVertex2d(20,-20);
		}
		glEnd();
		glColor3d(0.95,0.,0.); // Texte en rouge 

		if(collision(grenouille[0], TrucABouffer ))
		{
			miam_miam = 1;
		}

		if(collision(voiture[i],grenouille[0] ))
		{
			char buffer[100]={'\0'};
			sprintf(buffer, "%d", points);

			if(points <5) strcat(buffer," points... C'est lamentable,affligeant,pitoyable...");
			else if(points <10) strcat(buffer," points... Tu devrais essayer la bataille ou les petits chevaux.");
			else if(points <15) strcat(buffer," points... Essaye encore !");
			else if(points <20) strcat(buffer," points. L'Histoire ne retiendra malheureusement pas cette partie.");
			else if(points <25) strcat(buffer," points. Pas mal, pas mal, mais il en faut plus pour m'impressionner !");
			else if(points <30) strcat(buffer," points ! Tu as un certain talent !");
			else strcat(buffer," points ! Quel talent inoui ! Tu deviendras un grand de ce monde...");
			//else if(points <10) vBitmapOutput(-150,0,"Tu devrais essayer la bataille ou les petits chevaux.",GLUT_BITMAP_TIMES_ROMAN_24);
			//else if(points <15) vBitmapOutput(-150,0,"L'Histoire ne retiendra pas cette partie...",GLUT_BITMAP_TIMES_ROMAN_24);
			//else if(points <20) vBitmapOutput(-150,0,"Pas mal, pas mal !",GLUT_BITMAP_TIMES_ROMAN_24);
			//else vBitmapOutput(-150,0,"Quel talent ! Tu deviendras un grand de ce monde...",GLUT_BITMAP_TIMES_ROMAN_24);
			vBitmapOutput(-250,20,buffer,GLUT_BITMAP_TIMES_ROMAN_24);
			leave=1;
		}	

	}

	glutSwapBuffers();	//Permutation des 2 buffers
}


float distance(struct obj obj1, struct obj obj2)
{
	float result = sqrt((obj1.xright-obj2.xright)*(obj1.xright-obj2.xright)+(obj1.yup-obj2.yup)*(obj1.yup-obj2.yup));
	//fprintf(stderr,"Dist %f width %d\n",result,WIDTH/5);
	return result;
}
void setSizeAndSpeed(int i)
{
	if(i%8 == 0 && i!=0) 
	{
		fLigne[i] = bigsizecar;
		nVitesse[i] = fastSpeed;
	}
	else
	{
		fLigne[i] = sizecar;
		nVitesse[i] = slowSpeed;
	}
}
void addvoiture()
{
	//fHoriz=0;
	//fVertic=0;
	//nVitesse=3;
	//fEcart=WIDTH/(nVoiture-1);
	int i=nVoiture-1;
	float dist=0;
	setSizeAndSpeed(i);

	while(dist<WIDTH/5) {
		fDistanceUp[i]=myrand(1,HEIGHT); // R�initialis� lorsque la voiture sort de l'�cran
		fDistanceRight[i]=myrand(1,WIDTH); // R�initialis� lorsque la voiture sort de l'�cran
		float xleftV = -WIDTH/2+fDistanceRight[i]-fLigne[i]*HEIGHT;
		float xrightV = -WIDTH/2+fDistanceRight[i];
		float ydownV = fDistanceUp[i]-(HEIGHT/2-fLigne[i]*HEIGHT);
		float yupV = fDistanceUp[i]-(HEIGHT/2-2*fLigne[i]*HEIGHT);
		voiture[i].xleft=xleftV;voiture[i].xright=xrightV;voiture[i].ydown=ydownV;voiture[i].yup=yupV;
		dist=distance(voiture[i],grenouille[0]);
	}
	if(myrand(1,2)==1)car_up[i]=1;else car_up[i]=-1;
	if(myrand(1,2)==1)car_right[i]=1;else car_right[i]=-1;
	//fprintf(stderr,"UP %f right %f dist %f\n",fDistanceUp[i],fDistanceRight[i],distance(voiture[i],grenouille[0]));
}

void vIdle()

{
	if(paused == 1) return;
	int i; // Compteur
	if(miam_miam == 1) {nVoiture +=1; miam_miam = 0; points ++; cpt2 = timeTrucABouffer -1; addvoiture();}
	for (i=0;i<nVoiture;i++)
	{
		if(cpt2 % 10 == 0) {
			int tmpup=fDistanceUp[i];
			int tmpright=fDistanceRight[i];
			fDistanceUp[i]+=car_up[i]*myrand(nVitesse[i]/3,nVitesse[i]); // D�pend du processeur !!! Mieux vaut utiliser un timer quand 
			fDistanceRight[i]+=car_right[i]*myrand(nVitesse[i]/3,nVitesse[i]); // D�pend du processeur !!! Mieux vaut utiliser un timer quand 
			lastdistup[i] = fDistanceUp[i] - tmpup;
			lastdistright[i] = fDistanceRight[i] - tmpright;
		}
		else
		{
			fDistanceUp[i] += lastdistup[i];
			fDistanceRight[i] += lastdistright[i];
		}
		// il y a beaucoup de choses � faire dans l'Idle

	}
	if(dir==up)fVertic+=nVitesseGrenouille;
	if(dir==down)fVertic-=nVitesseGrenouille;
	if(dir==left)fHoriz-=nVitesseGrenouille;
	if(dir==right)fHoriz+=nVitesseGrenouille;

	cpt2++;if(cpt2==timeTrucABouffer){cpt2=0;changeTrucABouffer=1;} else changeTrucABouffer=0;
	if(drawTrucmucheAuMilieu == 1)cpt++;
	if(cpt==100){cpt=0;drawTrucmucheAuMilieu = 0;}
	glutPostRedisplay(); // force le r�affichage de la sc�ne
	if(leave == 1)
	{
		sleep(2);
		glFinish();
		glutDestroyWindow(window);
	}
}



void vReshape(int w, int h) 
{ 
	float L; 
	float H; 

	glViewport(0,0,w,h); 
	glMatrixMode(GL_PROJECTION); // Choisit la matrice de projection 
	glLoadIdentity();
	//glOrtho(0.0, w, h, 0.0, 0.0, 100.0);
	//if (w<=h) 
	//{ 
		//if (w==0) H=HEIGHT; 
		//else H=(GLfloat) (WIDTH*h/w); 
		//L=WIDTH; 
	//} 
	//else 
	//{ 
		//H=HEIGHT; 
		//if (h==0) L=WIDTH; 
		//else L=(GLfloat) (HEIGHT*w/h); 
	//} 
	L=WIDTH;H=HEIGHT;
	gluOrtho2D(-L/2,L/2,-H/2,H/2); 
	glMatrixMode(GL_MODELVIEW);
	glutPostRedisplay();
} 

// Gestion du clavier

void vSpecial(int key, int x, int y)
{
	(void)x; (void)y;
	int i;
	switch (key)
	{

		case GLUT_KEY_UP :
			dir=up;
			break;

		case GLUT_KEY_DOWN :
			dir=down;
			break;

		case GLUT_KEY_LEFT :
			dir=left;
			break;

		case GLUT_KEY_RIGHT :
			dir=right;
			break;

		case GLUT_KEY_F2 : 
			slowSpeed/=1.3;fastSpeed/=1.3;for(i=0;i<nVoiture;i++)setSizeAndSpeed(i);
			break;
		case GLUT_KEY_F1 :
			slowSpeed*=1.3;fastSpeed*=1.3;for(i=0;i<nVoiture;i++)setSizeAndSpeed(i);
			break;

		default :
			printf(" Autre Touche Speciale\n ");
			break;
	}
}

int LoadImage(char *filename)
{
	ILuint    image; 
	ILboolean success; 

	ilGenImages(1, &image);    /* Generation of one image name */
	ilBindImage(image);        /* Binding of image name */


	/* Loading of the image filename by DevIL */
	if ( (success = ilLoadImage(filename)) )
	{
		/* Convert every colour component into unsigned byte */
		/* You can replace IL_RGB with IL_RGBA if your image contains alpha channel */

		success = ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE); 

		if (!success)
		{
			return -1;
		}
	}
	else
		return -1;

	return image;
}

int main( int argc, char *argv[ ])
{
	srand(time(NULL));
	reset();
	int image,image2,image3;

	glutInit(&argc,argv);	// initialisation de GLUT : argc et argv sont respectivement 
	// le nombre et la liste des param�tres pass�es en ligne de commande

	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);	// GLUT_RGBA mode "vraies couleurs" 32 bits
	// GLUT_DOUBLE "douflle buffering" - deux tampons

	glutInitWindowSize(WIDTH,HEIGHT);	// Initialisation de la largeur et de la hauteur de la fen�tre
	glutInitWindowPosition(50,50);	// Position de la fen�tre sur l'�cran par rapport au coin haut gauche
	window = glutCreateWindow("snakouuuuu");	// Titre de la fen�tre
	glutDisplayFunc(vDisplay); // pr�cise la fonction � utiliser pour l'affichage 
	glutReshapeFunc(vReshape); // pr�cise la fonction � utiliser pour le redimensionnement 

	if (ilGetInteger(IL_VERSION_NUM) < IL_VERSION)
	{
		printf("wrong DevIL version \n");
		return -1;
	}
	ilInit(); 


	/* load the file picture with DevIL */

	image2 = LoadImage(argv[1]);
	gentextures(&heros);
	image = LoadImage(argv[2]);
	gentextures(&monstre[0]);
	image3 = LoadImage(argv[3]);
	gentextures(&bouffe[0]);


	/* OpenGL texture binding of the image loaded by DevIL  */
	if ( image == -1 || image2 == -1 || image3 == -1)
	{
		printf("Can't load picture file %s by DevIL \n", argv[1]);
		return -1;
	}
	glutIdleFunc(vIdle);		// Activation du callback

	glutKeyboardFunc(keypress);
	glutSpecialFunc(vSpecial); // Gestion des touches sp�ciales du clavier
	glutMainLoop(); // lance le gestionnaire glut
	return 0;
}

