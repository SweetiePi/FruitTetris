/*
CMPT 361 Assignment 1 - FruitTetris implementation Sample Skeleton Code

- This is ONLY a skeleton code showing:
How to use multiple buffers to store different objects
An efficient scheme to represent the grids and blocks

- Compile and Run:
Type make in terminal, then type ./FruitTetris

This code is extracted from Connor MacLeod's (crmacleo@sfu.ca) assignment submission
by Rui Ma (ruim@sfu.ca) on 2014-03-04. 

Modified in Sep 2014 by Honghua Li (honghual@sfu.ca).
*/

/* Alex Sweeten
 * 301265554
 */

#include "include/Angel.h"
#include <cstdlib>
#include <iostream>
#include <set>

using namespace std;


// xsize and ysize represent the window size - updated if window is reshaped to prevent stretching of the game
int xsize = 400; 
int ysize = 720;
int tileX[4]; //stores the location of each piece of the tile, used for collision detection
int tileY[4];
int speed = 400;

// current tile
vec2 tile[4]; // An array of 4 2d vectors representing displacement from a 'center' piece of the tile, on the grid
vec2 tilepos = vec2(5, 19); // The position of the current tile using grid coordinates ((0,0) is the bottom left corner)

// An array storing all possible orientations of all possible tiles
// The 'tile' array will always be some element [i][j] of this array (an array of vec2)

//Both orientations of L-shapes
vec2 allRotationsLshapeLeft[4][4] = 
	{{vec2(0, 0), vec2(-1,0), vec2(1, 0), vec2(-1,-1)},
	{vec2(0, 1), vec2(0, 0), vec2(0,-1), vec2(1, -1)},     
	{vec2(1, 1), vec2(-1,0), vec2(0, 0), vec2(1,  0)},  
	{vec2(-1,1), vec2(0, 1), vec2(0, 0), vec2(0, -1)}};

vec2 allRotationsLshapeRight[4][4] = 
	{{vec2(1, -1), vec2(-1,0), vec2(0, 0), vec2(1,  0)},
	{vec2(0, 1), vec2(0, 0), vec2(0,-1), vec2(-1, -1)},     
	{vec2(0, 0), vec2(-1,0), vec2(1, 0), vec2(-1,1)},  
	{vec2(1,1), vec2(0, 1), vec2(0, 0), vec2(0, -1)}};
//I-shape. Certain vector spaces are repeated to make calling newtile() more manageable.
vec2 allRotationsIshape[4][4] = 
 	{{vec2(0, 0), vec2(1,0), vec2(-1, 0), vec2(-2,0)},
 	{vec2(0, 1), vec2(0,0), vec2(0, -1), vec2(0,-2)},
 	{vec2(0, 0), vec2(1,0), vec2(-1, 0), vec2(-2,0)},
 	{vec2(0, 1), vec2(0,0), vec2(0, -1), vec2(0,-2)}};

//T-shape
vec2 allRotationsTshape[4][4] = 
 	{{vec2(0, 0), vec2(1,0), vec2(-1, 0), vec2(0, -1)},
 	{vec2(0, 0), vec2(1,0), vec2(0, 1), vec2(0, -1)},
 	{vec2(0, 0), vec2(1,0), vec2(-1, 0), vec2(0, 1)},
 	{vec2(0, 0), vec2(0,1), vec2(-1, 0), vec2(0, -1)}};

//Both orientations of S-shapes. Certain vector spaces are repeated to make calling newtile() more manageable.
vec2 allRotationsSshapeLeft[4][4] =
	{{vec2(0, 0), vec2(0,-1), vec2(-1, -1), vec2(1,0)},
 	{vec2(0, 1), vec2(0,0), vec2(1, 0), vec2(1,-1)},
 	{vec2(0, 0), vec2(0,-1), vec2(-1, -1), vec2(1,0)},
 	{vec2(0, 1), vec2(0,0), vec2(1, 0), vec2(1,-1)}};

