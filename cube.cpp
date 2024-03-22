 //include file
#include "rubiks.h"

//global variables
int GW;					//global width
int GH;					//global height
int mouseX;				//saves first mouse click coordinates
int mouseY;				//saves first mouse click coordinates
int endX;				//saves ending mouse coordinates (for trackball)
int endY;				//saves ending mouse coordinates (for trackball)
bool draw_axis = true;	//draw axis boolean
struct vector* v1;		//Used for rotation
struct vector* v2;		//Used for rotation
struct vector* axis;	//Used for rotation (axis of rotation)
struct cubelet* cubelets[3][3][3];	//array to hold cubelets
struct vertex* centerPositions[3][3][3];	//array to hold center positions
bool animate = false;		//animate boolean
struct cubelet* A_cubelet;	//cubelet for slice rotations
int A_axis;					//axis for slice rotations (X_AXIS, etc)
int A_direction;			//direction for slice rotations (CLOCKWISE, etc)
float A_angle;				//angle for slice rotations
int A_slice;				//slice we are rotating (BA,LE,FR,etc)
bool mouse_click = false;	//mouse clicked boolean
GLuint selectBuf[BUFSIZE];	//select buffer for picking
bool picked_cubelet = false;	//is a cubelet currently selected?
int selected_cubelet;			//id of the selected cubelet
int cubeletLocations[3][3][3];	//locations of the cubelets within the cube
int selected_face;				//the currently selected face (picking)
bool track = false;			//trackball
float angle_of_rotation = 0;	//angle for rotation of the entire cube

//translation matrix for rotating the entire cube
float M[4][4] = { 1,0,0,0,
				 0,1,0,0,
				 0,0,1,0,
				 0,0,0,1 };

//translation matrix for rotating/animating slices
float rotate[4][4] = { 1,0,0,0,
					  0,1,0,0,
					  0,0,1,0,
					  0,0,0,1 };
//****************************************************************************
//DRAWING STUFF
//****************************************************************************
/** display
 *
 * The main display() method will be called whenever we refresh the screen
 */
void display() {
	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);

	renderObjs();

	glutSwapBuffers();
}
/** renderObjs()
 *
 * This method takes care of drawing the cube to the screen.  It applies the
 * cube rotation matrix and the individual slice rotation matrix.
 */
void renderObjs() {
	glLoadIdentity();

	//update cube rotation matrix M if we are dynamically rotating the cube
	if (track) {
		//trick to use the hardware to compute what the rotation matrix would be
		glPushMatrix();
		glLoadIdentity();
		glRotatef(angle_of_rotation, axis->x, axis->y, axis->z);
		glMultMatrixf((float*)M);
		glGetFloatv(GL_MODELVIEW_MATRIX, *M);
		glPopMatrix();
	}
	//apply the computed rotation
	glMultMatrixf((float*)M);

	//draw vertices
	if (draw_axis) {
		glColor3f(0, 1, 1);
		glBegin(GL_LINES);
		glVertex3f(0, 0, 0);
		glVertex3f(1, 0, 0);
		glVertex3f(0, 0, 0);
		glVertex3f(0, 1, 0);
		glVertex3f(0, 0, 0);
		glVertex3f(0, 0, 1);
		glEnd();
	}
	//are we animating this?
	if (animate == true) {
		//draw all the non-slice cubelets
		for (int i = 0; i < 3; i++)
			for (int j = 0; j < 3; j++)
				for (int k = 0; k < 3; k++) {
					if (cubelets[i][j][k]->ID == A_cubelet->children[A_axis][0]->ID ||
						cubelets[i][j][k]->ID == A_cubelet->children[A_axis][1]->ID ||
						cubelets[i][j][k]->ID == A_cubelet->children[A_axis][2]->ID ||
						cubelets[i][j][k]->ID == A_cubelet->children[A_axis][3]->ID ||
						cubelets[i][j][k]->ID == A_cubelet->children[A_axis][4]->ID ||
						cubelets[i][j][k]->ID == A_cubelet->children[A_axis][5]->ID ||
						cubelets[i][j][k]->ID == A_cubelet->children[A_axis][6]->ID ||
						cubelets[i][j][k]->ID == A_cubelet->children[A_axis][7]->ID ||
						cubelets[i][j][k]->ID == A_cubelet->ID)
					{
						//do nothing
					}
					else {
						drawCubelet(cubelets[i][j][k], centerPositions[i][j][k]);
					}
				}
		//apply the slice rotation matrix
		glMultMatrixf(*rotate);
		//draw all the slice cubelets
		for (int i = 0; i < 3; i++)
			for (int j = 0; j < 3; j++)
				for (int k = 0; k < 3; k++) {
					if (cubelets[i][j][k]->ID == A_cubelet->children[A_axis][0]->ID ||
						cubelets[i][j][k]->ID == A_cubelet->children[A_axis][1]->ID ||
						cubelets[i][j][k]->ID == A_cubelet->children[A_axis][2]->ID ||
						cubelets[i][j][k]->ID == A_cubelet->children[A_axis][3]->ID ||
						cubelets[i][j][k]->ID == A_cubelet->children[A_axis][4]->ID ||
						cubelets[i][j][k]->ID == A_cubelet->children[A_axis][5]->ID ||
						cubelets[i][j][k]->ID == A_cubelet->children[A_axis][6]->ID ||
						cubelets[i][j][k]->ID == A_cubelet->children[A_axis][7]->ID ||
						cubelets[i][j][k]->ID == A_cubelet->ID)
					{
						drawCubelet(cubelets[i][j][k], centerPositions[i][j][k]);
					}
				}
	}
	else { //animate == false
		//draw picking faces so we can determine which face we are selecting
		drawFacesPicking();
		//draw the cube
		for (int i = 0; i < 3; i++)
			for (int j = 0; j < 3; j++)
				for (int k = 0; k < 3; k++)
					drawCubelet(cubelets[i][j][k], centerPositions[i][j][k]);
	}
}

/** drawFacesPicking()
 *
 * This method will draw 6 entire faces on the cube (NOT individual cubelets)
 * and will push the appropriate face name (FRONT,BACK,BOTTOM,etc) onto the
 * name stack when drawing.  This will allow us to determine which face
 * the mouse is clicked on.  Note that the face is drawn slightly "below"
 * the "real" outer face to ensure it will not block the "real" face.
 */
void drawFacesPicking() {
	glLineWidth(1);
	setColor(BLACK);
	//initialize the namestack
	glInitNames();

	//draw each respective face, pushing the name onto the stack before drawing
	glPushName(FRONT);
	glBegin(GL_POLYGON);
	glVertex3f(-((C_W - .0001) * 1.5), -((C_W - .0001) * 1.5), ((C_W - .0001) * 1.5));
	glVertex3f(-((C_W - .0001) * 1.5), ((C_W - .0001) * 1.5), ((C_W - .0001) * 1.5));
	glVertex3f(((C_W - .0001) * 1.5), ((C_W - .0001) * 1.5), ((C_W - .0001) * 1.5));
	glVertex3f(((C_W - .0001) * 1.5), -((C_W - .0001) * 1.5), ((C_W - .0001) * 1.5));
	glEnd();
	glPopName();

	glPushName(BACK);
	glBegin(GL_POLYGON);
	glVertex3f(-((C_W - .0001) * 1.5), -((C_W - .0001) * 1.5), -((C_W - .0001) * 1.5));
	glVertex3f(-((C_W - .0001) * 1.5), ((C_W - .0001) * 1.5), -((C_W - .0001) * 1.5));
	glVertex3f(((C_W - .0001) * 1.5), ((C_W - .0001) * 1.5), -((C_W - .0001) * 1.5));
	glVertex3f(((C_W - .0001) * 1.5), -((C_W - .0001) * 1.5), -((C_W - .0001) * 1.5));
	glEnd();
	glPopName();

	glPushName(LEFT);
	glBegin(GL_POLYGON);
	glVertex3f(-((C_W - .0001) * 1.5), -((C_W - .0001) * 1.5), ((C_W - .0001) * 1.5));
	glVertex3f(-((C_W - .0001) * 1.5), -((C_W - .0001) * 1.5), -((C_W - .0001) * 1.5));
	glVertex3f(-((C_W - .0001) * 1.5), ((C_W - .0001) * 1.5), -((C_W - .0001) * 1.5));
	glVertex3f(-((C_W - .0001) * 1.5), ((C_W - .0001) * 1.5), ((C_W - .0001) * 1.5));
	glEnd();
	glPopName();

	glPushName(RIGHT);
	glBegin(GL_POLYGON);
	glVertex3f(((C_W - .0001) * 1.5), -((C_W - .0001) * 1.5), ((C_W - .0001) * 1.5));
	glVertex3f(((C_W - .0001) * 1.5), -((C_W - .0001) * 1.5), -((C_W - .0001) * 1.5));
	glVertex3f(((C_W - .0001) * 1.5), ((C_W - .0001) * 1.5), -((C_W - .0001) * 1.5));
	glVertex3f(((C_W - .0001) * 1.5), ((C_W - .0001) * 1.5), ((C_W - .0001) * 1.5));
	glEnd();
	glPopName();

	glPushName(TOP);
	glBegin(GL_POLYGON);
	glVertex3f(-((C_W - .0001) * 1.5), ((C_W - .0001) * 1.5), ((C_W - .0001) * 1.5));
	glVertex3f(-((C_W - .0001) * 1.5), ((C_W - .0001) * 1.5), -((C_W - .0001) * 1.5));
	glVertex3f(((C_W - .0001) * 1.5), ((C_W - .0001) * 1.5), -((C_W - .0001) * 1.5));
	glVertex3f(((C_W - .0001) * 1.5), ((C_W - .0001) * 1.5), ((C_W - .0001) * 1.5));
	glEnd();
	glPopName();

	glPushName(BOTTOM);
	glBegin(GL_POLYGON);
	glVertex3f(-((C_W - .0001) * 1.5), -((C_W - .0001) * 1.5), ((C_W - .0001) * 1.5));
	glVertex3f(-((C_W - .0001) * 1.5), -((C_W - .0001) * 1.5), -((C_W - .0001) * 1.5));
	glVertex3f(((C_W - .0001) * 1.5), -((C_W - .0001) * 1.5), -((C_W - .0001) * 1.5));
	glVertex3f(((C_W - .0001) * 1.5), -((C_W - .0001) * 1.5), ((C_W - .0001) * 1.5));
	glEnd();
	glPopName();
}

