/******************************************************************************

                  ��Ȩ���� (C), 2001-2011, ��Ϊ�������޹�˾

 ******************************************************************************
  �� �� ��   : TafApsFsmMainTbl.h
  �� �� ��   : ����
  ��    ��   : h44270
  ��������   : 2011��12��16��
  ����޸�   :
  ��������   : TafApsFsmMainTbl.C ��ͷ�ļ�
  �����б�   :
  �޸���ʷ   :
  1.��    ��   : 2011��12��16��
    ��    ��   : h44270
    �޸�����   : �����ļ�

******************************************************************************/

#ifndef _TAF_APS_FSM_MAIN_TBL_H
#define _TAF_APS_FSM_MAIN_TBL_H

/*****************************************************************************
  1 ����ͷ�ļ�����
*****************************************************************************/
#include "vos.h"
#include "TafFsm.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#pragma pack(4)

/*****************************************************************************
  2 �궨��
*****************************************************************************/

/*****************************************************************************
  3 ö�ٶ���
*****************************************************************************/
/*****************************************************************************
 ö����    : TAF_APS_STA_ENUM_UINT32
 ö��˵��  : ��״̬״̬ö�ٶ���
 1.��    ��   : 2011��12��15��
   ��    ��   : h44270
   �޸�����   : �½�

*****************************************************************************/
enum TAF_APS_STA_ENUM
{
    TAF_APS_STA_INACTIVE,                                                       /* δ����̬ */
    TAF_APS_STA_MS_ACTIVATING,                                                  /* ���ڼ����� */
    TAF_APS_STA_NW_ACTIVATING,                                                  /* ���缤��ֶ�Ӧ����*/
    TAF_APS_STA_ACTIVE,                                                         /* ����̬ */
    TAF_APS_STA_MS_MODIFYING,                                                   /* �޸���  */
    TAF_APS_STA_MS_DEACTIVATING,                                                /* ȥ������ */
    TAF_APS_STA_PREPROC,                                                        /* Ԥ����״̬ */

    TAF_APS_STA_CDATA_ESTING,
    TAF_APS_STA_CDATA_DISCING,
    TAF_APS_STA_DORMANT,

    TAF_APS_STA_CDATA_HANDOFF,

    TAF_APS_STA_BUTT                  = 0xFF
} ;
typedef VOS_UINT32 TAF_APS_STA_ENUM_UINT32;

/*****************************************************************************
  4 ȫ�ֱ�������
*****************************************************************************/


/*****************************************************************************
  5 ��Ϣͷ����
*****************************************************************************/


/*****************************************************************************
  6 ��Ϣ����
*****************************************************************************/


/*****************************************************************************
  7 STRUCT����
*****************************************************************************/

/*****************************************************************************
  8 UNION����
*****************************************************************************/


/*****************************************************************************
  9 OTHERS����
*****************************************************************************/


/*****************************************************************************
  10 ��������
*****************************************************************************/
VOS_UINT32 TAF_APS_GetMainFsmStaTblSize( VOS_VOID );
TAF_FSM_DESC_STRU * TAF_APS_GetMainFsmDescAddr( VOS_VOID );


#if (VOS_OS_VER == VOS_WIN32)
#pragma pack()
#else
#pragma pack(0)
#endif




#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of _NAS_MMC_FSM_MAIN.h */
