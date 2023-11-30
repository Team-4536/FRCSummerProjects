import ntcore
import wpilib
import wpimath.system.plant as plant
import rev
import navx
import math
import flymer
import timing
from real import V2f
from subsystems.mech import mechController
import socketing
from inputs import deadZone
from PIDController import PIDController
from real import angleWrap
from collections.abc import Callable


def retreat(r: flymer.Flymer) -> bool: #RETREATING&RETRACTING-------------------
    r.driveUnif(-r.autospeed)
    r.driveArmGoal(r.defaultgoal.y, r.defaultgoal.x)
    if r.time.timeSinceInit - r.auto.stagestart > 5.5:
        r.driveUnif(0)
        if r.hal.retractPos <= (r.defaultgoal.x+.05):
            r.driveArmSpeed(0,0)
            return True
    return False


def balance(r: flymer.Flymer) -> bool:
    if r.ontop == False:
        r.driveUnif(r.autospeed)
        if abs(r.hal.gyroPitch) > 10:
            r.ontop = True
        if r.time.timeSinceInit - r.auto.stagestart > 10:
            r.driveUnif(0)
            return True
    else:
        if r.ontop == True and abs(r.hal.gyroPitch) < 10:
            r.hal.brakesExtended = True
            r.driveUnif(0)
            return True
        if r.ontop == True and r.hal.gyroPitch > 10:
            r.driveUnif(-r.balancespeed)
        if r.ontop == True and r.hal.gyroPitch < 10:
            r.driveUnif(r.balancespeed)
    return False

def makeDriveStage(time, speed):
    def function(r: flymer.Flymer) -> bool:
        if r.time.timeSinceInit - r.auto.stagestart < time:
            r.driveUnif(speed)
            return False
        else:
            r.driveUnif(0)
            return True
    return function

def makeArmStage(goalx, goaly):
    def function(r: flymer.Flymer) -> bool:
        r.driveArmGoal(goaly, goalx)
        if abs(r.hal.retractPos - (r.scoregoal.x)) <= .05:
            return True
        return False
    return function

def makeGrabberStage(state): #grabber state true/false
    def function(r: flymer.Flymer) -> bool:
            r.hal.grabberOpen = state
            if r.time.timeSinceInit - r.auto.stagestart < .5:
                return True
            return False
    return function

def makeTurnStage(goalAngle, direction): #clockwise = true counterclockwise = false
    def function(r: flymer.Flymer) -> bool:
        angle = (goalAngle - r.hal.gyroYaw)
        if direction and angle < 0:
            angle += 360
        elif not direction and angle > 0:
            angle -= 360
        turnspeed = angle*.005
        r.driveTurn(turnspeed)
        if abs(angleWrap(angle)) <= 2:
            return True
        return False
    return function
        
              





class Auto():
    def __init__(self, stagelist: list[Callable[[flymer.Flymer], bool]], time:float) -> None:
        self.list = stagelist
        self.listindex = 0
        self.stagestart = time
    def update(self, r: flymer.Flymer) -> None:

        r.server.putUpdate("stage start", self.stagestart)
        r.server.putUpdate("stage number", self.listindex)
        if self.listindex < len(self.list):
            stage = self.list[self.listindex]
            done = stage(r)
            if done:
                self.listindex+=1
                self.stagestart = r.time.timeSinceInit