/** drawCubelet()
 *
 * This method will draw each individual cubelet.  It takes a reference to
 * the cubelet and a vertex which represents it's center position in
 * world coordinates.
 */
void drawCubelet(struct cubelet* c, struct vertex* center) {
	//draw the center positions as points (debugging)
	setColor(WHITE);
	glPointSize(2.0);
	glBegin(GL_POINTS);
	glVertex3f(center->x, center->y, center->z);
	glEnd();

	//push the name of this cubelet for picking
	glInitNames();
	glPushName(c->ID);

	//draw edges
	setColor(BLACK);
	glLineWidth(2);
	glBegin(GL_LINE_LOOP);
	glVertex3f(center->x - (C_W / 2), center->y - (C_W / 2), center->z + (C_W / 2));
	glVertex3f(center->x - (C_W / 2), center->y + (C_W / 2), center->z + (C_W / 2));
	glVertex3f(center->x + (C_W / 2), center->y + (C_W / 2), center->z + (C_W / 2));
	glVertex3f(center->x + (C_W / 2), center->y - (C_W / 2), center->z + (C_W / 2));
	glVertex3f(center->x - (C_W / 2), center->y - (C_W / 2), center->z + (C_W / 2));
	glVertex3f(center->x - (C_W / 2), center->y - (C_W / 2), center->z - (C_W / 2));
	glVertex3f(center->x - (C_W / 2), center->y + (C_W / 2), center->z - (C_W / 2));
	glVertex3f(center->x + (C_W / 2), center->y + (C_W / 2), center->z - (C_W / 2));
	glVertex3f(center->x + (C_W / 2), center->y - (C_W / 2), center->z - (C_W / 2));
	glVertex3f(center->x - (C_W / 2), center->y - (C_W / 2), center->z - (C_W / 2));
	glEnd();
	glBegin(GL_LINES);
	glVertex3f(center->x - (C_W / 2), center->y + (C_W / 2), center->z + (C_W / 2));
	glVertex3f(center->x - (C_W / 2), center->y + (C_W / 2), center->z - (C_W / 2));

	glVertex3f(center->x + (C_W / 2), center->y + (C_W / 2), center->z + (C_W / 2));
	glVertex3f(center->x + (C_W / 2), center->y + (C_W / 2), center->z - (C_W / 2));

	glVertex3f(center->x + (C_W / 2), center->y - (C_W / 2), center->z + (C_W / 2));
	glVertex3f(center->x + (C_W / 2), center->y - (C_W / 2), center->z - (C_W / 2));
	glEnd();

	//front face
	setColor(c->faceColors[FRONT]);
	glBegin(GL_POLYGON);
	glVertex3f(center->x - (C_W / 2), center->y - (C_W / 2), center->z + (C_W / 2));
	glVertex3f(center->x - (C_W / 2), center->y + (C_W / 2), center->z + (C_W / 2));
	glVertex3f(center->x + (C_W / 2), center->y + (C_W / 2), center->z + (C_W / 2));
	glVertex3f(center->x + (C_W / 2), center->y - (C_W / 2), center->z + (C_W / 2));
	glEnd();

	//back face
	setColor(c->faceColors[BACK]);
	glBegin(GL_POLYGON);
	glVertex3f(center->x - (C_W / 2), center->y - (C_W / 2), center->z - (C_W / 2));
	glVertex3f(center->x - (C_W / 2), center->y + (C_W / 2), center->z - (C_W / 2));
	glVertex3f(center->x + (C_W / 2), center->y + (C_W / 2), center->z - (C_W / 2));
	glVertex3f(center->x + (C_W / 2), center->y - (C_W / 2), center->z - (C_W / 2));
	glEnd();

	//top face
	setColor(c->faceColors[TOP]);
	glBegin(GL_POLYGON);
	glVertex3f(center->x - (C_W / 2), center->y + (C_W / 2), center->z + (C_W / 2));
	glVertex3f(center->x - (C_W / 2), center->y + (C_W / 2), center->z - (C_W / 2));
	glVertex3f(center->x + (C_W / 2), center->y + (C_W / 2), center->z - (C_W / 2));
	glVertex3f(center->x + (C_W / 2), center->y + (C_W / 2), center->z + (C_W / 2));
	glEnd();

	//bottom face
	setColor(c->faceColors[BOTTOM]);
	glBegin(GL_POLYGON);
	glVertex3f(center->x - (C_W / 2), center->y - (C_W / 2), center->z + (C_W / 2));
	glVertex3f(center->x - (C_W / 2), center->y - (C_W / 2), center->z - (C_W / 2));
	glVertex3f(center->x + (C_W / 2), center->y - (C_W / 2), center->z - (C_W / 2));
	glVertex3f(center->x + (C_W / 2), center->y - (C_W / 2), center->z + (C_W / 2));
	glEnd();

	//left face
	setColor(c->faceColors[LEFT]);
	glBegin(GL_POLYGON);
	glVertex3f(center->x - (C_W / 2), center->y + (C_W / 2), center->z + (C_W / 2));
	glVertex3f(center->x - (C_W / 2), center->y + (C_W / 2), center->z - (C_W / 2));
	glVertex3f(center->x - (C_W / 2), center->y - (C_W / 2), center->z - (C_W / 2));
	glVertex3f(center->x - (C_W / 2), center->y - (C_W / 2), center->z + (C_W / 2));
	glEnd();

	//right face
	setColor(c->faceColors[RIGHT]);
	glBegin(GL_POLYGON);
	glVertex3f(center->x + (C_W / 2), center->y + (C_W / 2), center->z + (C_W / 2));
	glVertex3f(center->x + (C_W / 2), center->y + (C_W / 2), center->z - (C_W / 2));
	glVertex3f(center->x + (C_W / 2), center->y - (C_W / 2), center->z - (C_W / 2));
	glVertex3f(center->x + (C_W / 2), center->y - (C_W / 2), center->z + (C_W / 2));
	glEnd();

	//for picking
	glPopName();
}



//****************************************************************************
//MOUSE STUFF
//****************************************************************************
/** mouse()
 *
 * This event callback is executed whenever there is a mouse event
 */
void mouse(int button, int state, int x, int y) {
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN && animate == false) {
		mouse_click = true;
		//printf("mouse clicked at %d %d\n", x, y);
		mouseX = x;
		mouseY = GH - y - 1;
		//check to see if we've selected a cubelet
		if ((selected_cubelet = doPicking(x, y)) < 0)
			picked_cubelet = false;
		else
			picked_cubelet = true;

		//initialize v1,v2 for trackball rotations
		initialize_vectors();
	}
	else if (button == GLUT_LEFT_BUTTON && state == GLUT_UP && animate == false) {
		mouse_click = false;
		angle_of_rotation = 0;
	}
}

/** mouseMove
 *
 * This even callback is executed whenver the mouse is moved
 */
void mouseMove(int x, int y) {
	//printf("mouse moved at %d %d\n", x, GH-y-1);
	if (animate == false && mouse_click == true && picked_cubelet == false) {
		endX = x;
		endY = GH - y - 1;
		track = true;
		//do trackball stuff
		trackBall();
		glutPostRedisplay();
	}
	else if (picked_cubelet == true) {
		int i;
		//check to see if the mouse has changed positions (from one cubelet
		//to another cubelet).  If so, then we need to do a slice
		//rotation.
		if (selected_cubelet != (i = doPicking(x, y))) {
			rotateSlice(selected_cubelet, i);
			picked_cubelet = false;
			mouse_click = false;
		}
	}
}

//****************************************************************************
//PICKING/ROTATION STUFF
//****************************************************************************
  /** doPicking()
 *
 * This method takes in two mouse coordinates and will return the name of
 * the cubelet that is selected.  It will also set the global variable
 * selected_face to the face that lies under the mouse coordinates.
 */
