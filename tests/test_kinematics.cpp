#include <cstdio>
#include <cmath>
#include <limits>
#include "kinematics.h"
#include "Profile.h"
#include "EEPROM.h"
const float L1 = 100.0f, L2 = 100.0f;
FakeEEPROM EEPROM;
static int failures = 0, checks = 0;
#define CHECK(condition) do { ++checks; if (!(condition)) { \
  std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); ++failures; } } while (0)
static bool near(float a, float b) { return std::fabs(a-b) < 0.002f; }
int main() {
  const float nan = std::numeric_limits<float>::quiet_NaN();
  const float inf = std::numeric_limits<float>::infinity();
  float a = 12, b = 13;
  CHECK(!calc_ik(nan, -100, a, b));
  CHECK(a == 12 && b == 13); // Failure must not publish a partial target.
  CHECK(!calc_ik(250, -10, a, b));
  CHECK(!calc_ik(inf, 0, a, b));
  CHECK(!calc_ik(0, 0, a, b)); // Fully folded singularity.
  CHECK(calc_ik(100, -100, a, b));
  CHECK(near(a, -1.5707963f) && near(b, 1.5707963f));
  float x = 0, y = 0;
  constrain_ik_point(x, y);
  CHECK(std::isfinite(x) && std::isfinite(y));
  CHECK(calc_ik(x, y, a, b));
  x = -100; y = 0;
  constrain_ik_point(x, y);
  CHECK(std::isfinite(x) && std::isfinite(y));
  CHECK(calc_ik(x, y, a, b));
  Profile p = {-80,-155,-76,-175,0,-177.5f,76,-180,80,-155};
  EEPROM.put(0, p);
  Profile loaded;
  CHECK(load_profile(0, loaded));
  CHECK(near(loaded.end_x, 80) && near(loaded.end_y, -155));
  p.middle_x = nan;
  EEPROM.put(0, p);
  CHECK(!load_profile(0, loaded));
  CHECK(!save_profile(p, 0));
  CHECK(!load_profile(4, loaded));
  CHECK(validate_profile(default_profile(0)));
  CHECK(validate_profile(default_profile(3)));
  CHECK(!twoLinkIK(5, 0, 100, 50, true, a, b)); // D < -1.
  CHECK(!twoLinkIK(100, -100, 0, 100, true, a, b));
  CHECK(!twoLinkIK(100, -100, inf, 100, true, a, b));
  CHECK(twoLinkIK(200.00001f, 0, 100, 100, true, a, b));
  CHECK(!twoLinkIK(200.01f, 0, 100, 100, true, a, b));
  x=nan; y=-100;
  CHECK(project_workspace(x,y) == WorkspaceResult::Invalid);
  CHECK(std::isnan(x) && y == -100);
  x=inf; y=-100;
  CHECK(project_workspace(x,y) == WorkspaceResult::Invalid);
  a = 12; b = 13;
  CHECK(!calc_fk(nan, 0, a, b));
  CHECK(a == 12 && b == 13);
  CHECK(calc_fk(0, 0, x, y));
  CHECK(near(x, 200) && near(y, 0));
  CHECK(calc_fk(-1.5707963f, 1.5707963f, x, y));
  CHECK(near(x, 100) && near(y, -100));
  for (float q1 = -2.8f; q1 < -0.5f; q1 += 0.2f) {
    for (float q2 = 0.3f; q2 < 2.7f; q2 += 0.2f) {
      CHECK(calc_fk(q1, q2, x, y));
      CHECK(calc_ik(x, y, a, b));
      CHECK(near(a, q1) && near(b, q2));
    }
  }
  std::printf("kinematics/profile: %d checks, %d failures\n", checks, failures);
  return failures ? 1 : 0;
}
