#pragma once

// Lara state routines.

#include "global/types.h"

void Lara_State_Duck(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_AllFours(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_AllFoursTurnLeft(
    struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_AllFoursTurnRight(
    struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_Crawl(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_CrawlB(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_Dash(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_DashDive(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_MonkeyHang(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_MonkeySwing(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_MonkeyLeft(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_MonkeyRight(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_HangTurnLeft(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_HangTurnRight(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_Monkey180(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_Stop(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_JumpUp(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_ForwardJump(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_Walk(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_Run(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_FastBack(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_TurnRight(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_TurnLeft(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_Death(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_FastFall(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_Hang(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_Reach(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_Splat(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_Compress(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_Back(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_FastTurn(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_StepRight(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_StepLeft(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_Slide(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_BackJump(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_RightJump(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_LeftJump(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_FallBack(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_HangLeft(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_HangRight(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_SlideBack(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_PushBlock(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_PushPullReady(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_Pickup(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_PickupFlare(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_SwitchOn(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_UseKey(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_Special(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_SwanDive(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_FastDive(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_Null(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_WaterOut(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_Wade(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_DeathSlide(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_ClimbLeft(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_ClimbRight(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_ClimbStance(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_Climbing(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_ClimbEnd(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_State_ClimbDown(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_StateExtra_Breath(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_StateExtra_SharkKill(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_StateExtra_AirLock(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_StateExtra_GongBong(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_StateExtra_TRexDeath(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_StateExtra_StartAnim(struct ITEM_INFO *item, struct COLL_INFO *coll);
void Lara_StateExtra_TrainKill(struct ITEM_INFO *item, struct COLL_INFO *col);