int doPicking(int x, int y) {
	GLint viewport[4];

	//set the select buffer and enter SELECT mode
	glSelectBuffer(BUFSIZE, selectBuf);
	glRenderMode(GL_SELECT);

	//store the current viewing
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	//set up the viewing
	glGetIntegerv(GL_VIEWPORT, viewport);

	//set up pixel picking region near mouse
	gluPickMatrix((GLdouble)x, (GLdouble)viewport[3] - y, PICK_REGION, PICK_REGION, viewport);
	glOrtho((-(float)GW / GH), ((float)GW / GH), -1, 1, -1, 1);

	//return to model matrix
	glMatrixMode(GL_MODELVIEW);

	//draw the cube
	renderObjs();

	// restoring the original projection matrix
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	glutSwapBuffers();

	//get the number of hits
	unsigned int hits = glRenderMode(GL_RENDER);

	if (hits != 0) { //we clicked on the cube somewhere
		unsigned int i;
		GLuint names, * ptr, minZ, * ptrNames, numberOfNames;

		ptr = (GLuint*)selectBuf;
		ptrNames = ptr;
		minZ = 0xffffffff;
		for (i = 0; i < hits; i++) {
			names = *ptr;
			ptr++;
			if (*ptr < minZ) {
				selected_face = *ptrNames;
				numberOfNames = names;
				minZ = *ptr;
				ptrNames = ptr + 2;
			}

			ptr += names + 2;
		}
		//printf ("Selected cubelet: [%d]\n",*ptrNames);
		//printf ("Selected face: [%d]\n",selected_face);
		//return the name of the selected cubelet
		return *ptrNames;
	}
	return -1;
}

/** rotateSlice()
 *
 * This function is the entry point into the slice rotation functionality.
 * This function is called by the mouse_move function when we have determined
 * that we need to do a slice rotation.  The ID of the original cubelet (first
 * click) and the new cublet (adjacent cubelet that the mouse is dragged onto)
 * are passed in as arguments.
 */