vec2 allRotationsSshapeRight[4][4] =
	{{vec2(0, 0), vec2(0,-1), vec2(1, -1), vec2(-1,0)},
 	{vec2(0, 1), vec2(0,0), vec2(-1, 0), vec2(-1,-1)},
 	{vec2(0, 0), vec2(0,-1), vec2(1, -1), vec2(-1,0)},
 	{vec2(0, 1), vec2(0,0), vec2(-1, 0), vec2(-1,-1)}};

// colors. x values are slighlty modified to simplify colour detection (no visual difference)
vec4 white  = vec4(1.0, 1.0, 1.0, 1.0);
vec4 black  = vec4(0.0, 0.0, 0.0, 1.0);
vec4 purple = vec4(0.8, 0.0, 1.0, 1.0);
vec4 red 	= vec4(0.99, 0.0, 0.0, 1.0);
vec4 yellow = vec4(0.97, 1.0, 0.0, 1.0);
vec4 green 	= vec4(0.1, 1.0, 0.0, 1.0);
vec4 orange = vec4(0.98, 0.5, 0.0, 1.0); 

vec4 colorList[5] = {purple, red, yellow, green, orange};
vec4 colors[4];//= {black, black, black, black};

//board[x][y] represents whether the cell (x,y) is occupied
bool board[10][20]; 

//An array containing the colour of each of the 10*20*2*3 vertices that make up the board
//Initially, all will be set to black. As tiles are placed, sets of 6 vertices (2 triangles; 1 square)
//will be set to the appropriate colour in this array before updating the corresponding VBO
vec4 boardcolours[1200];

// location of vertex attributes in the shader program
GLuint vPosition;
GLuint vColor;

// locations of uniform variables in shader program
GLuint locxsize;
GLuint locysize;

// VAO and VBO
GLuint vaoIDs[3]; // One VAO for each object: the grid, the board, the current piece
GLuint vboIDs[6]; // Two Vertex Buffer Objects for each VAO (specifying vertex positions and colours, respectively)

//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------

// When the current tile is moved or rotated (or created), update the VBO containing its vertex position data
void updatetile()
{
	// Bind the VBO containing current tile vertex positions
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[4]); 

	// For each of the 4 'cells' of the tile,
	for (int i = 0; i < 4; i++) 
	{
		// Calculate the grid coordinates of the cell
		GLfloat x = tilepos.x + tile[i].x; 
		GLfloat y = tilepos.y + tile[i].y;

		tileX[i] = (int)x;
		tileY[i] = (int)y;

		// Create the 4 corners of the square - these vertices are using location in pixels
		// These vertices are later converted by the vertex shader
		vec4 p1 = vec4(33.0 + (x * 33.0), 33.0 + (y * 33.0), .4, 1); 
		vec4 p2 = vec4(33.0 + (x * 33.0), 66.0 + (y * 33.0), .4, 1);
		vec4 p3 = vec4(66.0 + (x * 33.0), 33.0 + (y * 33.0), .4, 1);
		vec4 p4 = vec4(66.0 + (x * 33.0), 66.0 + (y * 33.0), .4, 1);

		// Two points are used by two triangles each
		vec4 newpoints[6] = {p1, p2, p3, p2, p3, p4}; 

		// Put new data in the VBO
		glBufferSubData(GL_ARRAY_BUFFER, i*6*sizeof(vec4), 6*sizeof(vec4), newpoints); 
	}

	glBindVertexArray(0);
}

//-------------------------------------------------------------------------------------------------------------------

