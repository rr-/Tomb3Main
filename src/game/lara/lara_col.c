#include "game/lara/lara_col.h"

#include "game/lara/lara_misc.h"
#include "global/const.h"
#include "global/stubs.h"
#include "global/types.h"
#include "global/vars.h"
#include "util.h"

#include <stdbool.h>
#include <stdint.h>

static void Lara_CollideStop(struct ITEM_INFO *item, struct COLL_INFO *coll);
static void Lara_MonkeySwingSnap(
    struct ITEM_INFO *item, struct COLL_INFO *coll);
static bool Lara_TestMonkeyDirOctant(int32_t angle);
static bool Lara_TestMonkeyLeft(struct ITEM_INFO *item, struct COLL_INFO *coll);
static bool Lara_TestMonkeyRight(
    struct ITEM_INFO *item, struct COLL_INFO *coll);
static bool Lara_TestHangJumpUp(struct ITEM_INFO *item, struct COLL_INFO *coll);
static void Lara_Col_Jumper(struct ITEM_INFO *item, struct COLL_INFO *coll);
static void Lara_Col_ClimbLeftRight(
    struct ITEM_INFO *item, struct COLL_INFO *coll, int32_t result,
    int32_t shift);

static void Lara_CollideStop(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    switch (coll->old_anim_state) {
    case LS_STOP:
    case LS_TURN_L:
    case LS_TURN_R:
    case LS_FAST_TURN:
        item->current_anim_state = coll->old_anim_state;
        item->anim_num = coll->old_anim_num;
        item->frame_num = coll->old_frame_num;
        if (g_Input & IN_LEFT) {
            item->goal_anim_state = LS_TURN_L;
        } else if (g_Input & IN_RIGHT) {
            item->goal_anim_state = LS_TURN_R;
        } else {
            item->goal_anim_state = LS_STOP;
        }
        Lara_Animate(item);
        break;

    default:
        item->anim_num = LA_STOP;
        item->frame_num = g_Anims[LA_STOP].frame_base;
        break;
    }
}

static void Lara_MonkeySwingSnap(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    int16_t room_num = item->room_num;
    struct FLOOR_INFO *floor =
        GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
    item->pos.y =
        GetCeiling(floor, item->pos.x, item->pos.y, item->pos.z) + 704;
}

static bool Lara_TestMonkeyDirOctant(int32_t angle)
{
    angle = ABS(angle);
    return angle >= DEG_45 && angle <= DEG_135;
}

static bool Lara_TestMonkeyLeft(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    g_Lara.move_angle = item->pos.y_rot - DEG_90;
    coll->facing = g_Lara.move_angle;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = NO_BAD_NEG;
    coll->bad_ceiling = 0;
    coll->radius = LARA_RADIUS;
    coll->slopes_are_walls = 0;

    GetCollisionInfo(
        coll, item->pos.x, item->pos.y, item->pos.z, item->room_num,
        LARA_HANG_HEIGHT);

    if (ABS(coll->mid_ceiling - coll->front_ceiling) > 50) {
        return false;
    }

    bool oct = Lara_TestMonkeyDirOctant(item->pos.y_rot);
    switch (coll->coll_type) {
    case COLL_FRONT:
        return oct;
    case COLL_LEFT:
        return false;
    case COLL_RIGHT:
        return !oct;
    default:
        break;
    }
    return true;
}

static bool Lara_TestMonkeyRight(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    g_Lara.move_angle = item->pos.y_rot + DEG_90;
    coll->facing = g_Lara.move_angle;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -LARA_STEP_UP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->radius = LARA_RADIUS;
    coll->slopes_are_walls = 0;

    GetCollisionInfo(
        coll, item->pos.x, item->pos.y, item->pos.z, item->room_num,
        LARA_HANG_HEIGHT);

    if (ABS(coll->mid_ceiling - coll->front_ceiling) > 50) {
        return false;
    }

    bool oct = Lara_TestMonkeyDirOctant(item->pos.y_rot);
    switch (coll->coll_type) {
    case COLL_FRONT:
        return false;
    case COLL_LEFT:
        return !oct;
    case COLL_RIGHT:
        return !oct;
    default:
        break;
    }
    return true;
}

static bool Lara_TestHangJumpUp(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    if (!(g_Input & IN_ACTION) || g_Lara.gun_status != LGS_ARMLESS
        || coll->hit_static) {
        return false;
    }

    if (g_Lara.can_monkey_swing && coll->coll_type == COLL_TOP) {
        item->gravity_status = 0;
        item->fall_speed = 0;
        item->speed = 0;
        item->current_anim_state = LS_MONKEY_HANG;
        item->goal_anim_state = LS_MONKEY_HANG;
        item->anim_num = LA_UP_JUMP_GRAB;
        item->frame_num = g_Anims[LA_UP_JUMP_GRAB].frame_base;
        g_Lara.gun_status = LGS_HANDS_BUSY;
        Lara_MonkeySwingSnap(item, coll);
        return true;
    }

    if (coll->coll_type != COLL_FRONT
        || coll->mid_ceiling > -LARA_STEP_UP_HEIGHT) {
        return false;
    }

    int32_t edge;
    int32_t edge_catch = Lara_TestEdgeCatch(item, coll, &edge);
    if (!edge_catch
        || (edge_catch < 0 && !Lara_TestHangOnClimbWall(item, coll))) {
        return false;
    }

    PHD_ANGLE angle = Lara_SnapAngle(item->pos.y_rot, LARA_HANG_ANGLE);
    if (angle % DEG_90 != 0) {
        return false;
    }

    if (Lara_TestHangSwingIn(item, angle)) {
        item->current_anim_state = LS_MONKEY_HANG;
        item->goal_anim_state = LS_MONKEY_HANG;
        item->anim_num = LA_UP_JUMP_GRAB;
        item->frame_num = g_Anims[LA_UP_JUMP_GRAB].frame_base;
    } else {
        item->current_anim_state = LS_HANG;
        item->goal_anim_state = LS_HANG;
        item->anim_num = LA_GRAB_LEDGE;
        item->frame_num = g_Anims[LA_GRAB_LEDGE].frame_base + 12;
    }

    int16_t *bounds = GetBoundsAccurate(item);
    if (edge_catch > 0) {
        item->pos.y += coll->front_floor - bounds[2];
    } else {
        item->pos.y = edge - bounds[2];
    }

    item->gravity_status = 0;
    item->fall_speed = 0;
    item->speed = 0;
    item->pos.x += coll->shift.x;
    item->pos.z += coll->shift.z;
    item->pos.y_rot = angle;
    g_Lara.gun_status = LGS_HANDS_BUSY;
    g_Lara.torso_x_rot = 0;
    g_Lara.torso_y_rot = 0;
    return true;
}

static void Lara_Col_Jumper(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -LARA_STEP_UP_HEIGHT;
    coll->bad_ceiling = BAD_JUMP_CEILING;

    Lara_GetCollisionInfo(item, coll);
    Lara_DeflectEdgeJump(item, coll);

    if (item->fall_speed <= 0 || coll->mid_floor > 0) {
        return;
    }

    item->gravity_status = 0;
    item->fall_speed = 0;
    item->goal_anim_state = Lara_LandedBad(item, coll) ? LS_DEATH : LS_STOP;
    item->pos.y += coll->mid_floor;
}

