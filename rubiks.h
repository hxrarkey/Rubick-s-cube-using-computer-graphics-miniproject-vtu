#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

//vector structure
struct vector {
	float x;
	float y;
	float z;
	float posx;
	float posy;
	float posz;
};

struct vertex {
	float x;
	float y;
	float z;
};

struct cubelet {
	int ID;
	int faceColors[6];
	struct cubelet* children[3][8];
};

//constants
#define TRANSLATE 0
#define SCALE 1
#define ROTATE 2
#define RED 0
#define ORANGE 1
#define WHITE 2
#define BLUE 3
#define GREEN 4
#define YELLOW 5
#define BLACK 6
#define GREY 7
#define FRONT 0
#define BACK 1
#define TOP 2
#define BOTTOM 3
#define LEFT 4 
#define RIGHT 5
#define FR 0
#define BA 2
#define LE 0
#define RI 2
#define BO 0
#define TO 2
#define MID 1
#define CLOCKWISE 10
#define C_CLOCKWISE 20
#define X_AXIS 0
#define Y_AXIS 1
#define Z_AXIS 2
#define C_W .3 //cubelet width
#define ANGLE_INCREMENT 5
#define PICK_REGION 5
#define BUFSIZE 512

//function declarations
float dot(struct vector*, struct vector*);
struct vector* cross_product(struct vector*, struct vector*);
void drawCube();
void display();
void reshape(int, int);
float p2w_x(int);
float p2w_y(int);
void mouse(int, int, int, int);
void mouseMove(int, int);
void keyboard(unsigned char, int, int);
void initialize_vectors();
void setColor(int);
void drawCubelet(struct cubelet*, struct vertex*);
void initCube();
void setFaceColor(int, int);
void updateHierarchy();
void rotateSlice(int, int, int);
void initCenterPositions();
void resetCubeletFaces(struct cubelet*);
void rotateCubelet(int, int, struct cubelet*);
void animateTurn(int, int, int);
void idle();
void renderObjs();
int doPicking(int, int);
void rotateSlice(int, int);
int getCubeletLocation(int);
void drawFacesPicking();
float p2i_x(int);
float p2i_y(int);
void trackBall();