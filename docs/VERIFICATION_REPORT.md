# Fall 2026 software verification report

Verified October 9, 2026. Physical testing pending. No firmware was uploaded,
no port was detected/opened, and no physical motion was commanded.

## Revisions and environment

- Starting development: `3fa818db2e62f5d08bb78fadfa861dc6b18c0f4f`.
- Inherited main: `c9ea8ac4f724e141cdb5ad1593cc350be78e9a39`.
- Tested source, harness and workflow: [`6cab55d6eb4339dc3eeef6a7af7acd11074dcaa9`](https://github.com/jeff729/FEEED-Fall26/commit/6cab55d6eb4339dc3eeef6a7af7acd11074dcaa9).
- Firmware fix revision: [`f06cc9a`](https://github.com/jeff729/FEEED-Fall26/commit/f06cc9ae6db1cf75b7fd87b2c27201efdce9273f).
- This report and the accompanying guide/slide updates are a later
  documentation-only change; the tested source revision is not this report's
  containing commit. No circular verification claim is intended.
- Local environment: Windows, PowerShell, Git Bash 5.2.37. No WSL installed.
  Arduino CLI 1.5.1 (01f3d4f2b), arduino:avr 1.8.8, AVR GCC
  7.3.0-atmel3.6.1-arduino7, Servo 1.2.2, `arduino:avr:uno`.
- Hosted host tests: GCC 14.2.0, official `gcc:14.2.0` container digest
  `sha256:b99b86a28812b1e6453a231a947dc43d76fe192788a12f344a9b568bf9f5d24c`,
  Ubuntu 24.04 runner. The same pinned Arduino versions build in the Uno jobs.

The initial local host commands were attempted before firmware edits:
`CXX=clang++ ./tests/run_tests.sh` and
`CXX=clang++ SANITIZE=1 ./tests/run_tests.sh`, both exit 1. Windows Application
Control blocked `ld.lld.exe` (0x11C7); no assertions executed in those attempts.
The portable LLVM-MinGW 20261006 UCRT package reported Clang 23.1.3; its SHA256
matched the official release asset digest. The policy was not bypassed and
local host success is not claimed. The documented AVR core 1.8.8 was installed
for these builds; the pre-existing CLI 1.5.1 and Servo 1.2.2 were used.

## Work already present and new fixes

The October 6 implementation was retained: finite transactional IK/FK,
profile endpoint correction and compatible four-slot EEPROM handling,
nonblocking states/input/LED feedback, synchronized time-based motion,
per-cycle mode/profile locking, bounded contact recovery, validated cancel
and return, stopped calibration plate, save readback, and disabled experiments.
`docs/CURRENT_FIRMWARE_ARCHITECTURE.md` continues to describe the imported
firmware, including its old defects, not this implementation.

This pass confirmed and fixed two defects:

1. A profile/storage fault suppressed overload sampling, so a reset could
   write startup servo commands and enable power while ADC current exceeded
   500. Overload now takes priority over those recoverable faults. Recovery
   samples current/voltage again after EEPROM writes before issuing startup
   commands or enabling power.
2. Cartesian paths checked joint validity but could cross inside the existing
   10 mm minimum radius. The path from (10,-0.001) to (0,-10) has valid endpoints
   but a roughly 7.071 mm midpoint radius. Closest-point analysis now rejects
   the whole chord before motion, preserving physical limits and joint-space
   startup/delivery support.

Regression-only commit [`d8f24d7`](https://github.com/jeff729/FEEED-Fall26/commit/d8f24d7e54ae51c825d92c1182cf92c1cc4b71fe)
[failed as expected](https://github.com/jeff729/FEEED-Fall26/actions/runs/37955564274)
for both defects before the fixes. Two additional threshold fixtures initially
injected contact at the shoulder-limited entry, where a 1 mm lift correctly
faults; injection was moved five seconds into scoop to isolate threshold
classification. No threshold or motion limit was weakened. The fixed revision
[passed](https://github.com/jeff729/FEEED-Fall26/actions/runs/37955758036).

## Fresh results

[Full verification of the tested revision](https://github.com/jeff729/FEEED-Fall26/actions/runs/37956093314)
completed successfully in all four jobs. This is an actual completed run, not
an inference from the workflow file. Commands below are the shared scripts.

| Command or suite | Exit/result |
|---|---|
| `./tests/run_tests.sh` (hosted Linux) | 0; all suites below pass |
| `SANITIZE=1 ./tests/run_tests.sh` (hosted Linux) | 0; same counts and timing scenarios, ASan + UBSan |
| Kinematics/profile | 471 assertion checks, 0 failures |
| Control helpers | 5,874 assertion checks, 0 failures |
| Default firmware integration | 108 assertion checks, 0 failures |
| Jiggle + D4 trace integration | 119 assertion checks, 0 failures |
| One-shot lab boot integration | 5 assertion checks, 0 failures |
| Review regressions | 21 scenario executions, 75 assertion checks, 0 failures |
| Hardware invariants | 18 compile-time groups and 6 joint-boundary predicates, pass |
| Default-profile phase timing | 8 completed profile/mode scenarios, 0 failures |
| `./tools/compile_uno.sh` / `./tools/check_uno.sh default` | 0; 20,100/32,256 bytes flash; 595/2,048 bytes static SRAM |
| `./tools/check_uno.sh experimental` | 0; 20,378 bytes flash; 601 bytes static SRAM |
| `./tools/check_uno.sh uart-rejection` | 0; compiler fails with the specific expected D1 UART conflict |
| `./tools/check_repository.sh` | 0; whitespace, shell syntax, tracked artifact checks |

Total counted assertions: **6,652 per normal/sanitizer run**, not 6,652
independent tests. Many checks repeat through sampled motion loops. The older
five executables do not separately count independent scenarios. The new review
suite counts 21 isolated/parameterized scenario executions; timing adds eight
whole-cycle simulations. Invariant compile groups and six boundary predicates
are reported separately, not folded into the assertion count.

Before firmware changes, fresh CI at
[`ec2b71f`](https://github.com/jeff729/FEEED-Fall26/actions/runs/37955303243)
reproduced 6,577 assertions in both modes, default Uno 19,882/595 bytes and
experimental 20,128/601 bytes. Its firmware matched the starting source.

Host flags: `-std=c++11 -Wall -Wextra -Werror`; sanitizer adds
`-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer`.
Default Uno uses no extra feature flags and `--warnings all`. Experimental Uno
adds `ENABLE_PLATE_JIGGLE=1`, `RUN_PLATE_JIGGLE_ONCE_AT_HOME=1` and
`ENABLE_DEBUG_PIN_TRACE=1`. UART rejection adds only `ENABLE_UART_TELEMETRY=1`.
It matches the conflict diagnostic rather than accepting any compiler failure.

Both Uno builds emit four dependency unused-parameter warnings in Arduino
core `new.cpp` (lines 59, 68, 103, 106). No firmware-owned warnings were emitted
and none were suppressed. Static SRAM is not measured runtime stack headroom.
CI uses pinned official checkout/setup action commits, `contents: read`, no
configured secrets, no feeder/self-hosted runner, and no hardware upload.

## Simulated phase duration

All values are simulated seconds, using ideal commanded joint positions,
normal current/voltage, no contact, and a regular 10 ms loop. Startup/home
includes the unchanged 0.50 s initial settle. Return combines singularity
exit, Cartesian return and home. Slots 0/1 share the built-in bowl; 2/3 share
the plate. All four slots were run separately in both modes.

| Slots | Startup/home | Descend | Scoop | Lift | Return | Cycle movement total |
|---|---|---|---|---|---|---|
| 0 and 1 (bowl) | 65.58 | 1.98 | 60.88 | 17.90 | 47.66 | 128.42 |
| 2 and 3 (plate) | 70.20 | 7.26 | 42.66 | 13.82 | 52.28 | 116.02 |

Simple adds 0.60 s plate rotation, 0.40 s settle and exactly 6.50 s feed wait.
Advanced feed wait is user-controlled and unbounded; the harness deliberately
waited 2.00 s then pressed return, producing 2.04 s including debounce. Those
waits, startup and initial user gesture time are excluded from cycle movement
above. The simulated Simple cycle from leaving WAIT to home is about 135.92 s
for bowl and 123.52 s for plate. These long provisional cycles need practical
team review. No speeds were increased to reduce the result, and the old
per-loop rate cannot reliably be converted to rad/s from source alone.

These are not physical benchmarks. No loaded spoon tracking, actual motor
rotation, food retention, force, voltage calibration or person testing occurred.

## Coverage and remaining limits

New checks cover current 399/400/401/500/501 and voltage 661/662/663 boundaries,
simultaneous input, bounce, delayed scheduling, cancellation from three motion
phases, unsafe reset recovery, ADC changes during reset writes, dropped and
partial writes, failed calibration save, unaffected EEPROM slots, normal
hardware/feature constants and zero PWM before direction reversal. Existing
checks retain rollover, held/idle-edge input, mode/profile locks, persistent
moderate resistance, immediate overload, calibration rejection, LED feedback
and experimental exits. Real firmware runs against fake time/I/O/EEPROM/servo
commands; no fake models mechanical motion or physical contact force.

- No remaining confirmed defect from this review is left unfixed. This is
  not exhaustive proof. Sampled IK/clearance checks and live command checks
  do not establish collision clearance from unknown bowl geometry.
- Point-valid saved profiles may still fail trajectory preflight. Nonfinite,
  unreachable, substantially projected or near-singular saved points are
  rejected. Only the exact inherited bowl template receives its known entry
  projection in RAM. Rejection never silently resets EEPROM.
- The four raw 40-byte slots have no checksum or transaction marker.
  Readback detects a mismatched save; a plausible mixture after power loss
  can still load. Reset-all can partially update slots before a write fails.
  EEPROM calls are synchronous; test injection changes ADC values but does
  not model write latency. Measure worst-case sampling latency on the Uno.
- Planned Motion segments have nominal velocity/acceleration checks. Manual
  calibration caps velocity only. Startup pose, joystick reversals and
  cancel/contact/fault interruptions have no continuous acceleration bound.
- A failed cancel path holds and faults; successful home return is not
  guaranteed from every supported command pose. Severe overload never
  triggers a blind retreat. Current relief is a bounded heuristic, not force
  control. A0/A4 calibration and A1 physical wiring remain unverified.
- Startup snapping, bowl clearance, spoon tracking under load, plate coast,
  power integrity and fault-hold versus servo power-off consequences need
  supervised bench work. PWM zero proves only the software command.
- Simple's unchanged 6500 ms eating timeout requires caregiver/supervisor
  review before use with a person. Follow MOVEMENT_TEST_CHECKLIST.md in order;
  do not deliberately jam the mechanism or test overload against a person.

## Publication review

Both fetched branches and all reachable history, including superseded/deleted
files, were reviewed; there are no tags. Local Gitleaks 8.30.1 scanned history
with `git . --log-opts="--all --full-history" --redact` and found no leaks.
Manual review inspected historical source comments/strings, documentation,
Doxyfile settings, paths/assets and commit metadata for private information.
All reviewed blobs are text; no images, recordings or other binary assets,
private school/Drive links, participant records, access codes or credentials
were found. Ordinary author names/email remain in Git attribution and become
public with history. Secret values were not copied into reports.

There were no pre-existing issues, PRs, releases or Actions artifacts; wiki
and discussions were disabled. All four completed Actions runs available at publication were downloaded
and reviewed locally, including the intentionally failing regression run;
Gitleaks found no leaks in those logs, the 14-commit reachable history or
the updated working tree. No Actions artifacts were present. Final source,
tests and documentation changes were also manually reviewed. No repository contents were
uploaded to a third-party scanning service. Scanners/manual review are not a
guarantee; unreachable server objects and external linked content are outside
this review. No project license was found; no license or authorship claim was
invented. Existing material and import provenance were retained.

Publication review passed on October 9. The explicitly requested GitHub CLI
visibility change succeeded; `gh repo view` returned PUBLIC with default
branch main. Fresh Python urllib requests with no Authorization header,
cookies, gh session or credential helper returned HTTP 200 for both repository
and development-branch web pages and API endpoints; repository API reported
`private: false`. Description identifies the development branch and physical
validation pending. The anonymous branch SHA at this check matched the tested
revision above. A later documentation-only commit contains this record; its
exact-head CI/remote verification is reported separately in the task delivery.

Public repository: https://github.com/jeff729/FEEED-Fall26

Current branch: https://github.com/jeff729/FEEED-Fall26/tree/fall26-development
Main remains the unchanged import/default branch. Only origin/fall26-development
is pushed; the original Ibrahimtourepe repository is not modified.