static void Lara_Col_ClimbLeftRight(
    struct ITEM_INFO *item, struct COLL_INFO *coll, int32_t result,
    int32_t shift)
{
    switch (result) {
    case 1:
        if (g_Input & IN_LEFT) {
            item->goal_anim_state = LS_CLIMB_LEFT;
        } else if (g_Input & IN_RIGHT) {
            item->goal_anim_state = LS_CLIMB_RIGHT;
        } else {
            item->goal_anim_state = LS_CLIMB_STANCE;
        }
        item->pos.y += shift;
        break;

    case 0:
        item->pos.x = coll->old.x;
        item->pos.z = coll->old.z;
        item->goal_anim_state = LS_CLIMB_STANCE;
        item->current_anim_state = LS_CLIMB_STANCE;
        if (coll->old_anim_state != LS_CLIMB_STANCE) {
            item->anim_num = LA_CLIMB_STANCE;
            item->frame_num = g_Anims[item->anim_num].frame_base;
        } else {
            item->frame_num = coll->old_frame_num;
            item->anim_num = coll->old_anim_num;
            Lara_Animate(item);
        }
        break;

    default:
        item->goal_anim_state = LS_HANG;
        do {
            Item_Animate(item);
        } while (item->current_anim_state != LS_HANG);
        item->pos.x = coll->old.x;
        item->pos.z = coll->old.z;
        break;
    }
}

void Lara_Col_Duck(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    item->gravity_status = 0;
    item->fall_speed = 0;

    g_Lara.move_angle = item->pos.y_rot;
    coll->facing = g_Lara.move_angle;
    coll->radius = LARA_DUCK_RADIUS;
    coll->bad_pos = LARA_STEP_UP_HEIGHT;
    coll->bad_neg = -LARA_STEP_UP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->slopes_are_walls = 1;

    GetCollisionInfo(
        coll, item->pos.x, item->pos.y, item->pos.z, item->room_num,
        LARA_DUCK_HEIGHT);

    if (Lara_Fallen(item, coll)) {
        g_Lara.gun_status = LGS_ARMLESS;
        return;
    }

    g_Lara.keep_ducked = coll->mid_ceiling >= -(LARA_HEIGHT - LARA_DUCK_HEIGHT);

    ShiftItem(item, coll);
    item->pos.y += coll->mid_floor;

    if ((!(g_Input & IN_DUCK) || g_Lara.water_status == LWS_WADE)
        && !g_Lara.keep_ducked && item->anim_num == LA_DUCK_BREATHE) {
        item->goal_anim_state = LS_STOP;
    }
}

void Lara_Col_AllFours(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    item->gravity_status = 0;
    item->fall_speed = 0;

    if (item->goal_anim_state == LS_CRAWL_TO_HANG) {
        return;
    }

    g_Lara.move_angle = item->pos.y_rot;
    coll->facing = g_Lara.move_angle;
    coll->radius = LARA_DUCK_RADIUS;
    coll->bad_pos = +(STEP_L - 1);
    coll->bad_neg = -(STEP_L - 1);
    coll->bad_ceiling = LARA_DUCK_HEIGHT;
    coll->slopes_are_walls = 1;
    coll->slopes_are_pits = 1;

    GetCollisionInfo(
        coll, item->pos.x, item->pos.y, item->pos.z, item->room_num,
        LARA_DUCK_HEIGHT);

    if (Lara_Fallen(item, coll)) {
        g_Lara.gun_status = LGS_ARMLESS;
        return;
    }

    g_Lara.keep_ducked = coll->mid_ceiling >= -(LARA_HEIGHT - LARA_DUCK_HEIGHT);

    ShiftItem(item, coll);
    item->pos.y += coll->mid_floor;

    if ((!(g_Input & IN_DUCK) && !g_Lara.keep_ducked) || (g_Input & IN_DRAW)) {
        item->goal_anim_state = LS_DUCK;
        return;
    }

    if (item->anim_num != LA_ALL_FOURS && item->anim_num != LA_ALL_FOURS_2) {
        return;
    }

    if (g_Input & IN_FORWARD) {
        int16_t height = Lara_FloorFront(item, item->pos.y_rot, STEP_L);
        if (height < +(STEP_L - 1) && height > -(STEP_L - 1)
            && g_HeightType != HT_BIG_SLOPE) {
            item->goal_anim_state = LS_CRAWL;
        }
    } else if (g_Input & IN_BACK) {
        int16_t height = Lara_CeilingFront(item, item->pos.y_rot, -300);
        if (height == NO_HEIGHT || height > STEP_L) {
            return;
        }

        height = Lara_FloorFront(item, item->pos.y_rot, -300);
        if (height < +(STEP_L - 1) && height > -(STEP_L - 1)
            && g_HeightType != HT_BIG_SLOPE) {
            item->goal_anim_state = LS_CRAWL_BACK;
        } else if (
            g_Input & IN_ACTION && height > 768
            && !GetStaticObjects(
                item, item->pos.y_rot + DEG_180, 512, 50, 300)) {

            enum DIRECTION dir = Lara_AngleToDirection(item->pos.y_rot);
            switch (dir) {
            case DIR_NORTH:
                item->pos.z = (item->pos.z & ~(WALL_L - 1)) + 225;
                item->pos.y_rot = 0;
                break;
            case DIR_EAST:
                item->pos.x = (item->pos.x & ~(WALL_L - 1)) + 225;
                item->pos.y_rot = DEG_90;
                break;
            case DIR_SOUTH:
                item->pos.z = (item->pos.z | (WALL_L - 1)) - 225;
                item->pos.y_rot = DEG_180;
                break;
            case DIR_WEST:
                item->pos.x = (item->pos.x | (WALL_L - 1)) - 225;
                item->pos.y_rot = DEG_270;
                break;
            }
            item->goal_anim_state = LS_CRAWL_TO_HANG;
        }
    } else if (g_Input & IN_LEFT) {
        item->current_anim_state = LS_ALL_FOURS_TURN_L;
        item->goal_anim_state = LS_ALL_FOURS_TURN_L;
        item->anim_num = LA_ALL_FOURS_TURN_L;
        item->frame_num = g_Anims[LA_ALL_FOURS_TURN_L].frame_base;
    } else if (g_Input & IN_RIGHT) {
        item->current_anim_state = LS_ALL_FOURS_TURN_R;
        item->goal_anim_state = LS_ALL_FOURS_TURN_R;
        item->anim_num = LA_ALL_FOURS_TURN_R;
        item->frame_num = g_Anims[LA_ALL_FOURS_TURN_R].frame_base;
    }
}

void Lara_Col_AllFoursTurnLR(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    GetCollisionInfo(
        coll, item->pos.x, item->pos.y, item->pos.z, item->room_num,
        LARA_DUCK_HEIGHT);
}

void Lara_Col_Crawl(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    item->gravity_status = 0;
    item->fall_speed = 0;

    g_Lara.move_angle = item->pos.y_rot;
    coll->facing = g_Lara.move_angle;
    coll->radius = LARA_DUCK_RADIUS;
    coll->bad_pos = +(STEP_L - 1);
    coll->bad_neg = -(STEP_L - 1);
    coll->bad_ceiling = LARA_DUCK_HEIGHT;
    coll->slopes_are_walls = 1;
    coll->slopes_are_pits = 1;

    GetCollisionInfo(
        coll, item->pos.x, item->pos.y, item->pos.z, item->room_num,
        LARA_DUCK_HEIGHT);

    if (Lara_DeflectEdgeDuck(item, coll)) {
        item->current_anim_state = LS_ALL_FOURS;
        item->goal_anim_state = LS_ALL_FOURS;
        if (item->anim_num != LA_ALL_FOURS) {
            item->anim_num = LA_ALL_FOURS;
            item->frame_num = g_Anims[LA_ALL_FOURS].frame_base;
        }
    } else if (Lara_Fallen(item, coll)) {
        g_Lara.gun_status = LGS_ARMLESS;
    } else {
        ShiftItem(item, coll);
        item->pos.y += coll->mid_floor;
    }
}

