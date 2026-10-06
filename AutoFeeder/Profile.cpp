#include "Profile.h"
#include "kinematics.h"
#include "Config.h"
#include <math.h>
#include <string.h>
#include <EEPROM.h>

Profile profiles[NUM_PROFILES];
static_assert(sizeof(Profile)==40,"Keep the inherited EEPROM profile layout");
static const Profile plate_profile={-80,-155,-76,-175,0,-177.5f,76,-180,80,-155};
// The inherited (-75,-82.5) entry was inside its own shoulder exclusion
// circle. These are the coordinates the old loader actually projected to;
// retain that effective baseline instead of inventing a new bowl geometry.
static const Profile bowl_profile={-70.999260f,-95.702440f,-70,-175,0,-175,70,-175,60,-90};

const Profile &default_profile(uint8_t idx) {
  return idx < NUM_PROFILES/2 ? bowl_profile : plate_profile;
}

static bool validate_point(float &x, float &y) {
  const float ox=x, oy=y;
  if (project_workspace(x,y) == WorkspaceResult::Invalid) return false;
  if (hypotf(x-ox,y-oy) > Config::PROFILE_ROUNDOFF_MM) return false;
  float a,b;
  return calc_ik(x,y,a,b) && valid_joint_angles(a,b);
}

bool normalize_profile(Profile &p) {
  Profile candidate=p;
  if (!validate_point(candidate.entry_x,candidate.entry_y) ||
      !validate_point(candidate.bottom_x,candidate.bottom_y) ||
      !validate_point(candidate.middle_x,candidate.middle_y) ||
      !validate_point(candidate.front_x,candidate.front_y) ||
      !validate_point(candidate.end_x,candidate.end_y)) return false;
  float x=candidate.end_x+Config::SCOOP_EXIT_X_MM;
  float y=candidate.end_y+Config::SCOOP_EXIT_Y_MM;
  if (!validate_point(x,y)) return false;
  x=candidate.end_x; y=candidate.end_y+Config::RETURN_CLEARANCE_MM;
  if (!validate_point(x,y)) return false;
  p=candidate;
  return true;
}

bool validate_profile(const Profile &p) {
  Profile copy=p;
  return normalize_profile(copy);
}

void reset_profiles() {
  for (uint8_t i=0; i<NUM_PROFILES; ++i) {
    profiles[i]=default_profile(i);
    save_profile(profiles[i],i);
  }
}

bool save_profile(const Profile &p, uint8_t idx) {
  if (idx >= NUM_PROFILES) return false;
  Profile candidate=p;
  if (!normalize_profile(candidate)) return false;
  // Same four raw 40-byte AVR structs, same addresses, no migration/header.
  EEPROM.put(sizeof(Profile)*idx,candidate);
  Profile readback;
  EEPROM.get(sizeof(Profile)*idx,readback);
  return memcmp(&candidate,&readback,sizeof(Profile))==0 && validate_profile(readback);
}

bool load_profile(uint8_t idx, Profile &p) {
  if (idx >= NUM_PROFILES) return false;
  Profile candidate;
  EEPROM.get(sizeof(Profile)*idx,candidate);
  if (!normalize_profile(candidate)) return false;
  p=candidate;
  return true;
}

bool get_profile_step(const Profile &p, int step, float &x, float &y) {
  switch(step) {
    case 0: x=p.entry_x; y=p.entry_y; break;
    case 1: x=p.bottom_x; y=p.bottom_y; break;
    case 2: x=p.middle_x; y=p.middle_y; break;
    case 3: x=p.front_x; y=p.front_y; break;
    case 4: x=p.end_x+Config::SCOOP_EXIT_X_MM; y=p.end_y+Config::SCOOP_EXIT_Y_MM; break;
    default: return false;
  }
  return isfinite(x) && isfinite(y);
}