// Called at the start of play and every time a tile is placed
void newtile()
{
	int RandTile	= rand() % 8; //Randomly selects an integer between 0 and 4
	int RandRotation= rand() % 3; //You get the idea...
	int RandStartX	= rand() % 6 + 2;

	//Specifying conditions for which starting tile doesn't have to be shifted down
	if (RandRotation == 0 || (RandRotation == 2 && RandTile > 3)){ 
	tilepos = vec2(RandStartX , 19); 
	}
	else{
		tilepos = vec2(RandStartX , 18);
	}
	// Update the geometry VBO of current tile
	for (int i = 0; i < 4; i++){
		// Determines a random shape. T and I are called twice to keep the probability of each shape 25%.
		switch (RandTile){ 
			case 0: tile[i] = allRotationsTshape[RandRotation][i];
				break;
			case 1: tile[i] = allRotationsTshape[RandRotation][i];
				break;
			case 2: tile[i] = allRotationsLshapeLeft[RandRotation][i];
				break;
			case 3: tile[i] = allRotationsLshapeRight[RandRotation][i];
				break;
			case 4: tile[i] = allRotationsSshapeLeft[RandRotation][i];
				break;
			case 5: tile[i] = allRotationsSshapeRight[RandRotation][i];
				break;
			case 6: tile[i] = allRotationsIshape[RandRotation][i];
				break;
			case 7: tile[i] = allRotationsIshape[RandRotation][i];
				break;
		}
	}

	updatetile(); 

	vec4 newcolours[24];
		vec4 randColor; 
		//assign every 6 verticies with the same randomly generated color
		for (int i = 0; i < 4; i++)
		{
			randColor = colorList[rand() % 5];
			for (int j = 0; j < 6; j++)
			{
				newcolours[(i * 6) + j] = randColor;
							
			}
			colors[i] = randColor;
		}
		glBindBuffer(GL_ARRAY_BUFFER, vboIDs[5]); // Bind the VBO containing current tile vertex colours
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(newcolours), newcolours); // Put the colour data in the VBO
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	
		glBindVertexArray(0);
	}

//-------------------------------------------------------------------------------------------------------------------

void initGrid()
{
	// ***Generate geometry data
	vec4 gridpoints[64]; // Array containing the 64 points of the 32 total lines to be later put in the VBO
	vec4 gridcolours[64]; // One colour per vertex
	// Vertical lines 
	for (int i = 0; i < 11; i++){
		gridpoints[2*i] = vec4((33.0 + (33.0 * i)), 33.0, 0, 1);
		gridpoints[2*i + 1] = vec4((33.0 + (33.0 * i)), 693.0, 0, 1);
		
	}
	// Horizontal lines
	for (int i = 0; i < 21; i++){
		gridpoints[22 + 2*i] = vec4(33.0, (33.0 + (33.0 * i)), 0, 1);
		gridpoints[22 + 2*i + 1] = vec4(363.0, (33.0 + (33.0 * i)), 0, 1);
	}
	// Make all grid lines white
	for (int i = 0; i < 64; i++)
		gridcolours[i] = white;


	// *** set up buffer objects
	// Set up first VAO (representing grid lines)
	glBindVertexArray(vaoIDs[0]); // Bind the first VAO
	glGenBuffers(2, vboIDs); // Create two Vertex Buffer Objects for this VAO (positions, colours)

	// Grid vertex positions
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[0]); // Bind the first grid VBO (vertex positions)
	glBufferData(GL_ARRAY_BUFFER, 64*sizeof(vec4), gridpoints, GL_STATIC_DRAW); // Put the grid points in the VBO
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, 0); 
	glEnableVertexAttribArray(vPosition); // Enable the attribute
	
	// Grid vertex colours
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[1]); // Bind the second grid VBO (vertex colours)
	glBufferData(GL_ARRAY_BUFFER, 64*sizeof(vec4), gridcolours, GL_STATIC_DRAW); // Put the grid colours in the VBO
	glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vColor); // Enable the attribute
}