void Lara_Col_CrawlB(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    item->fall_speed = 0;
    item->gravity_status = 0;

    g_Lara.move_angle = item->pos.y_rot + DEG_180;
    coll->facing = g_Lara.move_angle;
    coll->radius = LARA_CRAWL_RADIUS;
    coll->bad_pos = STEP_L - 1;
    coll->bad_neg = -(STEP_L - 1);
    coll->bad_ceiling = LARA_DUCK_HEIGHT;
    coll->slopes_are_walls = 1;
    coll->slopes_are_pits = 1;

    GetCollisionInfo(
        coll, item->pos.x, item->pos.y, item->pos.z, item->room_num,
        LARA_DUCK_HEIGHT);

    if (Lara_DeflectEdgeDuck(item, coll)) {
        item->current_anim_state = LS_ALL_FOURS;
        item->goal_anim_state = LS_ALL_FOURS;
        if (item->anim_num != LA_ALL_FOURS) {
            item->anim_num = LA_ALL_FOURS;
            item->frame_num = g_Anims[LA_ALL_FOURS].frame_base;
        }
    } else if (Lara_Fallen(item, coll)) {
        g_Lara.gun_status = LGS_ARMLESS;
    } else {
        ShiftItem(item, coll);
        item->pos.y += coll->mid_floor;
        g_Lara.move_angle = item->pos.y_rot;
    }
}

void Lara_Col_Crawl2Hang(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    if (item->anim_num != 302) {
        return;
    }

    item->fall_speed = 512;
    item->pos.y |= (STEP_L - 1);

    g_Lara.move_angle = item->pos.y_rot;
    coll->facing = g_Lara.move_angle;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -LARA_STEP_UP_HEIGHT;
    coll->bad_ceiling = BAD_JUMP_CEILING;

    GetCollisionInfo(
        coll, item->pos.x, item->pos.y, item->pos.z, item->room_num, 870);

    int32_t edge;
    int32_t edge_catch = Lara_TestEdgeCatch(item, coll, &edge);
    if (!edge_catch
        || (edge_catch < 0 && !Lara_TestHangOnClimbWall(item, coll))) {
        return;
    }

    PHD_ANGLE angle = Lara_SnapAngle(item->pos.y_rot, LARA_HANG_ANGLE);
    if (angle % DEG_90 != 0) {
        return;
    }

    if (Lara_TestHangSwingIn(item, angle)) {
        g_Lara.head_x_rot = 0;
        g_Lara.head_y_rot = 0;
        g_Lara.torso_x_rot = 0;
        g_Lara.torso_y_rot = 0;
        item->current_anim_state = LS_MONKEY_HANG;
        item->goal_anim_state = LS_MONKEY_HANG;
        item->anim_num = LA_GRAB_LEDGE_IN;
        item->frame_num = g_Anims[LA_GRAB_LEDGE_IN].frame_base;
    } else {
        item->current_anim_state = LS_HANG;
        item->goal_anim_state = LS_HANG;
        item->anim_num = LA_GRAB_LEDGE;
        item->frame_num = g_Anims[LA_GRAB_LEDGE].frame_base;
    }

    int16_t *bounds = GetBoundsAccurate(item);
    if (edge_catch > 0) {
        item->pos.y += coll->front_floor - bounds[2];
        item->pos.x += coll->shift.x;
        item->pos.z += coll->shift.z;
    } else {
        item->pos.y = edge - bounds[2];
    }

    item->pos.y_rot = angle;
    item->gravity_status = 1;
    item->speed = 2;
    item->fall_speed = 1;
    g_Lara.gun_status = LGS_HANDS_BUSY;
}

void Lara_Col_Dash(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    g_Lara.move_angle = item->pos.y_rot;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -LARA_STEP_UP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->slopes_are_walls = 1;

    Lara_GetCollisionInfo(item, coll);

    if (Lara_HitCeiling(item, coll)) {
        return;
    }
    if (Lara_TestVault(item, coll)) {
        return;
    }

    if (Lara_DeflectEdge(item, coll)) {
        item->pos.z_rot = 0;
        if (Lara_TestWall(item, STEP_L, 0, -640)) {
            item->current_anim_state = LS_SPLAT;
            item->anim_num = LA_HIT_WALL_L;
            item->frame_num = g_Anims[LA_HIT_WALL_L].frame_base;
            return;
        }
        Lara_CollideStop(item, coll);
    }

    if (Lara_Fallen(item, coll)) {
        return;
    }

    if (coll->mid_floor >= -LARA_STEP_UP_HEIGHT
        && coll->mid_floor < -STEP_L / 2) {
        if (item->frame_num >= 3 && item->frame_num <= 14) {
            item->anim_num = LA_RUN_STEP_UP_L;
            item->frame_num = g_Anims[LA_RUN_STEP_UP_L].frame_base;
        } else {
            item->anim_num = LA_RUN_STEP_UP_R;
            item->frame_num = g_Anims[LA_RUN_STEP_UP_R].frame_base;
        }
    }

    if (Lara_TestSlide(item, coll)) {
        return;
    }

    if (coll->mid_floor >= 50) {
        item->pos.y += 50;
    } else {
        item->pos.y += coll->mid_floor;
    }
}

void Lara_Col_DashDive(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    if (item->speed >= 0) {
        g_Lara.move_angle = item->pos.y_rot;
    } else {
        g_Lara.move_angle = item->pos.y_rot + DEG_180;
    }

    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEP_L;
    coll->bad_ceiling = BAD_JUMP_CEILING;
    coll->slopes_are_walls = 1;

    Lara_GetCollisionInfo(item, coll);
    Lara_DeflectEdgeJump(item, coll);

    if (Lara_Fallen(item, coll)) {
        return;
    }

    if (item->speed < 0) {
        g_Lara.move_angle = item->pos.y_rot;
    }

    if (coll->mid_floor <= 0 && item->fall_speed > 0) {
        if (Lara_LandedBad(item, coll)) {
            item->goal_anim_state = LS_DEATH;
        } else if (g_Lara.water_status == LWS_WADE) {
            item->goal_anim_state = LS_STOP;
        } else if (g_Input & IN_FORWARD && !(g_Input & IN_SLOW)) {
            item->goal_anim_state = LS_RUN;
        } else {
            item->goal_anim_state = LS_STOP;
        }
        item->fall_speed = 0;
        item->pos.y += coll->mid_floor;
        item->gravity_status = 0;
        item->speed = 0;
        Lara_Animate(item);
    }

    ShiftItem(item, coll);
    item->pos.y += coll->mid_floor;
}

