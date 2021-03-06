#pragma once

// clang-format off
#define DEG_FULL                     0x10000
#define DEG_1                        (DEG_FULL / 360) // = 182
#define DEG_45                       (DEG_FULL / 8) // = 8192 = 0x2000
#define DEG_90                       (DEG_FULL / 4) // = 16384 = 0x4000
#define DEG_180                      (DEG_FULL / 2) // = 32768 = 0x8000
#define DEG_270                      (DEG_90 * 3) // = 49152 = 0xC000
#define DEG_135                      (DEG_45 * 3) // = 24576 = 0x6000
#define DEG_360                      (DEG_FULL) // = 65536 = 0x10000

#define GAME_FOV                     80
#define WALL_L                       1024 // = 0x1000
#define W2V_SHIFT                    14
#define WALL_SHIFT                   10
#define STEP_L                       (WALL_L / 4) // = 256
#define NO_HEIGHT                    (-32512)
#define NO_BAD_POS                   32512
#define NO_BAD_NEG                   NO_HEIGHT
#define NO_ITEM                      (-1)
#define BAD_JUMP_CEILING             ((STEP_L * 3) / 4) // = 192

#define GRAVITY                      6
#define GRAVITY_SWAMP                2
#define FAST_FALL_SPEED              128

#define LARA_HIT_POINTS              1000
#define LARA_RADIUS                  100
#define LARA_RADIUS_UW               300
#define LARA_RADIUS_SURF             LARA_RADIUS
#define LARA_HEIGHT                  762
#define LARA_HEIGHT_UW               400
#define LARA_HANG_HEIGHT             600
#define LARA_DUCK_RADIUS             (LARA_RADIUS * 2) // = 200
#define LARA_CRAWL_RADIUS            (LARA_RADIUS * 2 + LARA_RADIUS / 2) // = 250
#define LARA_DUCK_HEIGHT             400

#define LARA_TURN_UNDO               (DEG_1 * 2) // = 364
#define LARA_TURN_RATE               ((DEG_1 / 4) + LARA_TURN_UNDO) // = 409
#define LARA_SLOW_TURN               ((DEG_1 * 2) + LARA_TURN_UNDO) // = 728
#define LARA_MED_TURN                ((DEG_1 * 4) + LARA_TURN_UNDO) // = 1092
#define LARA_FAST_TURN               ((DEG_1 * 6) + LARA_TURN_UNDO) // = 1456
#define LARA_MAX_HEAD_ROTATION       (DEG_1 * 44) // = 8008
#define LARA_MAX_HEAD_TILT           (DEG_1 * 30) // = 5460
#define LARA_MIN_HEAD_TILT           (DEG_1 * -35) // = -6370
#define LARA_HEAD_TURN               (DEG_1 * 2) // = 364

#define LARA_LEAN_UNDO               (DEG_1) // = 182
#define LARA_LEAN_RATE               ((DEG_1 / 2) + LARA_LEAN_UNDO) // = 273
#define LARA_LEAN_MAX                ((DEG_1 * 10) + LARA_LEAN_UNDO) // = 2002
#define LARA_LEAN_MAX_DASH           ((DEG_1 * 15) + LARA_LEAN_UNDO) // = 2912
#define LARA_LEAN_MAX_UW             (LARA_LEAN_MAX * 2) // = 4004

#define LARA_JUMP_TURN               (DEG_1 + LARA_TURN_UNDO) // = 546
#define LARA_FALL_DAMAGE_START       140
#define LARA_FALL_DAMAGE_LENGTH      14
#define LARA_FAST_FALL_SPEED         (FAST_FALL_SPEED + 3) // = 131
#define LARA_DASH_TIME               (30 * 4) // 4 seconds
#define LARA_STEP_UP_HEIGHT          ((STEP_L * 3) / 2) // = 384
#define LARA_CLIMB_WIDTH_R           120
#define LARA_CLIMB_WIDTH_L           120
#define LARA_CLIMB_HEIGHT            (STEP_L * 2) // = 512
#define LARA_CLIMB_SHIFT             70
#define LARA_CLIMB_HANG              900
#define LARA_VAULT_ANGLE             (DEG_1 * 30) // = 5460
#define LARA_SLOPE_DIF               60
#define LARA_DEF_ADD_EDGE            (DEG_1 * 5) // = 910
#define LARA_HANG_ANGLE              (DEG_1 * 35) // = 6370
#define LARA_DUCK_DEFLECT            (DEG_1 * 2) // = 364

#define CAMERA_HANG_ANGLE            0
#define CAMERA_HANG_ELEVATION        (DEG_1 * -60) // = -10920
#define CAMERA_SLIDE_ELEVATION       (DEG_1 * -45) // = -8190
#define CAMERA_REACH_ANGLE           (DEG_1 * 85) // = 15470
#define CAMERA_PUSHBLOCK_ANGLE       (DEG_1 * 35) // = 6370
#define CAMERA_PUSHBLOCK_ELEVATION   (DEG_1 * -25) // = -4550
#define CAMERA_PICKUP_ANGLE          (DEG_1 * -130) // = -23660
#define CAMERA_PICKUP_ELEVATION      (DEG_1 * -15) // = -2730
#define CAMERA_PICKUP_DISTANCE       WALL_L // = 1024
#define CAMERA_PICKUPFLARE_ANGLE     (DEG_1 * 130) // = 23660
#define CAMERA_PICKUPFLARE_ELEVATION (DEG_1 * -15) // = -2730
#define CAMERA_PICKUPFLARE_DISTANCE  WALL_L // = 1024
#define CAMERA_SWITCHON_ANGLE        (DEG_1 * 80) // = 14560
#define CAMERA_SWITCHON_ELEVATION    (DEG_1 * -25) // = -4550
#define CAMERA_SWITCHON_DISTANCE     WALL_L // = 1024
#define CAMERA_SWITCHON_SPEED        6
#define CAMERA_USEKEY_ANGLE          (DEG_1 * -80) // = -14560
#define CAMERA_USEKEY_ELEVATION      (DEG_1 * -25) // = -4550
#define CAMERA_USEKEY_DISTANCE       WALL_L // 1024
#define CAMERA_EXTRA_ANGLE           (DEG_1 * 170) // = 30940
#define CAMERA_EXTRA_ELEVATION       (DEG_1 * -25) // = -4550
#define CAMERA_WADE_ELEVATION        (DEG_1 * -22) // = -4004
#define CAMERA_PUSHPULL_ANGLE        (DEG_1 * 75) // = 13650
#define CAMERA_FOLLOW_CENTRE         1
#define CAMERA_NO_CHUNKY             2
#define CAMERA_BACKJUMP_ANGLE        (DEG_1 * 135) // = 24570

#define TEXT_MAX_STRINGS             64
#define TEXT_MAX_STRING_SIZE         64
// clang-format on