void rotateSlice(int old_cubelet, int new_cubelet) {

	//check which face we are dealing with
	if (selected_face == FRONT) {
		if (getCubeletLocation(old_cubelet) == 0 && getCubeletLocation(new_cubelet) == 3 ||
			getCubeletLocation(old_cubelet) == 3 && getCubeletLocation(new_cubelet) == 6)
			animateTurn(X_AXIS, C_CLOCKWISE, LE);
		else if (getCubeletLocation(old_cubelet) == 0 && getCubeletLocation(new_cubelet) == 1 ||
			getCubeletLocation(old_cubelet) == 1 && getCubeletLocation(new_cubelet) == 2)
			animateTurn(Y_AXIS, CLOCKWISE, BO);
		else if (getCubeletLocation(old_cubelet) == 1 && getCubeletLocation(new_cubelet) == 0 ||
			getCubeletLocation(old_cubelet) == 2 && getCubeletLocation(new_cubelet) == 1)
			animateTurn(Y_AXIS, C_CLOCKWISE, BO);
		else if (getCubeletLocation(old_cubelet) == 2 && getCubeletLocation(new_cubelet) == 5 ||
			getCubeletLocation(old_cubelet) == 5 && getCubeletLocation(new_cubelet) == 8)
			animateTurn(X_AXIS, C_CLOCKWISE, RI);
		else if (getCubeletLocation(old_cubelet) == 3 && getCubeletLocation(new_cubelet) == 4 ||
			getCubeletLocation(old_cubelet) == 4 && getCubeletLocation(new_cubelet) == 5)
			animateTurn(Y_AXIS, CLOCKWISE, MID);
		else if (getCubeletLocation(old_cubelet) == 3 && getCubeletLocation(new_cubelet) == 0 ||
			getCubeletLocation(old_cubelet) == 6 && getCubeletLocation(new_cubelet) == 3)
			animateTurn(X_AXIS, CLOCKWISE, LE);
		else if (getCubeletLocation(old_cubelet) == 4 && getCubeletLocation(new_cubelet) == 3 ||
			getCubeletLocation(old_cubelet) == 5 && getCubeletLocation(new_cubelet) == 4)
			animateTurn(Y_AXIS, C_CLOCKWISE, MID);
		else if (getCubeletLocation(old_cubelet) == 4 && getCubeletLocation(new_cubelet) == 1 ||
			getCubeletLocation(old_cubelet) == 7 && getCubeletLocation(new_cubelet) == 4)
			animateTurn(X_AXIS, CLOCKWISE, MID);
		else if (getCubeletLocation(old_cubelet) == 4 && getCubeletLocation(new_cubelet) == 7 ||
			getCubeletLocation(old_cubelet) == 1 && getCubeletLocation(new_cubelet) == 4)
			animateTurn(X_AXIS, C_CLOCKWISE, MID);
		else if (getCubeletLocation(old_cubelet) == 5 && getCubeletLocation(new_cubelet) == 2 ||
			getCubeletLocation(old_cubelet) == 8 && getCubeletLocation(new_cubelet) == 5)
			animateTurn(X_AXIS, CLOCKWISE, RI);
		else if (getCubeletLocation(old_cubelet) == 6 && getCubeletLocation(new_cubelet) == 7 ||
			getCubeletLocation(old_cubelet) == 7 && getCubeletLocation(new_cubelet) == 8)
			animateTurn(Y_AXIS, CLOCKWISE, TO);
		else if (getCubeletLocation(old_cubelet) == 7 && getCubeletLocation(new_cubelet) == 6 ||
			getCubeletLocation(old_cubelet) == 8 && getCubeletLocation(new_cubelet) == 7)
			animateTurn(Y_AXIS, C_CLOCKWISE, TO);
		else
			printf("SHOULD NEVER HAPPEN\n");
	}
	else if (selected_face == BACK) {
		if (getCubeletLocation(old_cubelet) == 21 && getCubeletLocation(new_cubelet) == 24 ||
			getCubeletLocation(old_cubelet) == 18 && getCubeletLocation(new_cubelet) == 21)
			animateTurn(X_AXIS, CLOCKWISE, LE);
		else if (getCubeletLocation(old_cubelet) == 19 && getCubeletLocation(new_cubelet) == 20 ||
			getCubeletLocation(old_cubelet) == 18 && getCubeletLocation(new_cubelet) == 19)
			animateTurn(Y_AXIS, C_CLOCKWISE, BO);
		else if (getCubeletLocation(old_cubelet) == 20 && getCubeletLocation(new_cubelet) == 19 ||
			getCubeletLocation(old_cubelet) == 19 && getCubeletLocation(new_cubelet) == 18)
			animateTurn(Y_AXIS, CLOCKWISE, BO);
		else if (getCubeletLocation(old_cubelet) == 22 && getCubeletLocation(new_cubelet) == 25 ||
			getCubeletLocation(old_cubelet) == 19 && getCubeletLocation(new_cubelet) == 22)
			animateTurn(X_AXIS, CLOCKWISE, MID);
		else if (getCubeletLocation(old_cubelet) == 23 && getCubeletLocation(new_cubelet) == 26 ||
			getCubeletLocation(old_cubelet) == 20 && getCubeletLocation(new_cubelet) == 23)
			animateTurn(X_AXIS, CLOCKWISE, RI);
		else if (getCubeletLocation(old_cubelet) == 22 && getCubeletLocation(new_cubelet) == 23 ||
			getCubeletLocation(old_cubelet) == 21 && getCubeletLocation(new_cubelet) == 22)
			animateTurn(Y_AXIS, C_CLOCKWISE, MID);
		else if (getCubeletLocation(old_cubelet) == 24 && getCubeletLocation(new_cubelet) == 21 ||
			getCubeletLocation(old_cubelet) == 21 && getCubeletLocation(new_cubelet) == 18)
			animateTurn(X_AXIS, C_CLOCKWISE, LE);
		else if (getCubeletLocation(old_cubelet) == 23 && getCubeletLocation(new_cubelet) == 22 ||
			getCubeletLocation(old_cubelet) == 22 && getCubeletLocation(new_cubelet) == 21)
			animateTurn(Y_AXIS, CLOCKWISE, MID);
		else if (getCubeletLocation(old_cubelet) == 25 && getCubeletLocation(new_cubelet) == 22 ||
			getCubeletLocation(old_cubelet) == 22 && getCubeletLocation(new_cubelet) == 19)
			animateTurn(X_AXIS, C_CLOCKWISE, MID);
		else if (getCubeletLocation(old_cubelet) == 26 && getCubeletLocation(new_cubelet) == 23 ||
			getCubeletLocation(old_cubelet) == 23 && getCubeletLocation(new_cubelet) == 20)
			animateTurn(X_AXIS, C_CLOCKWISE, RI);
		else if (getCubeletLocation(old_cubelet) == 25 && getCubeletLocation(new_cubelet) == 26 ||
			getCubeletLocation(old_cubelet) == 24 && getCubeletLocation(new_cubelet) == 25)
			animateTurn(Y_AXIS, C_CLOCKWISE, TO);
		else if (getCubeletLocation(old_cubelet) == 26 && getCubeletLocation(new_cubelet) == 25 ||
			getCubeletLocation(old_cubelet) == 25 && getCubeletLocation(new_cubelet) == 24)
			animateTurn(Y_AXIS, CLOCKWISE, TO);
		else
			printf("SHOULD NEVER HAPPEN\n");
	}
	else if (selected_face == BOTTOM) {
		if (getCubeletLocation(old_cubelet) == 18 && getCubeletLocation(new_cubelet) == 19 ||
			getCubeletLocation(old_cubelet) == 19 && getCubeletLocation(new_cubelet) == 20)
			animateTurn(Z_AXIS, CLOCKWISE, BA);
		else if (getCubeletLocation(old_cubelet) == 18 && getCubeletLocation(new_cubelet) == 9 ||
			getCubeletLocation(old_cubelet) == 9 && getCubeletLocation(new_cubelet) == 0)
			animateTurn(X_AXIS, C_CLOCKWISE, LE);
		else if (getCubeletLocation(old_cubelet) == 19 && getCubeletLocation(new_cubelet) == 18 ||
			getCubeletLocation(old_cubelet) == 20 && getCubeletLocation(new_cubelet) == 19)
			animateTurn(Z_AXIS, C_CLOCKWISE, BA);
		else if (getCubeletLocation(old_cubelet) == 19 && getCubeletLocation(new_cubelet) == 10 ||
			getCubeletLocation(old_cubelet) == 10 && getCubeletLocation(new_cubelet) == 1)
			animateTurn(X_AXIS, C_CLOCKWISE, MID);
		else if (getCubeletLocation(old_cubelet) == 20 && getCubeletLocation(new_cubelet) == 11 ||
			getCubeletLocation(old_cubelet) == 11 && getCubeletLocation(new_cubelet) == 2)
			animateTurn(X_AXIS, C_CLOCKWISE, RI);
		else if (getCubeletLocation(old_cubelet) == 9 && getCubeletLocation(new_cubelet) == 10 ||
			getCubeletLocation(old_cubelet) == 10 && getCubeletLocation(new_cubelet) == 11)
			animateTurn(Z_AXIS, CLOCKWISE, MID);
		else if (getCubeletLocation(old_cubelet) == 9 && getCubeletLocation(new_cubelet) == 18 ||
			getCubeletLocation(old_cubelet) == 0 && getCubeletLocation(new_cubelet) == 9)
			animateTurn(X_AXIS, CLOCKWISE, LE);
		else if (getCubeletLocation(old_cubelet) == 10 && getCubeletLocation(new_cubelet) == 19 ||
			getCubeletLocation(old_cubelet) == 1 && getCubeletLocation(new_cubelet) == 10)
			animateTurn(X_AXIS, CLOCKWISE, MID);
		else if (getCubeletLocation(old_cubelet) == 10 && getCubeletLocation(new_cubelet) == 9 ||
			getCubeletLocation(old_cubelet) == 11 && getCubeletLocation(new_cubelet) == 10)
			animateTurn(Z_AXIS, C_CLOCKWISE, MID);
		else if (getCubeletLocation(old_cubelet) == 11 && getCubeletLocation(new_cubelet) == 20 ||
			getCubeletLocation(old_cubelet) == 2 && getCubeletLocation(new_cubelet) == 11)
			animateTurn(X_AXIS, CLOCKWISE, RI);
		else if (getCubeletLocation(old_cubelet) == 0 && getCubeletLocation(new_cubelet) == 1 ||
			getCubeletLocation(old_cubelet) == 1 && getCubeletLocation(new_cubelet) == 2)
			animateTurn(Z_AXIS, CLOCKWISE, FR);
		else if (getCubeletLocation(old_cubelet) == 1 && getCubeletLocation(new_cubelet) == 0 ||
			getCubeletLocation(old_cubelet) == 2 && getCubeletLocation(new_cubelet) == 1)
			animateTurn(Z_AXIS, C_CLOCKWISE, FR);
		else
			printf("SHOULD NEVER HAPPEN\n");
	}
	else if (selected_face == TOP) {
		if (getCubeletLocation(old_cubelet) == 25 && getCubeletLocation(new_cubelet) == 26 ||
			getCubeletLocation(old_cubelet) == 24 && getCubeletLocation(new_cubelet) == 25)
			animateTurn(Z_AXIS, C_CLOCKWISE, BA);
		else if (getCubeletLocation(old_cubelet) == 15 && getCubeletLocation(new_cubelet) == 6 ||
			getCubeletLocation(old_cubelet) == 24 && getCubeletLocation(new_cubelet) == 15)
			animateTurn(X_AXIS, CLOCKWISE, LE);
		else if (getCubeletLocation(old_cubelet) == 26 && getCubeletLocation(new_cubelet) == 25 ||
			getCubeletLocation(old_cubelet) == 25 && getCubeletLocation(new_cubelet) == 24)
			animateTurn(Z_AXIS, CLOCKWISE, BA);
		else if (getCubeletLocation(old_cubelet) == 16 && getCubeletLocation(new_cubelet) == 7 ||
			getCubeletLocation(old_cubelet) == 25 && getCubeletLocation(new_cubelet) == 16)
			animateTurn(X_AXIS, CLOCKWISE, MID);
		else if (getCubeletLocation(old_cubelet) == 17 && getCubeletLocation(new_cubelet) == 8 ||
			getCubeletLocation(old_cubelet) == 26 && getCubeletLocation(new_cubelet) == 17)
			animateTurn(X_AXIS, CLOCKWISE, RI);
		else if (getCubeletLocation(old_cubelet) == 16 && getCubeletLocation(new_cubelet) == 17 ||
			getCubeletLocation(old_cubelet) == 15 && getCubeletLocation(new_cubelet) == 16)
			animateTurn(Z_AXIS, C_CLOCKWISE, MID);
		else if (getCubeletLocation(old_cubelet) == 6 && getCubeletLocation(new_cubelet) == 15 ||
			getCubeletLocation(old_cubelet) == 15 && getCubeletLocation(new_cubelet) == 24)
			animateTurn(X_AXIS, C_CLOCKWISE, LE);
		else if (getCubeletLocation(old_cubelet) == 7 && getCubeletLocation(new_cubelet) == 16 ||
			getCubeletLocation(old_cubelet) == 16 && getCubeletLocation(new_cubelet) == 25)
			animateTurn(X_AXIS, C_CLOCKWISE, MID);
		else if (getCubeletLocation(old_cubelet) == 17 && getCubeletLocation(new_cubelet) == 16 ||
			getCubeletLocation(old_cubelet) == 16 && getCubeletLocation(new_cubelet) == 15)
			animateTurn(Z_AXIS, CLOCKWISE, MID);
		else if (getCubeletLocation(old_cubelet) == 8 && getCubeletLocation(new_cubelet) == 17 ||
			getCubeletLocation(old_cubelet) == 17 && getCubeletLocation(new_cubelet) == 26)
			animateTurn(X_AXIS, C_CLOCKWISE, RI);
		else if (getCubeletLocation(old_cubelet) == 7 && getCubeletLocation(new_cubelet) == 8 ||
			getCubeletLocation(old_cubelet) == 6 && getCubeletLocation(new_cubelet) == 7)
			animateTurn(Z_AXIS, C_CLOCKWISE, FR);
		else if (getCubeletLocation(old_cubelet) == 8 && getCubeletLocation(new_cubelet) == 7 ||
			getCubeletLocation(old_cubelet) == 7 && getCubeletLocation(new_cubelet) == 6)
			animateTurn(Z_AXIS, CLOCKWISE, FR);
		else
			printf("SHOULD NEVER HAPPEN\n");
	}
	else if (selected_face == LEFT) {
		if (getCubeletLocation(old_cubelet) == 9 && getCubeletLocation(new_cubelet) == 0 ||
			getCubeletLocation(old_cubelet) == 18 && getCubeletLocation(new_cubelet) == 9)
			animateTurn(Y_AXIS, CLOCKWISE, BO);
		else if (getCubeletLocation(old_cubelet) == 21 && getCubeletLocation(new_cubelet) == 24 ||
			getCubeletLocation(old_cubelet) == 18 && getCubeletLocation(new_cubelet) == 21)
			animateTurn(Z_AXIS, C_CLOCKWISE, BA);
		else if (getCubeletLocation(old_cubelet) == 0 && getCubeletLocation(new_cubelet) == 9 ||
			getCubeletLocation(old_cubelet) == 9 && getCubeletLocation(new_cubelet) == 18)
			animateTurn(Y_AXIS, C_CLOCKWISE, BO);
		else if (getCubeletLocation(old_cubelet) == 12 && getCubeletLocation(new_cubelet) == 15 ||
			getCubeletLocation(old_cubelet) == 9 && getCubeletLocation(new_cubelet) == 12)
			animateTurn(Z_AXIS, C_CLOCKWISE, MID);
		else if (getCubeletLocation(old_cubelet) == 3 && getCubeletLocation(new_cubelet) == 6 ||
			getCubeletLocation(old_cubelet) == 0 && getCubeletLocation(new_cubelet) == 3)
			animateTurn(Z_AXIS, C_CLOCKWISE, FR);
		else if (getCubeletLocation(old_cubelet) == 21 && getCubeletLocation(new_cubelet) == 18 ||
			getCubeletLocation(old_cubelet) == 24 && getCubeletLocation(new_cubelet) == 21)
			animateTurn(Z_AXIS, CLOCKWISE, BA);
		else if (getCubeletLocation(old_cubelet) == 12 && getCubeletLocation(new_cubelet) == 3 ||
			getCubeletLocation(old_cubelet) == 21 && getCubeletLocation(new_cubelet) == 12)
			animateTurn(Y_AXIS, CLOCKWISE, MID);
		else if (getCubeletLocation(old_cubelet) == 15 && getCubeletLocation(new_cubelet) == 12 ||
			getCubeletLocation(old_cubelet) == 12 && getCubeletLocation(new_cubelet) == 9)
			animateTurn(Z_AXIS, CLOCKWISE, MID);
		else if (getCubeletLocation(old_cubelet) == 3 && getCubeletLocation(new_cubelet) == 12 ||
			getCubeletLocation(old_cubelet) == 12 && getCubeletLocation(new_cubelet) == 21)
			animateTurn(Y_AXIS, C_CLOCKWISE, MID);
		else if (getCubeletLocation(old_cubelet) == 6 && getCubeletLocation(new_cubelet) == 3 ||
			getCubeletLocation(old_cubelet) == 3 && getCubeletLocation(new_cubelet) == 0)
			animateTurn(Z_AXIS, CLOCKWISE, FR);
		else if (getCubeletLocation(old_cubelet) == 15 && getCubeletLocation(new_cubelet) == 6 ||
			getCubeletLocation(old_cubelet) == 24 && getCubeletLocation(new_cubelet) == 15)
			animateTurn(Y_AXIS, CLOCKWISE, TO);
		else if (getCubeletLocation(old_cubelet) == 6 && getCubeletLocation(new_cubelet) == 15 ||
			getCubeletLocation(old_cubelet) == 15 && getCubeletLocation(new_cubelet) == 24)
			animateTurn(Y_AXIS, C_CLOCKWISE, TO);
		else
			printf("SHOULD NEVER HAPPEN\n");
	}
	else if (selected_face == RIGHT) {
		if (getCubeletLocation(old_cubelet) == 11 && getCubeletLocation(new_cubelet) == 2 ||
			getCubeletLocation(old_cubelet) == 20 && getCubeletLocation(new_cubelet) == 11)
			animateTurn(Y_AXIS, C_CLOCKWISE, BO);
		else if (getCubeletLocation(old_cubelet) == 23 && getCubeletLocation(new_cubelet) == 26 ||
			getCubeletLocation(old_cubelet) == 20 && getCubeletLocation(new_cubelet) == 23)
			animateTurn(Z_AXIS, CLOCKWISE, BA);
		else if (getCubeletLocation(old_cubelet) == 2 && getCubeletLocation(new_cubelet) == 11 ||
			getCubeletLocation(old_cubelet) == 11 && getCubeletLocation(new_cubelet) == 20)
			animateTurn(Y_AXIS, CLOCKWISE, BO);
		else if (getCubeletLocation(old_cubelet) == 14 && getCubeletLocation(new_cubelet) == 17 ||
			getCubeletLocation(old_cubelet) == 11 && getCubeletLocation(new_cubelet) == 14)
			animateTurn(Z_AXIS, CLOCKWISE, MID);
		else if (getCubeletLocation(old_cubelet) == 5 && getCubeletLocation(new_cubelet) == 8 ||
			getCubeletLocation(old_cubelet) == 2 && getCubeletLocation(new_cubelet) == 5)
			animateTurn(Z_AXIS, CLOCKWISE, FR);
		else if (getCubeletLocation(old_cubelet) == 14 && getCubeletLocation(new_cubelet) == 5 ||
			getCubeletLocation(old_cubelet) == 23 && getCubeletLocation(new_cubelet) == 14)
			animateTurn(Y_AXIS, C_CLOCKWISE, MID);
		else if (getCubeletLocation(old_cubelet) == 26 && getCubeletLocation(new_cubelet) == 23 ||
			getCubeletLocation(old_cubelet) == 23 && getCubeletLocation(new_cubelet) == 20)
			animateTurn(Z_AXIS, C_CLOCKWISE, BA);
		else if (getCubeletLocation(old_cubelet) == 17 && getCubeletLocation(new_cubelet) == 14 ||
			getCubeletLocation(old_cubelet) == 14 && getCubeletLocation(new_cubelet) == 11)
			animateTurn(Z_AXIS, C_CLOCKWISE, MID);
		else if (getCubeletLocation(old_cubelet) == 5 && getCubeletLocation(new_cubelet) == 14 ||
			getCubeletLocation(old_cubelet) == 14 && getCubeletLocation(new_cubelet) == 23)
			animateTurn(Y_AXIS, CLOCKWISE, MID);
		else if (getCubeletLocation(old_cubelet) == 8 && getCubeletLocation(new_cubelet) == 5 ||
			getCubeletLocation(old_cubelet) == 5 && getCubeletLocation(new_cubelet) == 2)
			animateTurn(Z_AXIS, C_CLOCKWISE, FR);
		else if (getCubeletLocation(old_cubelet) == 17 && getCubeletLocation(new_cubelet) == 8 ||
			getCubeletLocation(old_cubelet) == 26 && getCubeletLocation(new_cubelet) == 17)
			animateTurn(Y_AXIS, C_CLOCKWISE, TO);
		else if (getCubeletLocation(old_cubelet) == 8 && getCubeletLocation(new_cubelet) == 17 ||
			getCubeletLocation(old_cubelet) == 17 && getCubeletLocation(new_cubelet) == 26)
			animateTurn(Y_AXIS, CLOCKWISE, TO);
		else
			printf("SHOULD NEVER HAPPEN\n");
	}
	else {
		printf("SHOULD NEVER HAPPEN\n");
	}

}
/** rotateSlice
 *
 * This function will "rotate" a slice of the cube by changing the cubelts[][][]
 * array.  It updates the pointers contained in this array to reflect the
 * cubelet's new positions in the cube.
 */