void Lara_Col_MonkeySwingHang(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    if (g_Lara.can_monkey_swing) {
        g_Lara.move_angle = item->pos.y_rot;
        coll->facing = g_Lara.move_angle;
        coll->bad_pos = NO_BAD_POS;
        coll->bad_neg = NO_BAD_NEG;
        coll->bad_ceiling = 0;
        coll->radius = LARA_RADIUS;
        coll->slopes_are_walls = 0;

        GetCollisionInfo(
            coll, item->pos.x, item->pos.y, item->pos.z, item->room_num,
            LARA_HANG_HEIGHT);

        if (g_Input & IN_FORWARD && coll->coll_type != COLL_FRONT
            && ABS(coll->mid_ceiling - coll->front_ceiling) < 50) {
            item->goal_anim_state = LS_MONKEY_SWING;
        } else if (g_Input & IN_STEP_L && Lara_TestMonkeyLeft(item, coll)) {
            item->goal_anim_state = LS_MONKEY_L;
        } else if (g_Input & IN_STEP_R && Lara_TestMonkeyRight(item, coll)) {
            item->goal_anim_state = LS_MONKEY_R;
        } else if (g_Input & IN_LEFT) {
            item->goal_anim_state = LS_HANG_TURN_L;
        } else if (g_Input & IN_RIGHT) {
            item->goal_anim_state = LS_HANG_TURN_R;
        }

        Lara_MonkeySwingSnap(item, coll);
        return;
    }

    Lara_TestHang(item, coll);
    if (item->goal_anim_state != LS_MONKEY_HANG) {
        return;
    }

    if (g_Input & IN_FORWARD && coll->front_floor > -850
        && coll->front_floor < -650
        && coll->front_floor - coll->front_ceiling >= 0
        && coll->left_floor2 - coll->left_ceiling2 >= 0
        && coll->right_floor2 - coll->right_ceiling2 >= 0
        && !coll->hit_static) {
        item->goal_anim_state = g_Input & IN_SLOW ? LS_GYMNAST : LS_NULL;
        return;
    }

    if (((g_Input & IN_DUCK) || (g_Input & IN_FORWARD))
        && coll->front_floor > -850 && coll->front_floor < -650
        && coll->front_floor - coll->front_ceiling >= -STEP_L
        && coll->left_floor2 - coll->left_ceiling2 >= -STEP_L
        && coll->right_floor2 - coll->right_ceiling2 >= -STEP_L
        && !coll->hit_static) {
        item->goal_anim_state = LS_HANG_TO_DUCK;
        item->required_anim_state = LS_DUCK;
        return;
    }

    if (g_Input & IN_LEFT || g_Input & IN_STEP_L) {
        item->goal_anim_state = LS_HANG_LEFT;
    } else if (g_Input & IN_RIGHT || g_Input & IN_STEP_R) {
        item->goal_anim_state = LS_HANG_RIGHT;
    }
}

void Lara_Col_MonkeySwing(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    if (!(g_Input & IN_ACTION) || !g_Lara.can_monkey_swing) {
        Lara_MonkeySwingFall(item);
        return;
    }

    g_Lara.move_angle = item->pos.y_rot;
    coll->facing = g_Lara.move_angle;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = NO_BAD_NEG;
    coll->bad_ceiling = 0;
    coll->radius = LARA_RADIUS;
    coll->slopes_are_walls = 0;

    GetCollisionInfo(
        coll, item->pos.x, item->pos.y, item->pos.z, item->room_num,
        LARA_HANG_HEIGHT);

    if (coll->coll_type == COLL_FRONT
        || ABS(coll->mid_ceiling - coll->front_ceiling) > 50) {
        item->current_anim_state = LS_MONKEY_HANG;
        item->goal_anim_state = LS_MONKEY_HANG;
        item->anim_num = LA_MONKEY_HANG;
        item->frame_num = g_Anims[LA_MONKEY_HANG].frame_base;
        return;
    }

    g_Camera.target_elevation = 10 * DEG_1;
    Lara_MonkeySwingSnap(item, coll);
}

void Lara_Col_MonkeyLeft(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    if (!(g_Input & IN_ACTION) || !g_Lara.can_monkey_swing) {
        Lara_MonkeySwingFall(item);
        return;
    }

    if (Lara_TestMonkeyLeft(item, coll)) {
        Lara_MonkeySwingSnap(item, coll);
    } else {
        item->current_anim_state = LS_MONKEY_HANG;
        item->goal_anim_state = LS_MONKEY_HANG;
        item->anim_num = LA_MONKEY_HANG;
        item->frame_num = g_Anims[LA_MONKEY_HANG].frame_base;
    }
}

void Lara_Col_MonkeyRight(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    if (!(g_Input & IN_ACTION) || !g_Lara.can_monkey_swing) {
        Lara_MonkeySwingFall(item);
        return;
    }

    if (Lara_TestMonkeyRight(item, coll)) {
        Lara_MonkeySwingSnap(item, coll);
    } else {
        item->current_anim_state = LS_MONKEY_HANG;
        item->goal_anim_state = LS_MONKEY_HANG;
        item->anim_num = LA_MONKEY_HANG;
        item->frame_num = g_Anims[LA_MONKEY_HANG].frame_base;
    }
}

void Lara_Col_HangTurnLR(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    if (!(g_Input & IN_ACTION) || !g_Lara.can_monkey_swing) {
        Lara_MonkeySwingFall(item);
        return;
    }

    g_Lara.move_angle = item->pos.y_rot;
    coll->facing = g_Lara.move_angle;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -LARA_STEP_UP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->radius = LARA_RADIUS;
    coll->slopes_are_walls = 1;

    GetCollisionInfo(
        coll, item->pos.x, item->pos.y, item->pos.z, item->room_num,
        LARA_HANG_HEIGHT);

    Lara_MonkeySwingSnap(item, coll);
}

void Lara_Col_Monkey180(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    Lara_Col_MonkeySwing(item, coll);
}

void Lara_Col_Stop(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    item->gravity_status = 0;
    item->fall_speed = 0;

    g_Lara.move_angle = item->pos.y_rot;
    coll->bad_pos = LARA_STEP_UP_HEIGHT;
    coll->bad_neg = -LARA_STEP_UP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->slopes_are_pits = 1;
    coll->slopes_are_walls = 1;

    Lara_GetCollisionInfo(item, coll);

    if (Lara_HitCeiling(item, coll) || Lara_Fallen(item, coll)
        || Lara_TestSlide(item, coll)) {
        return;
    }

    ShiftItem(item, coll);

    if (!(g_Rooms[item->room_num].flags & RF_SWAMP)) {
        item->pos.y += coll->mid_floor;
    } else if (coll->mid_floor < 0) {
        item->pos.y += coll->mid_floor;
    } else if (coll->mid_floor > 0) {
        item->pos.y += GRAVITY_SWAMP;
    }
}

void Lara_Col_JumpUp(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    if (item->hit_points <= 0) {
        item->goal_anim_state = LS_STOP;
        return;
    }

    g_Lara.move_angle = item->pos.y_rot;
    coll->facing = g_Lara.move_angle + (item->speed < 0 ? DEG_180 : 0);
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -LARA_STEP_UP_HEIGHT;
    coll->bad_ceiling = BAD_JUMP_CEILING;
    coll->hit_ceiling = 0;

    GetCollisionInfo(
        coll, item->pos.x, item->pos.y, item->pos.z, item->room_num, 870);

    if (Lara_TestHangJumpUp(item, coll)) {
        return;
    }

    ShiftItem(item, coll);
    if (coll->coll_type == COLL_CLAMP || coll->coll_type == COLL_TOP
        || coll->coll_type == COLL_TOP_FRONT || coll->hit_ceiling) {
        item->fall_speed = 1;
    }

    if (coll->coll_type != COLL_NONE) {
        item->speed = item->speed > 0 ? 2 : -2;
    } else if (item->fall_speed < -70) {
        if (g_Input & IN_FORWARD && item->speed < 5) {
            item->speed++;
        } else if (g_Input & IN_BACK && item->speed > -5) {
            item->speed -= 2;
        }
    }

    if (item->fall_speed > 0 && coll->mid_floor <= 0) {
        item->goal_anim_state = Lara_LandedBad(item, coll) ? LS_DEATH : LS_STOP;
        item->gravity_status = 0;
        item->fall_speed = 0;
        item->pos.y += coll->mid_floor;
    }
}