void initBoard()
{
	// *** Generate the geometric data
	vec4 boardpoints[1200];
	for (int i = 0; i < 1200; i++)
		boardcolours[i] = black; // Let the empty cells on the board be black
	// Each cell is a square (2 triangles with 6 vertices)
	for (int i = 0; i < 20; i++){
		for (int j = 0; j < 10; j++)
		{		
			vec4 p1 = vec4(33.0 + (j * 33.0), 33.0 + (i * 33.0), .5, 1);
			vec4 p2 = vec4(33.0 + (j * 33.0), 66.0 + (i * 33.0), .5, 1);
			vec4 p3 = vec4(66.0 + (j * 33.0), 33.0 + (i * 33.0), .5, 1);
			vec4 p4 = vec4(66.0 + (j * 33.0), 66.0 + (i * 33.0), .5, 1);
			
			// Two points are reused
			boardpoints[6*(10*i + j)    ] = p1;
			boardpoints[6*(10*i + j) + 1] = p2;
			boardpoints[6*(10*i + j) + 2] = p3;
			boardpoints[6*(10*i + j) + 3] = p2;
			boardpoints[6*(10*i + j) + 4] = p3;
			boardpoints[6*(10*i + j) + 5] = p4;
		}
	}

	// Initially no cell is occupied
	for (int i = 0; i < 10; i++)
		for (int j = 0; j < 20; j++)
			board[i][j] = false; 


	// *** set up buffer objects
	glBindVertexArray(vaoIDs[1]);
	glGenBuffers(2, &vboIDs[2]);

	// Grid cell vertex positions
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[2]);
	glBufferData(GL_ARRAY_BUFFER, 1200*sizeof(vec4), boardpoints, GL_STATIC_DRAW);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vPosition);

	// Grid cell vertex colours
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[3]);
	glBufferData(GL_ARRAY_BUFFER, 1200*sizeof(vec4), boardcolours, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vColor);
}

// No geometry for current tile initially
void initCurrentTile()
{
	glBindVertexArray(vaoIDs[2]);
	glGenBuffers(2, &vboIDs[4]);

	// Current tile vertex positions
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[4]);
	glBufferData(GL_ARRAY_BUFFER, 24*sizeof(vec4), NULL, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vPosition);

	// Current tile vertex colours
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[5]);
	glBufferData(GL_ARRAY_BUFFER, 24*sizeof(vec4), NULL, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vColor);
}

void init()
{
	// Load shaders and use the shader program
	GLuint program = InitShader("vshader.glsl", "fshader.glsl");
	glUseProgram(program);

	// Get the location of the attributes (for glVertexAttribPointer() calls)
	vPosition = glGetAttribLocation(program, "vPosition");
	vColor = glGetAttribLocation(program, "vColor");

	// Create 3 Vertex Array Objects, each representing one 'object'. Store the names in array vaoIDs
	glGenVertexArrays(3, &vaoIDs[0]);

	// Initialize the grid, the board, and the current tile
	initGrid();
	initBoard();
	initCurrentTile();

	// The location of the uniform variables in the shader program
	locxsize = glGetUniformLocation(program, "xsize"); 
	locysize = glGetUniformLocation(program, "ysize");

	// Game initialization
	newtile(); // create new next tile

	// set to default
	glBindVertexArray(0);
	glClearColor(0, 0, 0, 0);
}

//-------------------------------------------------------------------------------------------------------------------