void rotateSlice(int axis, int direction, int slice) {
	if (axis == Z_AXIS) {
		if (direction == C_CLOCKWISE) {
			//corner pieces
			struct cubelet* c = cubelets[LE][BO][slice];
			cubelets[LE][BO][slice] = cubelets[RI][BO][slice];
			rotateCubelet(axis, direction, cubelets[LE][BO][slice]);

			cubelets[RI][BO][slice] = cubelets[RI][TO][slice];
			rotateCubelet(axis, direction, cubelets[RI][BO][slice]);

			cubelets[RI][TO][slice] = cubelets[LE][TO][slice];
			rotateCubelet(axis, direction, cubelets[RI][TO][slice]);

			cubelets[LE][TO][slice] = c;
			rotateCubelet(axis, direction, cubelets[LE][TO][slice]);

			//edge pieces
			c = cubelets[LE][MID][slice];
			cubelets[LE][MID][slice] = cubelets[MID][BO][slice];
			rotateCubelet(axis, direction, cubelets[LE][MID][slice]);

			cubelets[MID][BO][slice] = cubelets[RI][MID][slice];
			rotateCubelet(axis, direction, cubelets[MID][BO][slice]);

			cubelets[RI][MID][slice] = cubelets[MID][TO][slice];
			rotateCubelet(axis, direction, cubelets[RI][MID][slice]);

			cubelets[MID][TO][slice] = c;
			rotateCubelet(axis, direction, cubelets[MID][TO][slice]);
		}
		else if (direction == CLOCKWISE) {
			struct cubelet* c = cubelets[LE][BO][slice];
			cubelets[LE][BO][slice] = cubelets[LE][TO][slice];
			rotateCubelet(axis, direction, cubelets[LE][BO][slice]);

			cubelets[LE][TO][slice] = cubelets[RI][TO][slice];
			rotateCubelet(axis, direction, cubelets[LE][TO][slice]);

			cubelets[RI][TO][slice] = cubelets[RI][BO][slice];
			rotateCubelet(axis, direction, cubelets[RI][TO][slice]);

			cubelets[RI][BO][slice] = c;
			rotateCubelet(axis, direction, cubelets[RI][BO][slice]);

			c = cubelets[LE][MID][slice];
			cubelets[LE][MID][slice] = cubelets[MID][TO][slice];
			rotateCubelet(axis, direction, cubelets[LE][MID][slice]);

			cubelets[MID][TO][slice] = cubelets[RI][MID][slice];
			rotateCubelet(axis, direction, cubelets[MID][TO][slice]);

			cubelets[RI][MID][slice] = cubelets[MID][BO][slice];
			rotateCubelet(axis, direction, cubelets[RI][MID][slice]);

			cubelets[MID][BO][slice] = c;
			rotateCubelet(axis, direction, cubelets[MID][BO][slice]);

		}
		else {
			printf("should never happen\n");
		}
	}
	else if (axis == Y_AXIS) {
		if (direction == C_CLOCKWISE) {
			//corner pieces
			struct cubelet* c = cubelets[LE][slice][FR];
			cubelets[LE][slice][FR] = cubelets[RI][slice][FR];
			rotateCubelet(axis, direction, cubelets[LE][slice][FR]);

			cubelets[RI][slice][FR] = cubelets[RI][slice][BA];
			rotateCubelet(axis, direction, cubelets[RI][slice][FR]);

			cubelets[RI][slice][BA] = cubelets[LE][slice][BA];
			rotateCubelet(axis, direction, cubelets[RI][slice][BA]);

			cubelets[LE][slice][BA] = c;
			rotateCubelet(axis, direction, cubelets[LE][slice][BA]);

			//edge pieces
			c = cubelets[LE][slice][MID];
			cubelets[LE][slice][MID] = cubelets[MID][slice][FR];
			rotateCubelet(axis, direction, cubelets[LE][slice][MID]);

			cubelets[MID][slice][FR] = cubelets[RI][slice][MID];
			rotateCubelet(axis, direction, cubelets[MID][slice][FR]);

			cubelets[RI][slice][MID] = cubelets[MID][slice][BA];
			rotateCubelet(axis, direction, cubelets[RI][slice][MID]);

			cubelets[MID][slice][BA] = c;
			rotateCubelet(axis, direction, cubelets[MID][slice][BA]);
		}
		else if (direction == CLOCKWISE) {
			struct cubelet* c = cubelets[LE][slice][FR];
			cubelets[LE][slice][FR] = cubelets[LE][slice][BA];
			rotateCubelet(axis, direction, cubelets[LE][slice][FR]);

			cubelets[LE][slice][BA] = cubelets[RI][slice][BA];
			rotateCubelet(axis, direction, cubelets[LE][slice][BA]);

			cubelets[RI][slice][BA] = cubelets[RI][slice][FR];
			rotateCubelet(axis, direction, cubelets[RI][slice][BA]);

			cubelets[RI][slice][FR] = c;
			rotateCubelet(axis, direction, cubelets[RI][slice][FR]);

			c = cubelets[LE][slice][MID];
			cubelets[LE][slice][MID] = cubelets[MID][slice][BA];
			rotateCubelet(axis, direction, cubelets[LE][slice][MID]);

			cubelets[MID][slice][BA] = cubelets[RI][slice][MID];
			rotateCubelet(axis, direction, cubelets[MID][slice][BA]);

			cubelets[RI][slice][MID] = cubelets[MID][slice][FR];
			rotateCubelet(axis, direction, cubelets[RI][slice][MID]);

			cubelets[MID][slice][FR] = c;
			rotateCubelet(axis, direction, cubelets[MID][slice][FR]);

		}
		else {
			printf("should never happen\n");
		}
	}
	else if (axis == X_AXIS) {
		if (direction == C_CLOCKWISE) {
			//corner pieces
			struct cubelet* c = cubelets[slice][BO][FR];
			cubelets[slice][BO][FR] = cubelets[slice][BO][BA];
			rotateCubelet(axis, direction, cubelets[slice][BO][FR]);

			cubelets[slice][BO][BA] = cubelets[slice][TO][BA];
			rotateCubelet(axis, direction, cubelets[slice][BO][BA]);

			cubelets[slice][TO][BA] = cubelets[slice][TO][FR];
			rotateCubelet(axis, direction, cubelets[slice][TO][BA]);

			cubelets[slice][TO][FR] = c;
			rotateCubelet(axis, direction, cubelets[slice][TO][FR]);

			//edge pieces
			c = cubelets[slice][MID][FR];
			cubelets[slice][MID][FR] = cubelets[slice][BO][MID];
			rotateCubelet(axis, direction, cubelets[slice][MID][FR]);

			cubelets[slice][BO][MID] = cubelets[slice][MID][BA];
			rotateCubelet(axis, direction, cubelets[slice][BO][MID]);

			cubelets[slice][MID][BA] = cubelets[slice][TO][MID];
			rotateCubelet(axis, direction, cubelets[slice][MID][BA]);

			cubelets[slice][TO][MID] = c;
			rotateCubelet(axis, direction, cubelets[slice][TO][MID]);
		}
		else if (direction == CLOCKWISE) {
			struct cubelet* c = cubelets[slice][BO][FR];
			cubelets[slice][BO][FR] = cubelets[slice][TO][FR];
			rotateCubelet(axis, direction, cubelets[slice][BO][FR]);

			cubelets[slice][TO][FR] = cubelets[slice][TO][BA];
			rotateCubelet(axis, direction, cubelets[slice][TO][FR]);

			cubelets[slice][TO][BA] = cubelets[slice][BO][BA];
			rotateCubelet(axis, direction, cubelets[slice][TO][BA]);

			cubelets[slice][BO][BA] = c;
			rotateCubelet(axis, direction, cubelets[slice][BO][BA]);

			//edge pieces
			c = cubelets[slice][MID][FR];
			cubelets[slice][MID][FR] = cubelets[slice][TO][MID];
			rotateCubelet(axis, direction, cubelets[slice][MID][FR]);

			cubelets[slice][TO][MID] = cubelets[slice][MID][BA];
			rotateCubelet(axis, direction, cubelets[slice][TO][MID]);

			cubelets[slice][MID][BA] = cubelets[slice][BO][MID];
			rotateCubelet(axis, direction, cubelets[slice][MID][BA]);

			cubelets[slice][BO][MID] = c;
			rotateCubelet(axis, direction, cubelets[slice][BO][MID]);

		}
		else {
			printf("should never happen\n");
		}
	}
	else {
		printf("SHOULD NEVER HAPPEN\n");
	}

	updateHierarchy();
}