void Lara_Col_Walk(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    item->gravity_status = 0;
    item->fall_speed = 0;

    g_Lara.move_angle = item->pos.y_rot;
    coll->bad_pos = LARA_STEP_UP_HEIGHT;
    coll->bad_neg = -LARA_STEP_UP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->slopes_are_walls = 1;
    coll->slopes_are_pits = 1;
    coll->lava_is_pit = 1;
    Lara_GetCollisionInfo(item, coll);

    if (Lara_HitCeiling(item, coll) || Lara_TestVault(item, coll)) {
        return;
    }

    if (Lara_DeflectEdge(item, coll)) {
        if (item->frame_num >= 22 && item->frame_num <= 28) {
            item->anim_num = LA_STOP_L;
            item->frame_num = g_Anims[LA_STOP_L].frame_base;
        } else if (item->frame_num >= 29 && item->frame_num <= 47) {
            item->anim_num = LA_STOP_R;
            item->frame_num = g_Anims[LA_STOP_R].frame_base;
        } else if (item->frame_num >= 48 && item->frame_num <= 57) {
            item->anim_num = LA_STOP_L;
            item->frame_num = g_Anims[LA_STOP_L].frame_base;
        } else {
            Lara_CollideStop(item, coll);
        }
    }

    if (Lara_Fallen(item, coll)) {
        return;
    }

    if (coll->mid_floor > STEP_L / 2) {
        if (item->frame_num >= 28 && item->frame_num <= 45) {
            item->anim_num = LA_WALK_STEP_DOWN_R;
            item->frame_num = g_Anims[LA_WALK_STEP_DOWN_R].frame_base;
        } else {
            item->anim_num = LA_WALK_STEP_DOWN_L;
            item->frame_num = g_Anims[LA_WALK_STEP_DOWN_L].frame_base;
        }
    }

    if (coll->mid_floor >= -LARA_STEP_UP_HEIGHT
        && coll->mid_floor < -STEP_L / 2) {
        if (item->frame_num >= 27 && item->frame_num <= 44) {
            item->anim_num = LA_WALK_STEP_UP_R;
            item->frame_num = g_Anims[LA_WALK_STEP_UP_R].frame_base;
        } else {
            item->anim_num = LA_WALK_STEP_UP_L;
            item->frame_num = g_Anims[LA_WALK_STEP_UP_L].frame_base;
        }
    }

    if (Lara_TestSlide(item, coll)) {
        return;
    }

    item->pos.y += coll->mid_floor;
}

void Lara_Col_Run(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    g_Lara.move_angle = item->pos.y_rot;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -LARA_STEP_UP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->slopes_are_walls = 1;
    Lara_GetCollisionInfo(item, coll);

    if (Lara_HitCeiling(item, coll) || Lara_TestVault(item, coll)) {
        return;
    }

    if (Lara_DeflectEdge(item, coll)) {
        item->pos.z_rot = 0;
        if (item->anim_num != LA_START_RUN
            && Lara_TestWall(item, STEP_L, 0, -640)) {
            item->current_anim_state = LS_SPLAT;
            if (item->frame_num >= 0 && item->frame_num <= 9) {
                item->anim_num = LA_HIT_WALL_L;
                item->frame_num = g_Anims[LA_HIT_WALL_L].frame_base;
                return;
            }
            if (item->frame_num >= 10 && item->frame_num <= 21) {
                item->anim_num = LA_HIT_WALL_R;
                item->frame_num = g_Anims[LA_HIT_WALL_R].frame_base;
                return;
            }
        }
        Lara_CollideStop(item, coll);
    }

    if (Lara_Fallen(item, coll)) {
        return;
    }

    if (coll->mid_floor >= -LARA_STEP_UP_HEIGHT
        && coll->mid_floor < -STEP_L / 2) {
        if (item->frame_num >= 3 && item->frame_num <= 14) {
            item->anim_num = LA_RUN_STEP_UP_L;
            item->frame_num = g_Anims[LA_RUN_STEP_UP_L].frame_base;
        } else {
            item->anim_num = LA_RUN_STEP_UP_R;
            item->frame_num = g_Anims[LA_RUN_STEP_UP_R].frame_base;
        }
    }

    if (Lara_TestSlide(item, coll)) {
        return;
    }

    if (coll->mid_floor >= 50) {
        item->pos.y += 50;
    } else {
        item->pos.y += coll->mid_floor;
    }
}

void Lara_Col_JumpForward(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    g_Lara.move_angle = item->pos.y_rot + (item->speed < 0 ? DEG_180 : 0);
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -LARA_STEP_UP_HEIGHT;
    coll->bad_ceiling = BAD_JUMP_CEILING;

    Lara_GetCollisionInfo(item, coll);
    Lara_DeflectEdgeJump(item, coll);

    if (item->speed < 0) {
        g_Lara.move_angle = item->pos.y_rot;
    }

    if (coll->mid_floor > 0 || item->fall_speed <= 0) {
        return;
    }

    if (Lara_LandedBad(item, coll)) {
        item->goal_anim_state = LS_DEATH;
    } else if (g_Lara.water_status == LWS_WADE) {
        item->goal_anim_state = LS_STOP;
    } else if (g_Input & IN_FORWARD && !(g_Input & IN_SLOW)) {
        item->goal_anim_state = LS_RUN;
    } else {
        item->goal_anim_state = LS_STOP;
    }

    item->gravity_status = 0;
    item->fall_speed = 0;
    item->pos.y += coll->mid_floor;
    item->speed = 0;
    Lara_Animate(item);
}

void Lara_Col_FastBack(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    item->gravity_status = 0;
    item->fall_speed = 0;

    g_Lara.move_angle = item->pos.y_rot + DEG_180;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -LARA_STEP_UP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->slopes_are_pits = 1;
    coll->slopes_are_walls = 0;
    Lara_GetCollisionInfo(item, coll);

    if (Lara_HitCeiling(item, coll)) {
        return;
    }

    if (coll->mid_floor > 200) {
        item->current_anim_state = LS_FALL_BACK;
        item->goal_anim_state = LS_FALL_BACK;
        item->anim_num = LA_FALL_BACK;
        item->frame_num = g_Anims[LA_FALL_BACK].frame_base;
        item->gravity_status = 1;
        item->fall_speed = 0;
        return;
    }

    if (Lara_TestSlide(item, coll)) {
        return;
    }

    if (Lara_DeflectEdge(item, coll)) {
        Lara_CollideStop(item, coll);
    }

    item->pos.y += coll->mid_floor;
}

void Lara_Col_TurnRight(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    item->gravity_status = 0;
    item->fall_speed = 0;

    g_Lara.move_angle = item->pos.y_rot;
    coll->bad_pos = LARA_STEP_UP_HEIGHT;
    coll->bad_neg = -LARA_STEP_UP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->slopes_are_pits = 1;
    coll->slopes_are_walls = 1;

    Lara_GetCollisionInfo(item, coll);

    if (coll->mid_floor > 100 && !(g_Rooms[item->room_num].flags & RF_SWAMP)) {
        item->gravity_status = 1;
        item->fall_speed = 0;
        item->current_anim_state = LS_FORWARD_JUMP;
        item->goal_anim_state = LS_FORWARD_JUMP;
        item->anim_num = LA_FALL_DOWN;
        item->frame_num = g_Anims[LA_FALL_DOWN].frame_base;
        return;
    }

    if (Lara_TestSlide(item, coll)) {
        return;
    }

    if (!(g_Rooms[item->room_num].flags & RF_SWAMP)) {
        item->pos.y += coll->mid_floor;
    } else if (coll->mid_floor < 0) {
        item->pos.y += coll->mid_floor;
    } else if (coll->mid_floor > 0) {
        item->pos.y += GRAVITY_SWAMP;
    }
}

void Lara_Col_TurnLeft(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    Lara_Col_TurnRight(item, coll);
}

void Lara_Col_Death(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    Sound_StopEffect(SFX_LARA_FALL);
    g_Lara.move_angle = item->pos.y_rot;
    coll->bad_pos = LARA_STEP_UP_HEIGHT;
    coll->bad_neg = -LARA_STEP_UP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->radius = LARA_RADIUS * 4;

    Lara_GetCollisionInfo(item, coll);

    ShiftItem(item, coll);
    item->pos.y += coll->mid_floor;
    item->hit_points = -1;
    g_Lara.air = -1;
}