// Rotates the current tile, if there is room
void rotate()
{
	//θ = 90 degrees
	//the following is the formula to rotate counterclockwise about 0,0      
	//x’ = xcosθ - ysinθ
	//y’ = xsinθ + ycosθ
	//reduced to
	//x' = -y
	//y' =  x

	bool rotatable = true;
	vec2 rotateTile[4];


	for (int i = 0; i < 4; i++)
	{
 		GLfloat rotX = -1 * tile[i].y;
 		GLfloat rotY = tile[i].x;
 		GLfloat newPosX = tilepos.x + rotX;
 		GLfloat newPosY = tilepos.y + rotY;
		
		if (newPosX < 0 || newPosX > 9|| newPosY < 0 || board[(int)newPosX][(int)newPosY] == true) 
		//check if rotation puts us out of bounds or collides with a set tile
		{
			rotatable = false;
			break;
		}
		else
		{
			rotateTile[i] = vec2(rotX,rotY);
		}

	}

	if (rotatable)
	{
		for (int i = 0; i < 4; i++)
		{
			tile[i] = rotateTile[i];
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------

// Checks if the specified row (0 is the bottom 19 the top) is full
// If every cell in the row is occupied, it will clear that cell and everything above it will shift down one row
void checkfullrow(int row)
{
	bool occupied = true;
	for (int i = 0; i < 10; i++)
	{
		occupied = occupied && board[i][row];
	}
	if (occupied)
	{
		glBindBuffer(GL_ARRAY_BUFFER, vboIDs[3]);
		//loop begins at the row which is cleared
		for (int i = row; i < 20; i++)
		{
			for (int j = 0; j < 60; j++)
			{
				//top row becomes black, all other rows take on the colors of the row above
				if (i == 19)
				{
					boardcolours[1140 + j] = black;
				}
				else
				{
					boardcolours[(i * 60) + j] = boardcolours[(i * 60) + j + 60]; 
				}
			}
			for (int j = 0; j < 10; j++)
			{
				//top row becomes unoccupied all other rows take on the status of the row above
				if (i == 19)
				{
					board[j][19] = false;
				}
				else
				{
					board[j][i] = board[j][i+1];
				}
			}
		}
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(boardcolours), boardcolours);
		glBindVertexArray(0);
	}

}


void testmatch()
{
	int i = 0;
	for (i; i < 200; i++)
	{	
		int colcount = (i % 10);
		int rowcount = (i/10);
		//exclude black colours
		if(boardcolours[6*i].x != 0)
		{	
			//Test for 3 in a row. Exclude columns 8 & 9.
			if((boardcolours[6*i].x == boardcolours[6*(i+1)].x) && (boardcolours[6*i].x == boardcolours[6*(i+2)].x) && (i % 10 != 8) && (i % 10 != 9))
			{
					//Removes the 3 tiles in a row
					glBindBuffer(GL_ARRAY_BUFFER, vboIDs[3]);
				//loop begins at the row which is cleared
				for (int j = rowcount; j < 20; j++)
				{
					for (int h = 0; h < 18; h++)
					{
						//top row becomes black, all other rows take on the colors of the row above
						if (j == 19)
						{
							boardcolours[1140 + (colcount + h)] = black;
						}
						else
						{
							boardcolours[(6*i) + h] = boardcolours[(6*i) + h + 60]; 
						}
					}
					for (int e = colcount; e < (colcount+3); e++)
					{
						//top row becomes unoccupied all other rows take on the status of the row above
						if (j == 19)
						{
							board[e][19] = false;
						}
						else
						{
							board[e][j] = board[e][j+1];
						}
					}
				}
				glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(boardcolours), boardcolours);
				glBindVertexArray(0);
			}

			//Test for 3 in a column. Exclude rows 18 & 19.
			if((boardcolours[6*i].x == boardcolours[(6*i)+60].x) && (boardcolours[6*i].x == boardcolours[(6*i)+120].x) && (i < 1080))
			{
				
			}
			//Test 3 diagonally ascending to the left. Exclude rows columns 1 & 2, and rows 18 & 19.
			if((boardcolours[6*i].x == boardcolours[(6*i)+66].x) && (boardcolours[6*i].x == boardcolours[(6*i)+132].x) && (i < 1080) && (i % 10 != 0) && (i % 10 != 1))
			{
				
			}
			//Test 3 diagonally ascending to the right. Exclude rows columns 8 & 9, and rows 18 & 19.
			if((boardcolours[6*i].x == boardcolours[(6*i)+54].x) && (boardcolours[6*i].x == boardcolours[(6*i)+108].x) && (i < 1080) && (i % 10 != 8) && (i % 10 != 9))
			{
				
			}
		}
	}
}

//Checking and deleting if 3 colours in a row are detected
/*
void checkrowmatch()
{
	vec4 rowcolours[10];
	int rowcount = 0;
	//for (int y=0; y < 20; y++)
	//{
		for (int i = 0; i < 10; i++)
		{
			rowcolours[i] = boardcolours[((6*i)+1)];//+(y*60)];
		}

		for (int j = 0; j < 8; j++)
		{	//Checks to make sure colour value is not black
			if(rowcolours[j].x != 0)
			{	//Checks if there are 3 unique x value in a row
				if((rowcolours[j].x == rowcolours[j+1].x) && (rowcolours[j].x == rowcolours[j+2].x))
				{	//Removes the 3 tiles in a row
					glBindBuffer(GL_ARRAY_BUFFER, vboIDs[3]);
			//loop begins at the row which is cleared
			for (int i = 0; i < 20; i++)
			{
				for (int h = (rowcount*6); h < ((rowcount*6) + 18); h++)
				{
					//top row becomes black, all other rows take on the colors of the row above
					if (i == 19)
					{
						boardcolours[1140 + h] = black;
					}
					else
					{
						boardcolours[(i * 60) + h] = boardcolours[(i * 60) + h + 60]; 
					}
				}
				for (int e = rowcount; e < (rowcount+3); e++)
				{
					//top row becomes unoccupied all other rows take on the status of the row above
					if (i == 19)
					{
						board[e][19] = false;
					}
					else
					{
						board[e][i] = board[e][i+1];
					}
				}
			}
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(boardcolours), boardcolours);
			glBindVertexArray(0);
				}
			}
			rowcount++;
		}
	//}	
}
*/
//-------------------------------------------------------------------------------------------------------------------

// Places the current tile - update the board vertex colour VBO and the array maintaining occupied cells
void settile()
{
{
	//update VBO color
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[3]);
	std::set<int, std::greater<int> > rowsAffected;
	std::set<int>::iterator it;
	for (int i = 0; i < 4; i++)
	{
		//calculate grid coorindates for each cell	
		GLuint x = tilepos.x + tile[i].x; 
		GLuint y = tilepos.y + tile[i].y;

		//update the corresponding cell to get the same color as the tile
		for (int j = 0; j < 6; j++)
		{
			boardcolours[(y*60)+(x*6) + j] = colors[i];
		}
		
		board[x][y] = true;

		rowsAffected.insert(y);
	}
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(boardcolours), boardcolours);
	glBindVertexArray(0);


	for (it = rowsAffected.begin(); it != rowsAffected.end(); ++it)
	{
		checkfullrow(*it);
	}	
	testmatch();
}
}

