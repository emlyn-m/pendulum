import os
os.environ["PYGAME_HIDE_SUPPORT_PROMPT"] = "1"
import pygame
from pygame import gfxdraw
import sys
import math
import random

class Matrix:
    def __init__(self, values):
        self.values = values


    def __index__(self, i): return self.values[i]

    def decomp(self):
        # Perform LU decomposition via the Doolittle Algorithm

        lower = [[0 for _ in range(N_BALLS)] for __ in range(N_BALLS)]
        upper = [[0 for _ in range(N_BALLS)] for __ in range(N_BALLS)]

        for i in range(N_BALLS):
            for k in range(i, N_BALLS):
                uSum = 0
                for j in range(i): uSum += lower[i][j] * upper[j][k]
                upper[i][k] = self.values[i][k] - uSum


            for k in range(i, N_BALLS):
                if (i == k): lower[i][i] = 1
                else:
                    lSum = 0
                    for j in range(i): lSum += lower[k][j] * upper[j][i]
                    lower[k][i] = (self.values[k][i] - lSum) / upper[i][i]

        return lower, upper

class Ball():

    MASS = 1  # kg

    RADIUS = 10

    def __init__(self, theta0):
        self.theta = theta0
        self.angularVel = 0

        self.color = (random.randint(0,255),random.randint(0,255),random.randint(0,255))
        
    def timestep(self, dT, angularAccel):
        self.angularVel += angularAccel * dT
        self.theta += self.angularVel * dT

        self.theta = ((self.theta + math.pi) % (2*math.pi)) - math.pi


class Pendulum():
    
    GRAVITY = 9.81  # ms^-2
    LENGTH = 1  # m

    PIXELS_PER_METER = 100  # px/m

    def __init__(self, pivot, initialThetas):
        self.pivot = pivot
        self.balls = []
        for thetaInit in initialThetas:
            self.balls.append(Ball(thetaInit))

    def tick(self, dT, surface):
        angularAccels = self.solveAccels()

        for i in range(len(self.balls)):
            self.balls[i].timestep(dT, angularAccels[i])

        # compute new positions
        positions = [None for i in range(N_BALLS+1)]
        positions[0] = ( self.pivot[0], self.pivot[1])
        for i in range(len(self.balls)):
            positions[i+1] = (
                positions[i][0] + (Pendulum.LENGTH * Pendulum.PIXELS_PER_METER * math.sin(self.balls[i].theta)),
                positions[i][1] + (Pendulum.LENGTH * Pendulum.PIXELS_PER_METER * math.cos(self.balls[i].theta)),
            )


        for i in range(len(positions)):

            if i == 0: mColor = (0,0,255)
            else: mColor = self.balls[i-1].color

            gfxdraw.aacircle(surface, int(positions[i][0]), int(positions[i][1]), Ball.RADIUS, mColor)
            gfxdraw.filled_circle(surface, int(positions[i][0]), int(positions[i][1]), Ball.RADIUS, mColor)

            if i+1 != len(positions):
                # draw line
                pygame.draw.aaline(surface, (255,255,255), positions[i], positions[i+1])


    def computeEnergy(self):  # WHY ISNT THIS CONSTANT
        eKinetic = 0
        for i in range(len(self.balls)):

            ck = (sum([Pendulum.LENGTH * self.balls[j].angularVel * math.cos(self.balls[j].theta) for j in range(i+1)]))**2
            ck += (sum([Pendulum.LENGTH * self.balls[j].angularVel * math.sin(self.balls[j].theta) for j in range(i+1)]))**2
            ck *= Ball.MASS
            eKinetic += ck

        eKinetic /= 2
    
        ePotential = 0
        for i in range(len(self.balls)):

            ePotential += Ball.MASS * ((N_BALLS * Pendulum.LENGTH) - sum([Pendulum.LENGTH * math.cos(self.balls[j].theta) for j in range(i+1)]))

        ePotential *= Pendulum.GRAVITY


        return eKinetic, ePotential


    def aMatrix(self):
        matA = [[0 for _ in range(N_BALLS)] for __ in range(N_BALLS)]

        for i in range(N_BALLS):
            for j in range(N_BALLS):
                matA[i][j] = Pendulum.LENGTH * math.cos(self.balls[i].theta - self.balls[j].theta) * (N_BALLS - max(i,j)) * Ball.MASS

        return Matrix(matA)


    def bMatrix(self):
        matB = [0 for _ in range(N_BALLS)]

        for i in range(N_BALLS):
            matB[i] = -Pendulum.GRAVITY * math.sin(self.balls[i].theta) * (N_BALLS - i) * Ball.MASS

            for j in range(N_BALLS):
                matB[i] -= Pendulum.LENGTH * (math.sin(self.balls[i].theta - self.balls[j].theta) + (self.balls[j].angularVel**2) * math.sin(self.balls[i].theta - self.balls[j].theta)) * (N_BALLS - max(i,j)) * Ball.MASS

        return matB


    def solveAccels(self):
        A = self.aMatrix()
        b = self.bMatrix()

        lower, upper = A.decomp()

        y = [0 for _ in range(N_BALLS)]
        for i in range(N_BALLS):
            mSum = 0
            for j in range(i):
                mSum += y[j] * lower[i][j]
            y[i] = b[i] - mSum


        x = [0 for _ in range(N_BALLS)]
        for i in range(N_BALLS-1, -1, -1):
            mSum = 0
            for j in range(N_BALLS-1, i, -1):
                mSum += x[j] * upper[i][j]
            x[i] = (y[i] - mSum) / upper[i][i]

        return x
            

def __main__():

    global N_BALLS
    N_BALLS = int(sys.argv[1])

    pygame.init()

    SCREEN_WIDTH = pygame.display.Info().current_w
    SCREEN_HEIGHT = pygame.display.Info().current_h

    FPS = 240

    CLOCK = pygame.time.Clock()
    SCREEN = pygame.display.set_mode((0,0), pygame.FULLSCREEN)

    mPend = Pendulum((SCREEN_WIDTH/2, SCREEN_HEIGHT/2), [random.uniform(0, math.pi) for _ in range(N_BALLS)])

    frames = 0
    while 1:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                exit

        SCREEN.fill((0,0,0))


        mPend.tick(1/FPS, SCREEN)
        
        frames += 1
        energy = mPend.computeEnergy()
        print("{:>5}".format(str(frames)) + "    |    " + "{:>8}".format(round(frames/FPS, 3))+"s" + "    |    "  + "{:.2e}".format(energy[0]) + "    |    " + "{:.2e}".format(energy[1]) + "    |    " + "{:.2e}".format(energy[0] + energy[1]    ) + "    |    " + str(int(round(CLOCK.get_fps(), 0))))

        pygame.display.flip()

        CLOCK.tick(FPS)

if __name__ == "__main__":
    __main__()