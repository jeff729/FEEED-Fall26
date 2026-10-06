#ifndef KINEMATICS_H
#define KINEMATICS_H

#include "Config.h"
#define Q1_HOME Config::HOME_Q1
#define Q2_HOME Config::HOME_Q2
#define HOME_X Config::HOME_X_MM
#define HOME_Y Config::HOME_Y_MM

extern const float L1;
extern const float L2;

// Outputs are committed only on success. Geometry arguments permit host tests
// for unequal/invalid links; the feeder itself still uses 100/100 mm.
bool twoLinkIK(float x, float y, float a, float b, bool elbowup, float &t1, float &t2);
bool valid_joint_angles(float q1, float q2);
bool valid_cartesian_angles(float q1, float q2);
enum class WorkspaceResult { Invalid, Unchanged, Adjusted };
WorkspaceResult project_workspace(float &x, float &y);

/**
 * @brief Calculates forward kinematics for given joint values.
 * @return True if the kinematic calculation was successful. If so, writes end effector position to x_ptr and y_ptr.
 */
bool calc_fk(float q1, float q2, float &x_ptr, float &y_ptr);

/**
 * @brief Calculates inverse kinematics for a given point relative to the robot's origin (the shoulder joint).
 * @return True if the kinematic calculation was successful. If so, writes joint values to q1_ptr and q2_ptr.
 */
bool calc_ik(float x, float y, float &q1_ptr, float &q2_ptr);

/**
 * @brief Constrains an ik point to be within workspace bounds.
 * @return True if modified or invalid. Invalid inputs are left unchanged;
 * new callers should use project_workspace to distinguish these outcomes.
 */
bool constrain_ik_point(float &x, float &y);

#endif