/** rotateCubelet()
 *
 * This function will "rotate" a cubelet by changing the
 * cubelet->faceColors[] array.  This is called whenever a slice
 * rotation is done (cubelets change position as well as orientation!)
 */
void rotateCubelet(int axis, int direction, struct cubelet* c) {
	if (axis == Z_AXIS) {
		int top = c->faceColors[TOP];
		if (direction == C_CLOCKWISE) {
			c->faceColors[TOP] = c->faceColors[LEFT];
			c->faceColors[LEFT] = c->faceColors[BOTTOM];
			c->faceColors[BOTTOM] = c->faceColors[RIGHT];
			c->faceColors[RIGHT] = top;
		}
		else if (direction == CLOCKWISE) {
			c->faceColors[TOP] = c->faceColors[RIGHT];
			c->faceColors[RIGHT] = c->faceColors[BOTTOM];
			c->faceColors[BOTTOM] = c->faceColors[LEFT];
			c->faceColors[LEFT] = top;
		}
		else {
			printf("should never happen!\n");
		}
	}
	else if (axis == Y_AXIS) {
		int front = c->faceColors[FRONT];
		if (direction == C_CLOCKWISE) {
			c->faceColors[FRONT] = c->faceColors[RIGHT];
			c->faceColors[RIGHT] = c->faceColors[BACK];
			c->faceColors[BACK] = c->faceColors[LEFT];
			c->faceColors[LEFT] = front;
		}
		else if (direction == CLOCKWISE) {
			c->faceColors[FRONT] = c->faceColors[LEFT];
			c->faceColors[LEFT] = c->faceColors[BACK];
			c->faceColors[BACK] = c->faceColors[RIGHT];
			c->faceColors[RIGHT] = front;
		}
		else {
			printf("should never happen!\n");
		}
	}
	else if (axis == X_AXIS) {
		int front = c->faceColors[FRONT];
		if (direction == C_CLOCKWISE) {
			c->faceColors[FRONT] = c->faceColors[BOTTOM];
			c->faceColors[BOTTOM] = c->faceColors[BACK];
			c->faceColors[BACK] = c->faceColors[TOP];
			c->faceColors[TOP] = front;
		}
		else if (direction == CLOCKWISE) {
			c->faceColors[FRONT] = c->faceColors[TOP];
			c->faceColors[TOP] = c->faceColors[BACK];
			c->faceColors[BACK] = c->faceColors[BOTTOM];
			c->faceColors[BOTTOM] = front;
		}
		else {
			printf("should never happen!\n");
		}
	}
	else {
		printf("should never happen\n");
	}
}

//****************************************************************************
//ANIMATION STUFF
//****************************************************************************

/** animateTurn()
 *
 * This function is the entry point into the animation functionality of this
 * program.  It will set the animate boolean to true and it will set the
 * global A_** variables so the idle() function will know which cubelet
 * is the "parent" and what direction/axis for rotation.
 */
void animateTurn(int axis, int direction, int slice) {
	animate = true;
	A_axis = axis;
	A_direction = direction;
	A_slice = slice;
	A_angle = 0;
	if (A_axis == X_AXIS)
		A_cubelet = cubelets[slice][MID][MID];
	else if (A_axis == Y_AXIS)
		A_cubelet = cubelets[MID][slice][MID];
	else if (A_axis == Z_AXIS)
		A_cubelet = cubelets[MID][MID][slice];
}
/** idle()
 *
 * This function is always being executed.  It will handle the slice
 * rotation animations.
 */
void idle() {
	if (animate == true) {
		//adjust the A_angle
		if (A_direction == CLOCKWISE)
			A_angle = A_angle + ANGLE_INCREMENT;
		else
			A_angle = A_angle - ANGLE_INCREMENT;

		//load the slice rotation matrix
		glPushMatrix();
		glLoadIdentity();
		if (A_axis == X_AXIS)
			glRotatef(A_angle, 1, 0, 0);
		if (A_axis == Y_AXIS)
			glRotatef(A_angle, 0, 1, 0);
		if (A_axis == Z_AXIS)
			glRotatef(A_angle, 0, 0, 1);
		glGetFloatv(GL_MODELVIEW_MATRIX, *rotate);
		glPopMatrix();

		//check here to see if rotation is finished
		if (A_angle >= 90 || A_angle <= -90) {
			animate = false;
			//do the actual "logical" rotation (update the cubelets[][][])
			rotateSlice(A_axis, A_direction, A_slice);
		}
	}
	else {
		animate = false;
	}
	glutPostRedisplay();
}
//****************************************************************************
//HELPER functions
//****************************************************************************
/** getCubeletLocation()
 *
 * This helper function will take in the ID of a cubelet and will return the
 * location of the cubelet within the cube.
 */