//-------------------------------------------------------------------------------------------------------------------

// Given (x,y), tries to move the tile x squares to the right and y squares down
// Returns true if the tile was successfully moved, or false if there was some issue
bool movetile(vec2 direction)
{
	
	return false;
}
//-------------------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------------------

void fallingtile(int data)
{
	//halt falling bricks if starting point is obstructed
	if (!board[5][19] && !board[5][18] && !board[4][19] && !board[4][18] && !board[6][19] && !board[6][18] )
	{
		tilepos.y -= 1;
		updatetile();

		glutPostRedisplay();
		glutTimerFunc(speed, fallingtile, 0);
	}
}

// Starts the game over - empties the board, creates new tiles, resets line counters
void restart()
{
	initBoard();
	//initCurrentTile();
	newtile();
	glutTimerFunc(speed, fallingtile, 0);
	speed = 400;
}

// Draws the game
void display()
{
	glClear(GL_COLOR_BUFFER_BIT);

	glUniform1i(locxsize, xsize); // x and y sizes are passed to the shader program to maintain shape of the vertices on screen
	glUniform1i(locysize, ysize);

	glBindVertexArray(vaoIDs[1]); // Bind the VAO representing the grid cells (to be drawn first)
	glDrawArrays(GL_TRIANGLES, 0, 1200); // Draw the board (10*20*2 = 400 triangles)

	glBindVertexArray(vaoIDs[2]); // Bind the VAO representing the current tile (to be drawn on top of the board)
	glDrawArrays(GL_TRIANGLES, 0, 24); // Draw the current tile (8 triangles)

	glBindVertexArray(vaoIDs[0]); // Bind the VAO representing the grid lines (to be drawn on top of everything else)
	glDrawArrays(GL_LINES, 0, 64); // Draw the grid lines (21+11 = 32 lines)


	glutSwapBuffers();
}

