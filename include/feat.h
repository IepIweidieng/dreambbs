/*-------------------------------------------------------*/
/* feat.h       ( NCKU CCNS WindTop-DreamBBS 3.21.1 )    */
/*-------------------------------------------------------*/
/* Author: ???                                           */
/* Target: BBS feature flags / non-configurable settings */
/* Create: 95/03/29 (as "config.h")                      */
/* Update: 2021-01-15 (split from "config.h")            */
/*       : by Wei-Cheng Yeh (IID) <iid@ccns.ncku.edu.tw> */
/*-------------------------------------------------------*/

#ifndef FEAT_H
#define FEAT_H

/* ----------------------------------------------------- */
/* Modules & Plug-in                       by cache      */
/* ------------------------------------------------------*/

#define USE_M3_MENU             /* 開啟 M3 選單介面 */

#ifdef  M3_USE_PMORE            /* Enable pmore */
#define USE_PMORE
#endif

#ifdef  M3_USE_PFTERM           /* Enable pfterm */
#define USE_PFTERM
#endif

#define MultiRecommend          /* 顯示多重推文 */

#define GRAYOUT                 /* 淡入淡出特效系統 */

#ifdef  M3_USE_BBSLUA           /* Enable BBS-Lua */
#define USE_BBSLUA
#endif

#ifdef  M3_USE_BBSRUBY          /* Enable BBS-Ruby */
#define USE_BBSRUBY
#endif

/* ----------------------------------------------------- */
/* 組態規劃                                              */
/* ------------------------------------------------------*/

#define EMAIL_JUSTIFY                  /* 發出 InterNet Email 身份認證信函 */

#define HAVE_BM_CHECK           /* 板主確認 */

#endif /* #ifndef FEAT_H */