int getCubeletLocation(int ID) {
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++)
			for (int k = 0; k < 3; k++)
				if (cubelets[i][j][k]->ID == ID)
					return cubeletLocations[i][j][k];

	//should never happen
	return -1;
}


/** initCube()
 *
 * This method will allocate memory for the 27 cubelets and will store
 * references to these cubelets in the cubelets[][][] array.  It will
 * also initialize the cubeletLocations[][][] array
 */
void initCube() {
	struct cubelet* c;
	int id = 0;
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			for (int k = 0; k < 3; k++) {
				c = (struct cubelet*)malloc(sizeof(struct cubelet));
				c->ID = id;
				resetCubeletFaces(c);
				cubelets[k][j][i] = c;
				cubeletLocations[k][j][i] = id;
				id++;
			}
		}
	}


	setFaceColor(FRONT, RED);
	setFaceColor(BACK, ORANGE);
	setFaceColor(TOP, WHITE);
	setFaceColor(BOTTOM, BLUE);
	setFaceColor(LEFT, GREEN);
	setFaceColor(RIGHT, YELLOW);

	//update the child/parent references
	updateHierarchy();
}

/** resetCubeletFaces()
 *
 * This helper function will set all faces of a cubelet to grey.
 */
void resetCubeletFaces(struct cubelet* c) {
	for (int i = 0; i < 6; i++)
		c->faceColors[i] = GREY;
}
/** updateHierarchy()
 *
 * This function will update every cubelet's children[] references.  This is
 * called after every move since cubelets will have changed positions.
 */
void updateHierarchy() {
	//update front face references
	cubelets[MID][MID][FR]->children[Z_AXIS][0] = cubelets[LE][BO][FR];
	cubelets[MID][MID][FR]->children[Z_AXIS][1] = cubelets[LE][MID][FR];
	cubelets[MID][MID][FR]->children[Z_AXIS][2] = cubelets[LE][TO][FR];
	cubelets[MID][MID][FR]->children[Z_AXIS][3] = cubelets[MID][TO][FR];
	cubelets[MID][MID][FR]->children[Z_AXIS][4] = cubelets[RI][TO][FR];
	cubelets[MID][MID][FR]->children[Z_AXIS][5] = cubelets[RI][MID][FR];
	cubelets[MID][MID][FR]->children[Z_AXIS][6] = cubelets[RI][BO][FR];
	cubelets[MID][MID][FR]->children[Z_AXIS][7] = cubelets[MID][BO][FR];

	//update back face references
	cubelets[MID][MID][BA]->children[Z_AXIS][0] = cubelets[LE][BO][BA];
	cubelets[MID][MID][BA]->children[Z_AXIS][1] = cubelets[LE][MID][BA];
	cubelets[MID][MID][BA]->children[Z_AXIS][2] = cubelets[LE][TO][BA];
	cubelets[MID][MID][BA]->children[Z_AXIS][3] = cubelets[MID][TO][BA];
	cubelets[MID][MID][BA]->children[Z_AXIS][4] = cubelets[RI][TO][BA];
	cubelets[MID][MID][BA]->children[Z_AXIS][5] = cubelets[RI][MID][BA];
	cubelets[MID][MID][BA]->children[Z_AXIS][6] = cubelets[RI][BO][BA];
	cubelets[MID][MID][BA]->children[Z_AXIS][7] = cubelets[MID][BO][BA];

	//update top face references
	cubelets[MID][TO][MID]->children[Y_AXIS][0] = cubelets[LE][TO][FR];
	cubelets[MID][TO][MID]->children[Y_AXIS][1] = cubelets[LE][TO][MID];
	cubelets[MID][TO][MID]->children[Y_AXIS][2] = cubelets[LE][TO][BA];
	cubelets[MID][TO][MID]->children[Y_AXIS][3] = cubelets[MID][TO][BA];
	cubelets[MID][TO][MID]->children[Y_AXIS][4] = cubelets[RI][TO][BA];
	cubelets[MID][TO][MID]->children[Y_AXIS][5] = cubelets[RI][TO][MID];
	cubelets[MID][TO][MID]->children[Y_AXIS][6] = cubelets[RI][TO][FR];
	cubelets[MID][TO][MID]->children[Y_AXIS][7] = cubelets[MID][TO][FR];

	//update bottom face references
	cubelets[MID][BO][MID]->children[Y_AXIS][0] = cubelets[LE][BO][FR];
	cubelets[MID][BO][MID]->children[Y_AXIS][1] = cubelets[LE][BO][MID];
	cubelets[MID][BO][MID]->children[Y_AXIS][2] = cubelets[LE][BO][BA];
	cubelets[MID][BO][MID]->children[Y_AXIS][3] = cubelets[MID][BO][BA];
	cubelets[MID][BO][MID]->children[Y_AXIS][4] = cubelets[RI][BO][BA];
	cubelets[MID][BO][MID]->children[Y_AXIS][5] = cubelets[RI][BO][MID];
	cubelets[MID][BO][MID]->children[Y_AXIS][6] = cubelets[RI][BO][FR];
	cubelets[MID][BO][MID]->children[Y_AXIS][7] = cubelets[MID][BO][FR];

	//update right face references
	cubelets[RI][MID][MID]->children[X_AXIS][0] = cubelets[RI][BO][FR];
	cubelets[RI][MID][MID]->children[X_AXIS][1] = cubelets[RI][MID][FR];
	cubelets[RI][MID][MID]->children[X_AXIS][2] = cubelets[RI][TO][FR];
	cubelets[RI][MID][MID]->children[X_AXIS][3] = cubelets[RI][TO][MID];
	cubelets[RI][MID][MID]->children[X_AXIS][4] = cubelets[RI][TO][BA];
	cubelets[RI][MID][MID]->children[X_AXIS][5] = cubelets[RI][MID][BA];
	cubelets[RI][MID][MID]->children[X_AXIS][6] = cubelets[RI][BO][BA];
	cubelets[RI][MID][MID]->children[X_AXIS][7] = cubelets[RI][BO][MID];

	//update left face references
	cubelets[LE][MID][MID]->children[X_AXIS][0] = cubelets[LE][BO][FR];
	cubelets[LE][MID][MID]->children[X_AXIS][1] = cubelets[LE][MID][FR];
	cubelets[LE][MID][MID]->children[X_AXIS][2] = cubelets[LE][TO][FR];
	cubelets[LE][MID][MID]->children[X_AXIS][3] = cubelets[LE][TO][MID];
	cubelets[LE][MID][MID]->children[X_AXIS][4] = cubelets[LE][TO][BA];
	cubelets[LE][MID][MID]->children[X_AXIS][5] = cubelets[LE][MID][BA];
	cubelets[LE][MID][MID]->children[X_AXIS][6] = cubelets[LE][BO][BA];
	cubelets[LE][MID][MID]->children[X_AXIS][7] = cubelets[LE][BO][MID];

	//update middle slice references
	//rotations about X_AXIS
	cubelets[MID][MID][MID]->children[X_AXIS][0] = cubelets[MID][MID][FR];
	cubelets[MID][MID][MID]->children[X_AXIS][1] = cubelets[MID][BO][FR];
	cubelets[MID][MID][MID]->children[X_AXIS][2] = cubelets[MID][BO][MID];
	cubelets[MID][MID][MID]->children[X_AXIS][3] = cubelets[MID][BO][BA];
	cubelets[MID][MID][MID]->children[X_AXIS][4] = cubelets[MID][MID][BA];
	cubelets[MID][MID][MID]->children[X_AXIS][5] = cubelets[MID][TO][BA];
	cubelets[MID][MID][MID]->children[X_AXIS][6] = cubelets[MID][TO][MID];
	cubelets[MID][MID][MID]->children[X_AXIS][7] = cubelets[MID][TO][FR];

	//rotations about Y_AXIS
	cubelets[MID][MID][MID]->children[Y_AXIS][0] = cubelets[MID][MID][FR];
	cubelets[MID][MID][MID]->children[Y_AXIS][1] = cubelets[LE][MID][FR];
	cubelets[MID][MID][MID]->children[Y_AXIS][2] = cubelets[LE][MID][MID];
	cubelets[MID][MID][MID]->children[Y_AXIS][3] = cubelets[LE][MID][BA];
	cubelets[MID][MID][MID]->children[Y_AXIS][4] = cubelets[MID][MID][BA];
	cubelets[MID][MID][MID]->children[Y_AXIS][5] = cubelets[RI][MID][BA];
	cubelets[MID][MID][MID]->children[Y_AXIS][6] = cubelets[RI][MID][MID];
	cubelets[MID][MID][MID]->children[Y_AXIS][7] = cubelets[RI][MID][FR];

	//rotations about Z_AXIS
	cubelets[MID][MID][MID]->children[Z_AXIS][0] = cubelets[LE][MID][MID];
	cubelets[MID][MID][MID]->children[Z_AXIS][1] = cubelets[LE][TO][MID];
	cubelets[MID][MID][MID]->children[Z_AXIS][2] = cubelets[MID][TO][MID];
	cubelets[MID][MID][MID]->children[Z_AXIS][3] = cubelets[RI][TO][MID];
	cubelets[MID][MID][MID]->children[Z_AXIS][4] = cubelets[RI][MID][MID];
	cubelets[MID][MID][MID]->children[Z_AXIS][5] = cubelets[RI][BO][MID];
	cubelets[MID][MID][MID]->children[Z_AXIS][6] = cubelets[MID][BO][MID];
	cubelets[MID][MID][MID]->children[Z_AXIS][7] = cubelets[LE][BO][MID];

}


