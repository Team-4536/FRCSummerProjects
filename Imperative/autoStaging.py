import ntcore
import wpilib
import wpimath.system.plant as plant
import rev
import navx
import math
import robot
import timing
from real import V2f
from subsystems.mech import mechController
import socketing
from inputs import deadZone
from PIDController import PIDController
from real import angleWrap
from collections.abc import Callable

def drive(self) ->bool:
    return False

def approach(r: robot.Flymer) -> bool: #APPROACHING----------------------------
    r.driveUnif(r.approachspeed)
    if r.time.timeSinceInit - r.auto.stagestart > .5:
        r.driveUnif(0)
        return True
    return False

def extend(r: robot.Flymer) -> bool: #EXTENDING--------------------------------
    r.driveArmGoal(r.scoregoal.y, r.scoregoal.x)

    if r.retractEncoder.getPosition() >= (r.scoregoal.x-.05):
        return True
    return False

def score(r: robot.Flymer) -> bool: #SCORING-----------------------------------
    r.driveArmGoal(r.scoregoal.y, r.scoregoal.x)
    r.grabber.set(wpilib.DoubleSolenoid.Value.kReverse)
    return True

def retreat(r: robot.Flymer) -> bool: #RETREATING&RETRACTING-------------------
    r.driveUnif(-r.approachspeed)
    r.driveArmGoal(r.defaultgoal.y, r.defaultgoal.x)
    if r.time.timeSinceInit - r.auto.stagestart > 1:  
        r.driveUnif(0)
        if r.retractEncoder.getPosition() <= (r.defaultgoal.x+.05):
            r.driveArmSpeed(0,0)
            return True
    return False

def turn(r: robot.Flymer) -> bool: #TURNING------------------------------------
    angle = angleWrap(180 - r.gyro.getAngle())
    turnspeed = angle*.005
    r.driveTurn(turnspeed)
    if abs(angle) <= 180:
        return True
    return False

def exit(r: robot.Flymer) -> bool:
    r.driveUnif(r.autospeed)
    if r.time.timeSinceInit - r.auto.stagestart > 6:  
        r.driveUnif(0)
        return True
    return False


def balance(r: robot.Flymer) -> bool:
    if r.ontop == False:
        r.driveUnif(r.autospeed)
        if abs(r.gyro.getPitch()) > 10:
            r.ontop = True
        if r.time.timeSinceInit - r.auto.stagestart > 10:
            r.driveUnif(0)
            return True
    else:
        if r.ontop == True and abs(r.gyro.getPitch()) < 10:
            r.brakes.set(wpilib.DoubleSolenoid.Value.kForward)
            r.driveUnif(0)
            return True
        if r.ontop == True and r.gyro.getPitch() > 10:
            r.driveUnif(-r.balancespeed)
        if r.ontop == True and r.gyro.getPitch() < 10:
            r.driveUnif(r.balancespeed)
    return False

class Auto():
    def __init__(self, stagelist: list[Callable[[robot.Flymer], bool]], time:float) -> None:
        self.list = stagelist
        self.listindex = 0
        self.stagestart = time
    def update(self, robot: robot.Flymer) -> None:
        robot.server.putUpdate("stage start", self.stagestart)
        robot.server.putUpdate("stage number", self.listindex)
     
        stage = self.list[self.listindex]
        done = stage(robot)
        if done:
            if self.listindex != len(self.list):
                self.listindex+=1 
                self.stagestart = robot.time.timeSinceInit
        