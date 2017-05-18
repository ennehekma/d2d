'''
Copyright (c) 2014, K. Kumar (me@kartikkumar.com)
All rights reserved.
'''

###################################################################################################
# Set up modules and packages
###################################################################################################

import numpy as np

###################################################################################################

###################################################################################################

###################################################################################################
# Convert mean-motion to semi-major axis.
###################################################################################################

def convertMeanMotionToSemiMajorAxis(meanMotion, gravitionalParameter):
    return ( gravitionalParameter / ( meanMotion ** 2.0 ) ) ** ( 1.0 / 3.0 )

###################################################################################################

###################################################################################################
# Convert mean-motion to semi-major axis.
###################################################################################################

# Convert semi-major axis to mean-motion.
def convertSemiMajorAxisToMeanMotion(semiMajorAxis, gravitionalParameter):
    return np.sqrt( gravitionalParameter / ( semiMajorAxis ** 3.0 ) )

###################################################################################################