/** trackBall()
 *
 * This function handles the virtual trackball stuff.  Thanks to Dr. Zoe Wood
 * for providing this code.
 */
void trackBall() {
	float startx_i, starty_i, endx_i, endy_i;

	//first set to image coordinates
	startx_i = p2i_x(mouseX);
	starty_i = p2i_y(mouseY);
	endx_i = p2i_x(endX);
	endy_i = p2i_y(endY);

	//first project the start  to the unit sphere
	float r = 1.0 - startx_i * startx_i - starty_i * starty_i;
	if (r > 0) {
		v1->x = startx_i; v1->y = starty_i; v1->z = sqrt(r);
	}
	else { //point outside of ball
		float d = sqrt(startx_i * startx_i + starty_i * starty_i);
		//assert(d != 0);
		v1->x = startx_i / d; v1->y = starty_i / d; v1->z = 0.0;
	}

	//then project the end to the unit sphere
	r = 1.0 - endx_i * endx_i - endy_i * endy_i;
	if (r > 0) {
		v2->x = endx_i; v2->y = endy_i; v2->z = sqrt(r);
	}
	else { //point outside of ball
		float d = sqrt(endx_i * endx_i + endy_i * endy_i);
		//assert(d != 0);
		v2->x = endx_i / d; v2->y = endy_i / d; v2->z = 0.0;
	}

	//compute the axis of rotation
	axis = cross_product(v1, v2);
	//compute the angle of rotation
	angle_of_rotation = 180.0 / 3.14 * acos(dot(v1, v2));
	if (angle_of_rotation < -180.0 || angle_of_rotation > 180)
		angle_of_rotation = 0.0;

	mouseX = endX;
	mouseY = endY;
}
/** keyboard()
 *
 * This callback will be executed whenver a key is pressed
 */
void keyboard(unsigned char key, int x, int y)
{
	if (animate == false) {
		switch (key) {
		case 'q': case 'Q':
			exit(EXIT_SUCCESS);
			break;
		case 'a':
			printf("toggling axis\n");
			if (draw_axis == true) {
				draw_axis = false;
			}
			else {
				draw_axis = true;
			}
			glutPostRedisplay();
			break;
		case 'r':
			printf("RESET!\n");
			//load identity matrix into our transformation matrix
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glGetFloatv(GL_MODELVIEW_MATRIX, *M);
			initCube();
			glutPostRedisplay();
			break;
		}
	}
}


/** reshape()
 *
 * This callback method is executed whenver the window is resized
 */
void reshape(int w, int h) {
	GW = w;
	GH = h;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-(float)w / h, (float)w / h, -1, 1, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glViewport(0, 0, w, h);
}


//***************************************************************************
//UTILITY FUNCTIONS
//***************************************************************************
/** dot()
 *
 * This method calculates the dot product of two vectors
 */
float dot(struct vector* a, struct vector* b) {
	return a->x * b->x + a->y * b->y + a->z * b->z;
}

/** cross_product
 *
 * This method calculates the cross product of two vectors
 */
struct vector* cross_product(struct vector* a, struct vector* b) {
	struct vector* v = (struct vector*)malloc(sizeof(struct vector));

	v->posx = a->posx;
	v->posy = a->posy;
	v->posz = a->posz;

	v->x = a->y * b->z - a->z * b->y;
	v->y = a->z * b->x - a->x * b->z;
	v->z = a->x * b->y - a->y * b->x;

	return v;
}

/** p2w_x()
 *
 * Pixel->world mapping
 */
float p2w_x(int p_x) {
	float x_i = ((float)p_x - ((GW - 1.0) / 2.0)) * 2.0 / GW;
	return(((float)GW / (float)GH) * x_i);
}

/** p2w_y()
 *
 * Pixel->world mapping
 */
float p2w_y(int p_y) {
	return(((float)p_y - ((GH - 1.0) / 2.0)) * 2.0 / GH);
}

/** p2i_x()
 *
 * Pixel->image mapping
 */
float p2i_x(int p_x) {
	float x_i = ((float)p_x - ((GW - 1.0) / 2.0)) * 2.0 / GW;
	return(x_i);
}

/** p2i_y()
 *
 * Pixel->image mapping
 */
float p2i_y(int p_y) {
	return(((float)p_y - ((GH - 1.0) / 2.0)) * 2.0 / GH);
}
/** setColor()
 *
 * This helper method will call glColor3f with the appropriate values.
 * It takes a single color argument (RED, GREEN, etc)
 */
void setColor(int color) {
	switch (color) {
	case RED:
		//printf("red");
		glColor3f(1, 0, 0);
		break;
	case GREEN:
		glColor3f(0, 1, 0);
		break;
	case BLUE:
		glColor3f(0, 0, 1);
		break;
	case WHITE:
		glColor3f(1, 1, 1);
		break;
	case YELLOW:
		glColor3f(1, 1, 0);
		break;
	case ORANGE:
		glColor3f(1, .5, 0);
		break;
	case BLACK:
		glColor3f(0, 0, 0);
		break;
	case GREY:
		glColor3f(.7, .7, .7);
		break;
	}
}
/** initialize_vectors()
 *
 * This helper function is used to initialize our rotation vectors
 */
void initialize_vectors() {
	//free up our previously malloc'd vectors
	free(v1);
	free(v2);
	free(axis);
	//malloc new vectors
	v1 = (struct vector*)malloc(sizeof(struct vector));
	v2 = (struct vector*)malloc(sizeof(struct vector));
	axis = (struct vector*)malloc(sizeof(struct vector));

	//set vectors with default values
	v1->posx = 0.0;
	v1->posy = 0.0;
	v1->posz = 0.0;
	v2->posx = 0.0;
	v2->posy = 0.0;
	v2->posz = 0.0;
	axis->posx = 0.0;
	axis->posy = 0.0;
	axis->posz = 0.0;
	v1->x = 0.0;
	v1->y = 0.0;
	v1->z = 0.0;
	v2->x = 0.0;
	v2->y = 0.0;
	v2->z = 0.0;
	axis->x = 0.0;
	axis->y = 0.0;
	axis->z = 0.0;
}
/** setFaceColor
 *
 * This helper function will set a single face of the cube to a single
 * color.  It does this by iterating through all cubelets on the
 * specified face, and setting the cubelet->faceColors[] array
 * appropriately.
 */
void setFaceColor(int face, int color) {
	int i, j;
	switch (face) {
	case FRONT: //front
		for (i = 0; i < 3; i++)
			for (j = 0; j < 3; j++)
				cubelets[i][j][FR]->faceColors[face] = color;
		break;
	case BACK: //back
		for (i = 0; i < 3; i++)
			for (j = 0; j < 3; j++)
				cubelets[i][j][BA]->faceColors[face] = color;
		break;
	case TOP: //top
		for (i = 0; i < 3; i++)
			for (j = 0; j < 3; j++)
				cubelets[i][TO][j]->faceColors[face] = color;
		break;
	case BOTTOM: //bottom
		for (i = 0; i < 3; i++)
			for (j = 0; j < 3; j++)
				cubelets[i][BO][j]->faceColors[face] = color;
		break;
	case RIGHT: //right
		for (i = 0; i < 3; i++)
			for (j = 0; j < 3; j++)
				cubelets[RI][i][j]->faceColors[face] = color;
		break;
	case LEFT: //left
		for (i = 0; i < 3; i++)
			for (j = 0; j < 3; j++)
				cubelets[LE][i][j]->faceColors[face] = color;
		break;
	}

}


/** initCenterPositions()
 *
 * This helper method will initialize the centerPositions[][][] array
 * with vertices that represent cubelet center positions.
 */
void initCenterPositions() {
	struct vertex* v;

	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++)
			for (int k = 0; k < 3; k++) {
				v = (struct vertex*)malloc(sizeof(struct vertex));
				v->x = -C_W + (i * C_W);
				v->y = -C_W + (j * C_W);
				v->z = C_W + (k * -C_W);
				centerPositions[i][j][k] = v;
			}
}

//*******************************************************************************
//*******************************************************************************
//MAIN FUNCTION
//*******************************************************************************
//*******************************************************************************
/** main()
 */
int main(int argc, char** argv) {

	glutInit(&argc, argv);
	glutInitWindowSize(300, 300);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutCreateWindow("Rubik's Cube");

	glutIdleFunc(idle);
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutMouseFunc(mouse);
	glutMotionFunc(mouseMove);
	glutKeyboardFunc(keyboard);
	glClearColor(0.0, 0.0, 0.0, 0.0);

	//set up the viewing
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
	glViewport(0, 0, 300, 300); //image to pixel

	//initialize globals
	GW = 300;
	GH = 300;

	initialize_vectors();

	//initialize the array that holds the center vertices for all cubelets
	//in the cube
	initCenterPositions();

	//initialize the cube
	initCube();
	glutMainLoop();
}