void Lara_Col_FastFall(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    item->gravity_status = 1;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -LARA_STEP_UP_HEIGHT;
    coll->bad_ceiling = BAD_JUMP_CEILING;

    Lara_GetCollisionInfo(item, coll);
    Lara_SlideEdgeJump(item, coll);

    if (coll->mid_floor > 0) {
        return;
    }

    if (Lara_LandedBad(item, coll)) {
        item->goal_anim_state = LS_DEATH;
    } else {
        item->current_anim_state = LS_STOP;
        item->goal_anim_state = LS_STOP;
        item->anim_num = LA_LAND_FAR;
        item->frame_num = g_Anims[LA_LAND_FAR].frame_base;
    }

    Sound_StopEffect(SFX_LARA_FALL);
    item->gravity_status = 0;
    item->fall_speed = 0;
    item->pos.y += coll->mid_floor;
}

void Lara_Col_Hang(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    Lara_TestHang(item, coll);

    if (item->goal_anim_state != LS_HANG) {
        return;
    }

    if (g_Input & IN_FORWARD) {
        if (coll->front_floor > -850 && coll->front_floor < -650
            && coll->front_floor - coll->front_ceiling >= 0
            && coll->left_floor2 - coll->left_ceiling2 >= 0
            && coll->right_floor2 - coll->right_ceiling2 >= 0
            && !coll->hit_static) {
            item->goal_anim_state = g_Input & IN_SLOW ? LS_GYMNAST : LS_NULL;
            return;
        } else if (
            g_Lara.climb_status && item->anim_num == LA_GRAB_LEDGE
            && item->frame_num == g_Anims[LA_GRAB_LEDGE].frame_base + 21
            && coll->mid_ceiling <= -STEP_L) {
            item->current_anim_state = LS_HANG;
            item->goal_anim_state = LS_HANG;
            item->anim_num = LA_HANG_UP;
            item->frame_num = g_Anims[LA_HANG_UP].frame_base;
            return;
        }
    }

    if (((g_Input & IN_DUCK) || (g_Input & IN_FORWARD))
        && coll->front_floor > -850 && coll->front_floor < -650
        && coll->front_floor - coll->front_ceiling >= -STEP_L
        && coll->left_floor2 - coll->left_ceiling2 >= -STEP_L
        && coll->right_floor2 - coll->right_ceiling2 >= -STEP_L
        && !coll->hit_static) {
        item->goal_anim_state = LS_HANG_TO_DUCK;
        item->required_anim_state = LS_DUCK;
    } else if (
        (g_Input & IN_BACK) && g_Lara.climb_status
        && item->anim_num == LA_GRAB_LEDGE
        && item->frame_num == g_Anims[LA_GRAB_LEDGE].frame_base + 21) {
        item->current_anim_state = LS_HANG;
        item->goal_anim_state = LS_HANG;
        item->anim_num = LA_HANG_DOWN;
        item->frame_num = g_Anims[LA_HANG_DOWN].frame_base;
    }
}

void Lara_Col_Reach(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    item->gravity_status = 1;

    g_Lara.move_angle = item->pos.y_rot;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = 0;
    coll->bad_ceiling = BAD_JUMP_CEILING;

    Lara_GetCollisionInfo(item, coll);
    if (Lara_TestHangJump(item, coll)) {
        return;
    }

    Lara_SlideEdgeJump(item, coll);
    Lara_GetCollisionInfo(item, coll);
    ShiftItem(item, coll);

    if (item->fall_speed <= 0 || coll->mid_floor > 0) {
        return;
    }

    item->gravity_status = 0;
    item->fall_speed = 0;
    item->goal_anim_state = Lara_LandedBad(item, coll) ? LS_DEATH : LS_STOP;
    item->pos.y += coll->mid_floor;
}

void Lara_Col_Splat(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    g_Lara.move_angle = item->pos.y_rot;
    coll->bad_pos = LARA_STEP_UP_HEIGHT;
    coll->bad_neg = -LARA_STEP_UP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->slopes_are_pits = 1;
    coll->slopes_are_walls = 1;

    Lara_GetCollisionInfo(item, coll);
    ShiftItem(item, coll);

    if (coll->mid_floor > -STEP_L && coll->mid_floor < STEP_L) {
        item->pos.y += coll->mid_floor;
    }
}

void Lara_Col_Land(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    Lara_Col_Stop(item, coll);
}

void Lara_Col_Compress(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    item->gravity_status = 0;
    item->fall_speed = 0;

    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = NO_BAD_NEG;
    coll->bad_ceiling = 0;

    Lara_GetCollisionInfo(item, coll);

    if (coll->mid_ceiling > -100) {
        item->gravity_status = 0;
        item->fall_speed = 0;
        item->speed = 0;
        item->pos.x = coll->old.x;
        item->pos.y = coll->old.y;
        item->pos.z = coll->old.z;
        item->current_anim_state = LS_STOP;
        item->goal_anim_state = LS_STOP;
        item->anim_num = LA_STOP;
        item->frame_num = g_Anims[LA_STOP].frame_base;
    }

    if (coll->mid_floor > -STEP_L && coll->mid_floor < STEP_L) {
        item->pos.y += coll->mid_floor;
    }
}

void Lara_Col_Back(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    item->gravity_status = 0;
    item->fall_speed = 0;

    g_Lara.move_angle = item->pos.y_rot + DEG_180;
    coll->bad_pos =
        g_Lara.water_status == LWS_WADE ? NO_BAD_POS : LARA_STEP_UP_HEIGHT;
    coll->bad_neg = -LARA_STEP_UP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->slopes_are_pits = 1;
    coll->slopes_are_walls = 1;

    Lara_GetCollisionInfo(item, coll);
    if (Lara_HitCeiling(item, coll)) {
        return;
    }

    if (Lara_DeflectEdge(item, coll)) {
        Lara_CollideStop(item, coll);
    }

    if (Lara_Fallen(item, coll)) {
        return;
    }

    if (coll->mid_floor > STEP_L / 2 && coll->mid_floor < LARA_STEP_UP_HEIGHT) {
        if (item->frame_num >= 964 && item->frame_num <= 993) {
            item->anim_num = LA_BACK_STEP_DOWN_R;
            item->frame_num = g_Anims[LA_BACK_STEP_DOWN_R].frame_base;
        } else {
            item->anim_num = LA_BACK_STEP_DOWN_L;
            item->frame_num = g_Anims[LA_BACK_STEP_DOWN_L].frame_base;
        }
    }

    if (Lara_TestSlide(item, coll)) {
        return;
    }

    if (!(g_Rooms[item->room_num].flags & RF_SWAMP)) {
        item->pos.y += coll->mid_floor;
    } else if (coll->mid_floor < 0) {
        item->pos.y += coll->mid_floor;
    } else if (coll->mid_floor > 0) {
        item->pos.y += GRAVITY_SWAMP;
    }
}

void Lara_Col_StepRight(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    item->gravity_status = 0;
    item->fall_speed = 0;

    g_Lara.move_angle = item->pos.y_rot
        + (item->current_anim_state == LS_STEP_RIGHT ? DEG_90 : -DEG_90);
    coll->bad_pos = g_Lara.water_status == LWS_WADE ? NO_BAD_POS : STEP_L / 2;
    coll->bad_neg = -STEP_L / 2;
    coll->bad_ceiling = 0;
    coll->slopes_are_pits = 1;
    coll->slopes_are_walls = 1;

    Lara_GetCollisionInfo(item, coll);
    if (Lara_HitCeiling(item, coll)) {
        return;
    }

    if (Lara_DeflectEdge(item, coll)) {
        Lara_CollideStop(item, coll);
    }
    if (Lara_Fallen(item, coll)) {
        return;
    }
    if (Lara_TestSlide(item, coll)) {
        return;
    }

    item->pos.y += coll->mid_floor;
}

