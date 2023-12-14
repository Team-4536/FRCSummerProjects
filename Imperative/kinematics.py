import ntcore
import wpilib
import wpimath.system.plant as plant
import rev
import navx
import math
import numpy
import timing
from real import V2f
from subsystems.mech import mechController
import socketing
from inputs import deadZone
from PIDController import PIDController
from real import angleWrap
from collections.abc import Callable

LIFT_CONVERSION_VALUE = .1
TURRET_CONVERSION_VALUE = .2
RETRACT_CONVERSION_VALUE = .15
 # physical angles WILL be different because of physical 
def getSphericalCoords(liftPos, turretPos, retractPos):   
    liftDegrees, turretDegrees, retractDistance = liftPos * LIFT_CONVERSION_VALUE, turretPos * TURRET_CONVERSION_VALUE, retractPos * RETRACT_CONVERSION_VALUE
   
    return [liftDegrees, turretDegrees, retractDistance]

def getCartesianCoords(liftPos, turretPos, retractPos):
    sphericalCoords = getSphericalCoords(liftPos, turretPos, retractPos)
    liftDegrees, turretDegrees, retractDistance = sphericalCoords[0], sphericalCoords[1], sphericalCoords[2]

    liftZ = math.sin(math.radians(liftDegrees))
    liftX = math.cos(math.radians(liftDegrees))

    turretY = math.sin(math.radians(turretDegrees))
    turretX = math.cos(math.radians(turretDegrees))

    manipulatorX = turretX*liftX*retractDistance
    manipulatorY = turretY*liftX*retractDistance
    manipulatorZ = liftZ*retractDistance

    return [manipulatorX, manipulatorY, manipulatorZ]

def cartesianToSpherical(manipulatorX, manipulatorY, manipulatorZ):
    dist = math.sqrt(manipulatorX**2+manipulatorY**2+manipulatorZ**2)
    theta = numpy.arcsin(manipulatorZ/dist)
    phi = numpy.arctan(manipulatorY/manipulatorX)
    
    return [math.degrees(theta), math.degrees(phi), dist]

def sphericalToEncoder(theta, phi, dist):
    liftDegrees = theta / LIFT_CONVERSION_VALUE
    turretDegrees = phi / TURRET_CONVERSION_VALUE
    retractDistance = dist / RETRACT_CONVERSION_VALUE

    return [liftDegrees, turretDegrees, retractDistance]

def encoderToCartesion(liftPos, turretPos, retractPos):
    return getCartesianCoords(getSphericalCoords(liftPos, turretPos, retractPos)[0],
                            getSphericalCoords(liftPos, turretPos, retractPos)[1],
                            getSphericalCoords(liftPos, turretPos, retractPos)[2])