//-------------------------------------------------------------------------------------------------------------------

// Reshape callback will simply change xsize and ysize variables, which are passed to the vertex shader
// to keep the game the same from stretching if the window is stretched
void reshape(GLsizei w, GLsizei h)
{
	xsize = w;
	ysize = h;
	glViewport(0, 0, w, h);
}

//-------------------------------------------------------------------------------------------------------------------

// Handle arrow key keypresses
void special(int key, int x, int y)
{
	bool movable = true;
		
		switch(key) {
		case GLUT_KEY_UP :
			rotate();
			break;
		case GLUT_KEY_DOWN :
			for (int i = 0; i < 4; i++)
			{
				if (board[tileX[i]][tileY[i]-1] == true)
				{
					movable = false;
					break;
				}			
			}
			if (movable)
			{
				tilepos.y -= 1;
			}
			break;
		case GLUT_KEY_LEFT :
			for (int i = 0; i < 4; i++)
			{
				if (tileX[i] == 0  || board[tileX[i]-1][tileY[i]] == true)
				{
					movable = false;
					break;
				}			
			}
			if (movable)
			{
				tilepos.x -= 1;
			}			
			break;
		case GLUT_KEY_RIGHT:
			for (int i = 0; i < 4; i++)
			{
				if (tileX[i] == 9 || board[tileX[i]+1][tileY[i]] == true)
				{
					movable = false;
					break;
				}		
			}
			if (movable)
			{
				tilepos.x += 1;
			}	
			
			break;
		}
		updatetile();

		glutPostRedisplay();
}

//-------------------------------------------------------------------------------------------------------------------

// Handles standard keypresses
void keyboard(unsigned char key, int x, int y)
{
	switch(key) 
	{
		case 033: // Both escape key and 'q' cause the game to exit
		    exit(EXIT_SUCCESS);
		    break;
		case 'q':
			exit (EXIT_SUCCESS);
			break;
		case 'r': // 'r' key restarts the game
			restart();
			break;
		case 'a':
			if(speed > 100)
			{
			speed -= 100;
			}
			break;
		case 'd':
			if(speed < 1000)
			{
			speed += 100;
			}
			break;
	glutPostRedisplay();
	}
}

//-------------------------------------------------------------------------------------------------------------------

void idle(void)
{
	//loop through each piece of the tile to check for collision with the floor or a set tile
	for (int i = 0; i < 4; i++) 
	{
		//if the tile reaches the floor or the cell directly below is occupied		
		if ((tileY[i]) == 0 || board[tileX[i]][tileY[i]-1] == true) 

		{
			settile();
			if (board[5][19] == true){
				break;
			}
			newtile();
			break;
		}
	}
	glutPostRedisplay();
}
//-------------------------------------------------------------------------------------------------------------------

int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
	glutInitWindowSize(xsize, ysize);
	glutInitWindowPosition(680, 178); // Center the game window (well, on a 1920x1080 display)
	glutCreateWindow("Fruit Tetris");
	glewInit();
	init();

	// Callback functions
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutSpecialFunc(special);
	glutKeyboardFunc(keyboard);
	glutIdleFunc(idle);
	glutTimerFunc(speed, fallingtile, 0);

	glutMainLoop(); // Start main loop
	return 0;
}