void Lara_Col_StepLeft(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    Lara_Col_StepRight(item, coll);
}

void Lara_Col_Slide(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    g_Lara.move_angle = item->pos.y_rot;
    Lara_SlideSlope(item, coll);
}

void Lara_Col_JumpBack(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    g_Lara.move_angle = item->pos.y_rot + DEG_180;
    Lara_Col_Jumper(item, coll);
}

void Lara_Col_JumpRight(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    g_Lara.move_angle = item->pos.y_rot + DEG_90;
    Lara_Col_Jumper(item, coll);
}

void Lara_Col_JumpLeft(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    g_Lara.move_angle = item->pos.y_rot - DEG_90;
    Lara_Col_Jumper(item, coll);
}

void Lara_Col_FallBack(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    g_Lara.move_angle = item->pos.y_rot + DEG_180;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -LARA_STEP_UP_HEIGHT;
    coll->bad_ceiling = BAD_JUMP_CEILING;

    Lara_GetCollisionInfo(item, coll);
    Lara_DeflectEdgeJump(item, coll);

    if (coll->mid_floor > 0 || item->fall_speed <= 0) {
        return;
    }

    item->gravity_status = 0;
    item->fall_speed = 0;
    item->goal_anim_state = Lara_LandedBad(item, coll) ? LS_DEATH : LS_STOP;
    item->pos.y += coll->mid_floor;
}

void Lara_Col_HangLeft(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    g_Lara.move_angle = item->pos.y_rot - DEG_90;
    Lara_TestHang(item, coll);
    g_Lara.move_angle = item->pos.y_rot - DEG_90;
}

void Lara_Col_HangRight(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    g_Lara.move_angle = item->pos.y_rot + DEG_90;
    Lara_TestHang(item, coll);
    g_Lara.move_angle = item->pos.y_rot + DEG_90;
}

void Lara_Col_SlideBack(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    g_Lara.move_angle = item->pos.y_rot + DEG_180;
    Lara_SlideSlope(item, coll);
}

void Lara_Col_Null(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    Lara_Col_Default(item, coll);
}

void Lara_Col_Roll(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    item->gravity_status = 0;
    item->fall_speed = 0;

    g_Lara.move_angle = item->pos.y_rot;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -LARA_STEP_UP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->slopes_are_walls = 1;

    Lara_GetCollisionInfo(item, coll);
    if (Lara_HitCeiling(item, coll)) {
        return;
    }

    if (Lara_TestSlide(item, coll)) {
        return;
    }
    if (Lara_Fallen(item, coll)) {
        return;
    }

    ShiftItem(item, coll);
    item->pos.y += coll->mid_floor;
}

void Lara_Col_Roll2(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    item->gravity_status = 0;
    item->fall_speed = 0;

    g_Lara.move_angle = item->pos.y_rot + DEG_180;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -LARA_STEP_UP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->slopes_are_walls = 1;

    Lara_GetCollisionInfo(item, coll);
    if (Lara_HitCeiling(item, coll)) {
        return;
    }

    if (Lara_TestSlide(item, coll)) {
        return;
    }

    if (coll->mid_floor > 200) {
        item->gravity_status = 1;
        item->fall_speed = 0;
        item->current_anim_state = LS_FALL_BACK;
        item->goal_anim_state = LS_FALL_BACK;
        item->anim_num = LA_FALL_BACK;
        item->frame_num = g_Anims[LA_FALL_BACK].frame_base;
    } else {
        ShiftItem(item, coll);
        item->pos.y += coll->mid_floor;
    }
}

void Lara_Col_SwanDive(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    g_Lara.move_angle = item->pos.y_rot;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -LARA_STEP_UP_HEIGHT;
    coll->bad_ceiling = BAD_JUMP_CEILING;

    Lara_GetCollisionInfo(item, coll);
    Lara_DeflectEdgeJump(item, coll);

    if (coll->mid_floor > 0 || item->fall_speed <= 0) {
        return;
    }

    item->gravity_status = 0;
    item->fall_speed = 0;
    item->goal_anim_state = LS_STOP;
    item->pos.y += coll->mid_floor;
}

void Lara_Col_FastDive(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    g_Lara.move_angle = item->pos.y_rot;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -LARA_STEP_UP_HEIGHT;
    coll->bad_ceiling = BAD_JUMP_CEILING;

    Lara_GetCollisionInfo(item, coll);
    Lara_DeflectEdgeJump(item, coll);

    if (coll->mid_floor > 0 || item->fall_speed <= 0) {
        return;
    }

    item->gravity_status = 0;
    item->fall_speed = 0;
    item->goal_anim_state = item->fall_speed > 133 ? LS_DEATH : LS_STOP;
    item->pos.y += coll->mid_floor;
}

void Lara_Col_Wade(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    g_Lara.move_angle = item->pos.y_rot;
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -LARA_STEP_UP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->slopes_are_walls = 1;

    Lara_GetCollisionInfo(item, coll);
    if (Lara_HitCeiling(item, coll)) {
        return;
    }

    if (Lara_TestVault(item, coll)) {
        return;
    }

    if (Lara_DeflectEdge(item, coll)) {
        item->pos.z_rot = 0;
        if ((coll->front_type == HT_WALL || coll->front_type == HT_SPLIT_TRI)
            && coll->front_floor < -(STEP_L * 5) / 2
            && !(g_Rooms[item->room_num].flags & RF_SWAMP)) {
            item->current_anim_state = LS_SPLAT;
            if (item->frame_num >= 0 && item->frame_num <= 9) {
                item->anim_num = LA_HIT_WALL_L;
                item->frame_num = g_Anims[LA_HIT_WALL_L].frame_base;
                return;
            }
            if (item->frame_num >= 10 && item->frame_num <= 21) {
                item->anim_num = LA_HIT_WALL_R;
                item->frame_num = g_Anims[LA_HIT_WALL_R].frame_base;
                return;
            }
        }
        Lara_CollideStop(item, coll);
    }

    if (coll->mid_floor >= -LARA_STEP_UP_HEIGHT && coll->mid_floor < -STEP_L / 2
        && !(g_Rooms[item->room_num].flags & RF_SWAMP)) {
        if (item->frame_num >= 3 && item->frame_num <= 14) {
            item->anim_num = LA_RUN_STEP_UP_L;
            item->frame_num = g_Anims[LA_RUN_STEP_UP_L].frame_base;
        } else {
            item->anim_num = LA_RUN_STEP_UP_R;
            item->frame_num = g_Anims[LA_RUN_STEP_UP_R].frame_base;
        }
    }

    if (!(g_Rooms[item->room_num].flags & RF_SWAMP)) {
        if (coll->mid_floor >= 50) {
            item->pos.y += 50;
        } else {
            item->pos.y += coll->mid_floor;
        }
    } else if (coll->mid_floor < 0) {
        item->pos.y += coll->mid_floor;
    } else if (coll->mid_floor > 0) {
        item->pos.y += GRAVITY_SWAMP;
    }
}

void Lara_Col_ClimbLeft(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    if (Lara_TestLetGo(item, coll)) {
        return;
    }

    g_Lara.move_angle = item->pos.y_rot - DEG_90;

    int32_t shift;
    int32_t result = Lara_TestClimbPos(
        item, coll->radius, -coll->radius - LARA_CLIMB_WIDTH_L,
        -LARA_CLIMB_HEIGHT, LARA_CLIMB_HEIGHT, &shift);

    Lara_Col_ClimbLeftRight(item, coll, result, shift);
}

