// Ball.cpp
#include "Ball.h"
#include "constants.h"
#include <cmath>
#include <GL/gl.h>
#include <iostream>

using namespace constants;

Ball::Ball() { real = false; }

Ball::Ball(float* lPos) {
    
    real = true;
    
    angle = rand() * 2 * PI;
    angularVel = 0;
    
    position = new float[2];
    
    position[0] = (stringLength * cos(angle)) + lPos[0];
    position[1] = (stringLength * sin(angle)) + lPos[1];
    
    color = new float[3];
    color[0] = ((float) (rand() % 255)) / 255; color[1] = ((float) (rand() % 255)) / 255; color[2] = ((float) (rand() % 255)) / 255;
}


void Ball::draw(){
	int i;
	int nSegment = 50; //# of triangles used to draw circle
    
    
    glColor3f(color[0], color[1], color[2]);
	
	GLfloat twicePi = 2.0f * PI;
	
	glBegin(GL_TRIANGLE_FAN);
		glVertex2f(position[0], position[1]); // center of circle
		for(i = 0; i <= nSegment;i++) { 
			glVertex2f(
                position[0] + (drawCircleSize * cos(i *  twicePi / nSegment)), 
			    position[1] + (drawCircleSize * sin(i * twicePi / nSegment))
			);
		}
	glEnd();
}

void Ball::tick(float dT, float angularAccel, float* abovePos) {
        
    std::cout << "angle=" << angle << std::endl;

    angularVel = angularVel + (angularAccel * dT);
    angle = angle + (angularVel * dT);
    
    position[0] = abovePos[0] + (drawStringLength * cos(angle));
    position[1] = abovePos[1] + (drawStringLength * sin(angle));
}