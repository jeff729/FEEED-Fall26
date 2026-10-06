#include "kinematics.h"
#include "Config.h"
#include <math.h>

static const float K_PI = 3.14159265358979323846f;

bool twoLinkIK(float x, float y, float a, float b, bool elbowup, float &t1, float &t2) {
  if (!isfinite(x) || !isfinite(y) || !isfinite(a) || !isfinite(b) || a <= 0 || b <= 0)
    return false;
  const float radius = hypotf(x,y);
  if (!isfinite(radius) || radius < 0.000001f) return false;
  const float denominator = 2.0f*a*b;
  if (!isfinite(denominator) || denominator <= 0) return false;
  float d = (x*x+y*y-a*a-b*b)/denominator;
  if (!isfinite(d) || d > 1.0f+Config::IK_ROUNDOFF_TOLERANCE ||
      d < -1.0f-Config::IK_ROUNDOFF_TOLERANCE) return false;
  // Only floating point overshoot within the explicit tolerance is clamped.
  if (d > 1) d=1;
  if (d < -1) d=-1;
  float radicand = 1.0f-d*d;
  if (radicand < 0) radicand=0;
  const float elbow = atan2f((elbowup ? 1.0f : -1.0f)*sqrtf(radicand),d);
  const float shoulder = atan2f(y,x)-atan2f(b*sinf(elbow),a+b*cosf(elbow));
  if (!isfinite(shoulder) || !isfinite(elbow)) return false;
  t1=shoulder; t2=elbow;
  return true;
}

bool valid_joint_angles(float q1, float q2) {
  return isfinite(q1) && isfinite(q2) && q1 >= -K_PI && q1 <= 0 && q2 >= 0 && q2 <= K_PI;
}

bool calc_ik(float x, float y, float &q1, float &q2) {
  return twoLinkIK(x,y,L1,L2,true,q1,q2);
}

bool calc_fk(float q1, float q2, float &x, float &y) {
  if (!isfinite(q1) || !isfinite(q2) || !isfinite(L1) || !isfinite(L2) || L1 <= 0 || L2 <= 0)
    return false;
  const float nx=L1*cosf(q1)+L2*cosf(q1+q2);
  const float ny=L1*sinf(q1)+L2*sinf(q1+q2);
  if (!isfinite(nx) || !isfinite(ny)) return false;
  x=nx; y=ny;
  return true;
}

WorkspaceResult project_workspace(float &x, float &y) {
  if (!isfinite(x) || !isfinite(y) || !isfinite(L1) || !isfinite(L2) || L1 <= 0 || L2 <= 0)
    return WorkspaceResult::Invalid;
  float nx=x, ny=y;
  const float reach=L1+L2;
  const float inner=fmaxf(Config::WORKSPACE_MIN_RADIUS,fabsf(L1-L2));
  if (!isfinite(reach) || inner >= reach) return WorkspaceResult::Invalid;
  // Project only manual calibration requests. Loaded profiles are separately
  // rejected if this requires more than a tiny roundoff correction.
  for (uint8_t i=0; i<8; ++i) {
    if (ny > Config::WORKSPACE_MIN_Y) ny=Config::WORKSPACE_MIN_Y;
    if (nx < -Config::WORKSPACE_LEFT_FRACTION*reach) nx=-Config::WORKSPACE_LEFT_FRACTION*reach;
    float radius=hypotf(nx,ny);
    if (!isfinite(radius)) return WorkspaceResult::Invalid;
    if (radius < inner) {
      if (radius < 0.01f) { nx=0; ny=-inner; }
      else { nx*=inner/radius; ny*=inner/radius; }
    } else if (radius > reach) { nx*=reach/radius; ny*=reach/radius; }
    radius=hypotf(nx+L1,ny);
    if (radius < L1) {
      if (radius < 0.01f) { nx=-L1; ny=-L1; }
      else { nx=(nx+L1)*L1/radius-L1; ny*=L1/radius; }
    }
  }
  float a,b;
  if (!isfinite(nx) || !isfinite(ny) || !calc_ik(nx,ny,a,b)) return WorkspaceResult::Invalid;
  // Roundoff at shoulder's hard stop does not justify an out-of-limit pulse.
  if (a < -K_PI && a >= -K_PI-Config::IK_ROUNDOFF_TOLERANCE) a=-K_PI;
  if (!valid_joint_angles(a,b) || hypotf(nx,ny) < inner-0.001f ||
      nx < -Config::WORKSPACE_LEFT_FRACTION*reach-0.001f || ny > Config::WORKSPACE_MIN_Y+0.000001f)
    return WorkspaceResult::Invalid;
  const bool changed=(nx != x || ny != y);
  x=nx; y=ny;
  return changed ? WorkspaceResult::Adjusted : WorkspaceResult::Unchanged;
}

bool constrain_ik_point(float &x, float &y) {
  return project_workspace(x,y) != WorkspaceResult::Unchanged;
}
