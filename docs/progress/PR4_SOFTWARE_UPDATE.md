# PR4 software update

Reporting period: October 3-9, 2026. All work below is supported by October
6-9 commits; PR means the ENME401 progress report. Physical testing pending.

## Slide 1: Firmware improvements

- Retained earlier timed arm motion, checked geometry and locked cycle settings.
- Kept bounded current-based recovery and cancel-without-delivery behavior.
- Calibration uses LED feedback, a stopped plate and checked save readback.
- Fixed reset recovery during overload and paths crossing the inner workspace.
- Preserved the Uno, existing wiring, physical limits and four saved slots.

## Slide 2: Verification and next tests

- Normal and sanitizer runs pass 6,652 assertions; these are not independent cases.
- Default/lab Uno builds pass; default uses 20,100 bytes flash and 595 bytes SRAM.
- CI, tuning guides and bench checklist updated; 21 focused scenarios pass.
- Eight simulations finish; commanded cycles take 116-128 s, excluding waiting.
- Next: verify wiring, startup, cancel, empty-bowl clearance and controlled food trials.

Jeff contribution-table sentence: Prepared the firmware review, targeted fixes,
automated checks and bench documentation with AI assistance; physical testing
and human team review remain pending. No hours or hardware performance claimed.

Evidence: [earlier firmware](https://github.com/jeff729/FEEED-Fall26/compare/c9ea8ac...3fa818d),
[CI](https://github.com/jeff729/FEEED-Fall26/commit/ec2b71f),
[fixes](https://github.com/jeff729/FEEED-Fall26/commit/f06cc9a),
[regressions/timing](https://github.com/jeff729/FEEED-Fall26/commit/6cab55d),
[passing run](https://github.com/jeff729/FEEED-Fall26/actions/runs/37956093314),
[development branch](https://github.com/jeff729/FEEED-Fall26/tree/fall26-development).
