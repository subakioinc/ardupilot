
#ifndef __ARDUCOPTER_CONFIG_MOTORS_H__
#define __ARDUCOPTER_CONFIG_MOTORS_H__

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
//
//  DO NOT EDIT this file to adjust your configuration.  Create your own
//  APM_Config.h and use APM_Config.h.example as a reference.
//
// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//

#include "config.h"

//
//
// Output CH mapping for ArduCopter motor channels
//
//
#define CH_INVALID (-1)

#if CONFIG_APM_HARDWARE == APM_HARDWARE_APM2
# define MOT_1 CH_1
# define MOT_2 CH_2
# define MOT_3 CH_3
# define MOT_4 CH_4
# define MOT_5 CH_5
# define MOT_6 CH_6
# define MOT_7 CH_7
# define MOT_8 CH_8
#elif CONFIG_APM_HARDWARE == APM_HARDWARE_APM1
#if FRAME_CONFIG ==	QUAD_FRAME
// X and Plus
# define MOT_1 CH_1
# define MOT_2 CH_2
# define MOT_3 CH_3
# define MOT_4 CH_4
// Placeholders:
# define MOT_5 CH_INVALID
# define MOT_6 CH_INVALID
# define MOT_7 CH_INVALID
# define MOT_8 CH_INVALID
#elif FRAME_CONFIG == TRI_FRAME
# define MOT_1 CH_1
# define MOT_2 CH_2
# define MOT_3 CH_4
// Placeholders:
# define MOT_4 CH_INVALID
# define MOT_5 CH_INVALID
# define MOT_6 CH_INVALID
# define MOT_7 CH_INVALID
# define MOT_8 CH_INVALID
#elif FRAME_CONFIG == HEXA_X_FRAME
// X orientation (what will be the best way to set 'if' 'else'? )
# define MOT_1 CH_7
# define MOT_2 CH_3
# define MOT_3 CH_2
# define MOT_4 CH_8
# define MOT_5 CH_4
# define MOT_6 CH_1
// Placeholders:
# define MOT_7 CH_INVALID
# define MOT_8 CH_INVALID
#elif FRAME_CONFIG == HEXA_PLUS_FRAME
# define MOT_1 CH_4
# define MOT_2 CH_1
# define MOT_3 CH_7
# define MOT_4 CH_3
# define MOT_5 CH_2
# define MOT_6 CH_8
// Placeholders:
# define MOT_7 CH_INVALID
# define MOT_8 CH_INVALID
#elif FRAME_CONFIG == Y6_FRAME
# define MOT_1 CH_1
# define MOT_2 CH_7
# define MOT_3 CH_3
# define MOT_4 CH_2
# define MOT_5 CH_8
# define MOT_6 CH_4
// Placeholders:
# define MOT_7 CH_INVALID
# define MOT_8 CH_INVALID
#elif FRAME_CONFIG == OCTA_FRAME
# define MOT_1 CH_2
# define MOT_2 CH_1
# define MOT_3 CH_11
# define MOT_4 CH_10
# define MOT_5 CH_8
# define MOT_6 CH_7
# define MOT_7 CH_4
# define MOT_8 CH_3
#elif FRAME_CONFIG == OCTA_V_FRAME
# define MOT_1 CH_4
# define MOT_2 CH_2
# define MOT_3 CH_8
# define MOT_4 CH_10
# define MOT_5 CH_7
# define MOT_6 CH_1
# define MOT_7 CH_3
# define MOT_8 CH_11
#elif FRAME_CONFIG == OCTA_QUAD_FRAME
# define MOT_1 CH_1
# define MOT_2 CH_2
# define MOT_3 CH_3
# define MOT_4 CH_4
# define MOT_5 CH_7
# define MOT_6 CH_8
# define MOT_7 CH_10
# define MOT_8 CH_11
#else // HELI
# define MOT_1 CH_1
# define MOT_2 CH_2
# define MOT_3 CH_3
# define MOT_4 CH_4
# define MOT_5 CH_5
# define MOT_6 CH_6
# define MOT_7 CH_7
# define MOT_8 CH_8
#endif // FRAME_CONFIG
#endif // CONFIG_APM_HARDWARE

//
//
// Output CH mapping for TRI_FRAME yaw servo
//
//
#if CONFIG_APM_HARDWARE == APM_HARDWARE_APM2
# define CH_TRI_YAW   CH_7
#elif CONFIG_APM_HARDWARE == APM_HARDWARE_APM1
# define CH_TRI_YAW   CH_7
#endif

//
//
// Output CH mapping for Aux channels
//
//
#if CONFIG_APM_HARDWARE == APM_HARDWARE_APM2
/* Camera Pitch and Camera Roll: Not yet defined for APM2 
 * They will likely be dependent on the frame config */
# define CH_CAM_PITCH (-1)
# define CH_CAM_ROLL  (-1)
#elif CONFIG_APM_HARDWARE == APM_HARDWARE_APM1
# define CH_CAM_PITCH CH_5
# define CH_CAM_ROLL  CH_6
#endif

#endif // __ARDUCOPTER_CONFIG_MOTORS_H__