void Lara_Col_ClimbRight(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    if (Lara_TestLetGo(item, coll)) {
        return;
    }

    g_Lara.move_angle = item->pos.y_rot + DEG_90;

    int32_t shift;
    int32_t result = Lara_TestClimbPos(
        item, coll->radius, coll->radius + LARA_CLIMB_WIDTH_R,
        -LARA_CLIMB_HEIGHT, LARA_CLIMB_HEIGHT, &shift);

    Lara_Col_ClimbLeftRight(item, coll, result, shift);
}

void Lara_Col_ClimbStance(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    if (Lara_TestLetGo(item, coll)) {
        return;
    }

    if (item->anim_num != LA_CLIMB_STANCE) {
        return;
    }

    if (g_Input & IN_FORWARD) {
        if (item->goal_anim_state == LS_NULL) {
            return;
        }

        item->goal_anim_state = LS_CLIMB_STANCE;

        int32_t shift_l;
        int32_t shift_r;
        int32_t ledge_l;
        int32_t ledge_r;
        int32_t result_r = Lara_TestClimbUpPos(
            item, coll->radius, coll->radius + LARA_CLIMB_WIDTH_R, &shift_r,
            &ledge_r);
        int32_t result_l = Lara_TestClimbUpPos(
            item, coll->radius, -coll->radius - LARA_CLIMB_WIDTH_L, &shift_l,
            &ledge_l);

        if (!result_r || !result_l) {
            return;
        }

        if (result_r < 0 || result_l < 0) {
            if (ABS(ledge_r - ledge_l) > LARA_CLIMB_WIDTH_R) {
                return;
            }

            item->pos.y += (ledge_l + ledge_r) / 2 - STEP_L;
            item->goal_anim_state = LS_NULL;
            return;
        }

        int32_t shift = shift_l;
        if (shift_r) {
            if (shift_l) {
                if ((shift_l < 0) != (shift_r < 0)) {
                    return;
                }
                if (shift_r < 0 && shift_r < shift_l) {
                    shift = shift_r;
                } else if (shift_r > 0 && shift_r > shift_l) {
                    shift = shift_r;
                }
            } else {
                shift = shift_r;
            }
        }

        item->goal_anim_state = LS_CLIMBING;
        item->pos.y += shift;
    } else if (g_Input & IN_BACK) {
        if (item->goal_anim_state == LS_HANG) {
            return;
        }

        item->goal_anim_state = LS_CLIMB_STANCE;

        item->pos.y += STEP_L;
        int32_t shift_l;
        int32_t shift_r;
        int32_t result_r = Lara_TestClimbPos(
            item, coll->radius, coll->radius + LARA_CLIMB_WIDTH_R,
            -LARA_CLIMB_HEIGHT, LARA_CLIMB_HEIGHT, &shift_r);
        int32_t result_l = Lara_TestClimbPos(
            item, coll->radius, -coll->radius - LARA_CLIMB_WIDTH_L,
            -LARA_CLIMB_HEIGHT, LARA_CLIMB_HEIGHT, &shift_l);
        item->pos.y -= STEP_L;

        if (!result_r || !result_l) {
            return;
        }

        int32_t shift = shift_l;
        if (shift_r && shift_l) {
            if ((shift_l < 0) != (shift_r < 0)) {
                return;
            }
            if (shift_r < 0 && shift_r < shift_l) {
                shift = shift_r;
            } else if (shift_r > 0 && shift_r > shift_l) {
                shift = shift_r;
            }
        }

        if (result_r == 1 && result_l == 1) {
            item->goal_anim_state = LS_CLIMB_DOWN;
            item->pos.y += shift;
        } else {
            item->goal_anim_state = LS_HANG;
        }
    }
}

void Lara_Col_Climbing(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    if (Lara_TestLetGo(item, coll)) {
        return;
    }

    if (item->anim_num != LA_CLIMBING) {
        return;
    }

    int32_t shift;
    int32_t frame = item->frame_num - g_Anims[LA_CLIMBING].frame_base;
    if (frame == 0) {
        shift = 0;
    } else if (frame == 28 || frame == 29) {
        shift = -STEP_L;
    } else if (frame == 57) {
        shift = -STEP_L * 2;
    } else {
        return;
    }

    item->pos.y += shift - STEP_L;
    int32_t shift_l;
    int32_t shift_r;
    int32_t ledge_l;
    int32_t ledge_r;
    int32_t result_r = Lara_TestClimbUpPos(
        item, coll->radius, coll->radius + LARA_CLIMB_WIDTH_R, &shift_r,
        &ledge_r);
    int32_t result_l = Lara_TestClimbUpPos(
        item, coll->radius, -coll->radius - LARA_CLIMB_WIDTH_L, &shift_l,
        &ledge_l);
    item->pos.y += STEP_L;

    if (!result_r || !result_l || !(g_Input & IN_FORWARD)) {
        item->goal_anim_state = LS_CLIMB_STANCE;
        if (shift) {
            Lara_Animate(item);
        }
        return;
    }

    if (result_r < 0 || result_l < 0) {
        item->goal_anim_state = LS_CLIMB_STANCE;
        Lara_Animate(item);
        if (ABS(ledge_l - ledge_r) <= LARA_CLIMB_WIDTH_L) {
            item->goal_anim_state = LS_NULL;
            item->pos.y += (ledge_l + ledge_r) / 2 - STEP_L;
        }
    } else {
        item->goal_anim_state = LS_CLIMBING;
        item->pos.y -= shift;
    }
}

void Lara_Col_ClimbDown(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    if (Lara_TestLetGo(item, coll)) {
        return;
    }

    if (item->anim_num != LA_CLIMB_DOWN) {
        return;
    }

    int32_t shift;
    int32_t frame = item->frame_num - g_Anims[LA_CLIMB_DOWN].frame_base;
    if (frame == 0) {
        shift = 0;
    } else if (frame == 28 || frame == 29) {
        shift = STEP_L;
    } else if (frame == 57) {
        shift = STEP_L * 2;
    } else {
        return;
    }

    item->pos.y += shift + STEP_L;
    int32_t shift_l;
    int32_t shift_r;
    int32_t result_r = Lara_TestClimbPos(
        item, coll->radius, coll->radius + LARA_CLIMB_WIDTH_R,
        -LARA_CLIMB_HEIGHT, LARA_CLIMB_HEIGHT, &shift_r);
    int32_t result_l = Lara_TestClimbPos(
        item, coll->radius, -coll->radius - LARA_CLIMB_WIDTH_L,
        -LARA_CLIMB_HEIGHT, LARA_CLIMB_HEIGHT, &shift_l);
    item->pos.y -= STEP_L;

    if (!result_r || !result_l || !(g_Input & IN_BACK)) {
        item->goal_anim_state = LS_CLIMB_STANCE;
        if (shift) {
            Lara_Animate(item);
        }
        return;
    }

    if (shift_r && shift_l) {
        if ((shift_l < 0) != (shift_r < 0)) {
            item->goal_anim_state = LS_CLIMB_STANCE;
            Lara_Animate(item);
            return;
        }
    }

    if (result_r == -1 || result_l == -1) {
        item->current_anim_state = LS_CLIMB_STANCE;
        item->anim_num = LA_CLIMB_STANCE;
        item->frame_num = g_Anims[item->anim_num].frame_base;
        item->goal_anim_state = LS_HANG;
        Lara_Animate(item);
    } else {
        item->goal_anim_state = LS_CLIMB_DOWN;
        item->pos.y -= shift;
    }
}

void Lara_Col_Default(struct ITEM_INFO *item, struct COLL_INFO *coll)
{
    g_Lara.move_angle = item->pos.y_rot;
    coll->bad_pos = LARA_STEP_UP_HEIGHT;
    coll->bad_neg = -LARA_STEP_UP_HEIGHT;
    coll->bad_ceiling = 0;
    coll->slopes_are_pits = 1;
    coll->slopes_are_walls = 1;

    Lara_GetCollisionInfo(item, coll);
}
