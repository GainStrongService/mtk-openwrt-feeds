
/******************************************************************************

    Copyright 2022 Maxlinear

    SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0-only)

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.
******************************************************************************/

/**
   \file cmds_fapi.c
    Implements CLI commands for lif mdio

*/
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "os_types.h"

#include "cmds.h"
#include "cmds_fapi.h"
#include "host_adapt.h"
#include "fapi_gsw_hostapi.h"
#include "fapi_gsw_hostapi_mdio_relay.h"

#define slif_lib ""

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */
static void cmds_fapi_help (void);


OS_boolean_t cmds_fapi (CmdArgs_t* pArgs, int *err)
{
    OS_boolean_t  api_executed; 
    int32_t ret;
 
    if (pArgs == NULL)
    {
        *err = OS_ERROR;
        return OS_TRUE; 
    }

    ret = OS_SUCCESS;        
    api_executed = OS_TRUE;

   /******************************************
   *  lif CLI cmds                          *
   ******************************************/   
      
   if ((strcmp(pArgs->name, "cmds-fapi-help")  == 0 ) || (strcmp(pArgs->name, "cmds-gsw-?")  == 0))
   {
     cmds_fapi_help ();
   }

   /***************************************
    * gsw_API:                       	*
    *   - api-gsw-internal-read    	   *
    *   - api-gsw-internal-write      	*
    *   - api-gsw-get-links           	*
    *   - api-gsw-read         	      *
    *   - api-gsw-write         	      *
    * ************************************/

   else if (strcmp(pArgs->name, "fapi-int-gphy-write") == 0)
   {

      char* slib ="";
      uint16_t reg = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;
      if (pArgs->prmc < 4)
      { 
         printf ("Usage: fapi-int-gphy-write phy=<> mmd=<> reg=<reg> data=<>\n");
         printf ("phy: phy id\n");
         printf ("mmd: mmd addres\n");
         printf ("reg: mdio register\n");
         printf ("data: value to write\n");
         goto goto_end_help;
      }

      slib      = slif_lib;

      api_gsw_get_links(slib);
      ret = fapi_int_gphy_write(pArgs->prmc, pArgs->prmvs);
   }

      else if (strcmp(pArgs->name, "fapi-int-gphy-read") == 0)
   {

      char* slib ="";
      uint16_t reg = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;
      if (pArgs->prmc < 3)
      { 
         printf ("Usage: fapi-int-gphy-read phy=<> mmd=<> reg=<reg>\n");
         printf ("phy: phy id\n");
         printf ("mmd: mmd addres\n");
         printf ("reg: mdio register\n");
         goto goto_end_help;
      }

      slib      = slif_lib;

      api_gsw_get_links(slib);
      ret = fapi_int_gphy_read(pArgs->prmc, pArgs->prmvs);
   }

   else if (strcmp(pArgs->name, "fapi-ext-mdio-write") == 0)
   {

      char* slib ="";
      uint16_t reg = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;
      if (pArgs->prmc < 4)
      { 
         printf ("Usage: fapi-ext-mdio-write phy=<> mmd=<> reg=<reg> data=<>\n");
         printf ("phy: phy address\n");
         printf ("mmd: mmd addres\n");
         printf ("reg: mdio register\n");
         printf ("data: value to write\n");
         goto goto_end_help;
      }

      slib      = slif_lib;

      api_gsw_get_links(slib);
      ret = fapi_ext_mdio_write(pArgs->prmc, pArgs->prmvs);
   }

      else if (strcmp(pArgs->name, "fapi-ext-mdio-read") == 0)
   {

      char* slib ="";
      uint16_t reg = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;
      if (pArgs->prmc < 3)
      { 
         printf ("Usage: fapi-ext-mdio-read phy=<> mmd=<> reg=<reg>\n");
         printf ("phy: phy address\n");
         printf ("mmd: mmd addres\n");
         printf ("reg: mdio register\n");
         goto goto_end_help;
      }

      slib      = slif_lib;

      api_gsw_get_links(slib);
      ret = fapi_ext_mdio_read(pArgs->prmc, pArgs->prmvs);
   }

   else if (strcmp(pArgs->name, "fapi-ext-mdio-mod") == 0)
   {

      char* slib ="";
      uint16_t reg = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;
      if (pArgs->prmc < 4)
      { 
         printf ("Usage: fapi-ext-mdio-write phy=<> mmd=<> reg=<reg> mask=<> data=<>\n");
         printf ("phy: phy address\n");
         printf ("mmd: mmd addres\n");
         printf ("reg: mdio register\n");
         printf ("mask: mask\n");
         printf ("data: data to write\n");
         goto goto_end_help;
      }

      slib      = slif_lib;

      api_gsw_get_links(slib);
      ret = fapi_ext_mdio_mod(pArgs->prmc, pArgs->prmvs);
   }

      else if (strcmp(pArgs->name, "fapi-int-gphy-mod") == 0)
   {

      char* slib ="";
      uint16_t reg = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;
      if (pArgs->prmc < 3)
      { 
         printf ("Usage: fapi-ext-mdio-read phy=<> mmd=<> reg=<reg> mask=<> data=<>\n");
         printf ("phy: phy id\n");
         printf ("mmd: mmd addres\n");
         printf ("reg: mdio register\n");
         printf ("mask: mask\n");
         printf ("data: data to write\n");
         
         goto goto_end_help;
      }

      slib      = slif_lib;

      api_gsw_get_links(slib);
      ret = fapi_int_gphy_mod(pArgs->prmc, pArgs->prmvs);
   }


   else if (strcmp(pArgs->name, "fapi-GSW-RegisterGet") == 0)
   {

      char* slib ="";
      uint16_t reg = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;
      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-RegisterGet nRegAddr=<reg>\n");
         printf ("reg: register\n");
         goto goto_end_help;
      }

      slib      = slif_lib;

      api_gsw_get_links(slib);
      ret = fapi_GSW_RegisterGet(pArgs->prmc, pArgs->prmvs);
   }

   else if (strcmp(pArgs->name, "fapi-GSW-RegisterSet") == 0)
   {

      char* slib ="";
      uint16_t reg = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;
      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-RegisterSet nRegAddr=<reg> nData=<data>\n");
         printf ("reg: register\n");
         printf ("data: data to write\n");
         goto goto_end_help;
      }

      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_RegisterSet(pArgs->prmc, pArgs->prmvs);
   }

   else if (strcmp(pArgs->name, "fapi-GSW-RegisterMod") == 0)
   {

      char* slib ="";
      uint16_t reg = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;
      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-RegisterMod nRegAddr=<reg> nData=<data> nMask=<mask>\n");
         printf ("nRegAddr: register\n");
         printf ("nData: data to write\n");
         printf ("nMask: mask\n");
         goto goto_end_help;
      }

      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_RegisterMod(pArgs->prmc, pArgs->prmvs);
   }

   else if (strcmp(pArgs->name, "fapi-GSW-PortLinkCfgGet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;
      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-PortLinkCfgGet nPortId=<port>\n");
         printf ("port: port index <1-8>\n");
         goto goto_end_help;
      }

      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_PortLinkCfgGet(pArgs->prmc, pArgs->prmvs);

   }
   else if (strcmp(pArgs->name, "fapi-GSW-PortLinkCfgSet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;
      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-PortLinkCfgSet nPortId=<port> [bDuplexForce=<> eDuplex=<> bSpeedForce=<> eSpeed=<> bLinkForce=<> eLink=<> eMII_Mode=<> eMII_Type=<> eClkMode=<> bLPI=<>]\n");
         printf ("port: port index <1-8>\n");
         goto goto_end_help;
      }

      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_PortLinkCfgSet(pArgs->prmc, pArgs->prmvs);

   }

   else if (strcmp(pArgs->name, "fapi-GSW-RMON-Clear") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;
      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-RMON-Clear nRmonId=<ID> eRmonType=<TYPE>\n");
         printf ("ID: RMON Counters Identifier\n");
         printf ("TYPE: RMON Counters Type\n");
         goto goto_end_help;
      }

      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_RMON_Clear(pArgs->prmc, pArgs->prmvs);

   }

   else if (strcmp(pArgs->name, "fapi-GSW-MonitorPortCfgGet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;
    

      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_MonitorPortCfgGet(pArgs->prmc, pArgs->prmvs);

   }
   else if (strcmp(pArgs->name, "fapi-GSW-MonitorPortCfgSet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;
      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-MonitorPortCfgSet nPortId=<port> nSubIfId=<ID>\n");
         printf ("port: port index <1-8>\n");
         printf ("ID: Monitoring Sub-IF id\n");
         goto goto_end_help;
      }

      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_MonitorPortCfgSet(pArgs->prmc, pArgs->prmvs);

   }



   else if (strcmp(pArgs->name, "fapi-GSW-QoS-PortCfgGet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;
      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-QoS-PortCfgGet nPortId=<port>\n");
         printf ("port: port index <1-8>\n");
         goto goto_end_help;
      }

      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_QoS_PortCfgGet(pArgs->prmc, pArgs->prmvs);

   }
   else if (strcmp(pArgs->name, "fapi-GSW-QoS-PortCfgSet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;
      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-QoS-PortCfgSet nPortId=<port> eClassMode=<CLASS> nTrafficClass=<TR>\n");
         printf ("port: port index <1-8>\n");
         printf ("eClassMode: Select the packet header field\n");
         printf ("nTrafficClass: Default port priority\n");
         goto goto_end_help;
      }

      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_QoS_PortCfgSet(pArgs->prmc, pArgs->prmvs);

   }
   

   else if (strcmp(pArgs->name, "fapi-GSW-QoS-DSCP-ClassGet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_QoS_DSCP_ClassGet(pArgs->prmc, pArgs->prmvs);

   }
   else if (strcmp(pArgs->name, "fapi-GSW-QoS-DSCP-ClassSet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;
      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-QoS-DSCP-ClassSet nTrafficClass=<TC> nDSCP=<DSCP>\n");
         printf ("nTrafficClass: Configures the DSCP to traffic class mapping\n");
         printf ("nDSCP: DSCP to drop precedence assignment\n");
         goto goto_end_help;
      }

      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_QoS_DSCP_ClassSet(pArgs->prmc, pArgs->prmvs);

   }

   else if (strcmp(pArgs->name, "fapi-GSW-QoS-PCP-ClassGet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_QoS_PCP_ClassGet(pArgs->prmc, pArgs->prmvs);

   }
   else if (strcmp(pArgs->name, "fapi-GSW-QoS-PCP-ClassSet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;
      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-QoS-PCP-ClassSet nTrafficClass=<TC> nPCP=<priority>\n");
         printf ("nTrafficClass: Configures the PCP to traffic class mapping\n");
         printf ("nPCP: priority to drop precedence assignment\n");
         goto goto_end_help;
      }

      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_QoS_PCP_ClassSet(pArgs->prmc, pArgs->prmvs);
   }

   else if (strcmp(pArgs->name, "fapi-GSW-QoS-SVLAN-PCP-ClassGet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_QoS_SVLAN_PCP_ClassGet(pArgs->prmc, pArgs->prmvs);

   }
   else if (strcmp(pArgs->name, "fapi-GSW-QoS-SVLAN-PCP-ClassSet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;
      if (pArgs->prmc < 1)
      {
         printf ("Usage: fapi-GSW-QoS-SVLAN-PCP-ClassSet nTrafficClass=<TC> nPCP=<priority>\n");
         printf ("nTrafficClass: Configures the SVLAN PCP to traffic class mapping\n");
         printf ("nPCP: priority to drop precedence assignment\n");
         goto goto_end_help;
      }

      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_QoS_SVLAN_PCP_ClassSet(pArgs->prmc, pArgs->prmvs);
   }

   else if (strcmp(pArgs->name, "fapi-GSW-QoS-ShaperCfgGet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-QoS-ShaperCfgGet nRateShaperId=<Id>\n");
         printf ("id:  Rate shaper index\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_QoS_ShaperCfgGet(pArgs->prmc, pArgs->prmvs);

   }
   else if (strcmp(pArgs->name, "fapi-GSW-QoS-ShaperCfgSet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;
      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-QoS-ShaperCfgSet nRateShaperId=<Id> bEnable=<En> nCbs=<CB> nRate=<Rt>\n");
         printf ("id:  Rate shaper index\n");
         printf ("En:  Enable/Disable the rate shaper\n");
         printf ("CB:  Committed Burst Size\n");
         printf ("Rt:  Rate [kbit/s]\n");
         goto goto_end_help;
      }

      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_QoS_ShaperCfgSet(pArgs->prmc, pArgs->prmvs);
   }

   else if (strcmp(pArgs->name, "fapi-GSW-QoS-ShaperQueueGet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-QoS-ShaperQueueGet nQueueId=<Id>\n");
         printf ("id:  Rate shaper index\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_QoS_ShaperQueueGet(pArgs->prmc, pArgs->prmvs);

   }
   else if (strcmp(pArgs->name, "fapi-GSW-QoS-ShaperQueueAssign") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-QoS-ShaperQueueAssign nQueueId=<QId> nRateShaperId=<RId>\n");
         printf ("QId:  Queue index\n");
         printf ("RId:  Rate shaper index\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_QoS_ShaperQueueAssign(pArgs->prmc, pArgs->prmvs);

   }

   else if (strcmp(pArgs->name, "fapi-GSW-QoS-ShaperQueueDeassign") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-QoS-ShaperQueueDeassign nQueueId=<QId> nRateShaperId=<RId>\n");
         printf ("QId:  Queue index\n");
         printf ("RId:  Rate shaper index\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_QoS_ShaperQueueDeassign(pArgs->prmc, pArgs->prmvs);

   }
   
   else if (strcmp(pArgs->name, "fapi-GSW-QoS-SchedulerCfgGet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-QoS-SchedulerCfgGet nQueueId=<QId>\n");
         printf ("QId:  Queue index\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_QoS_SchedulerCfgGet(pArgs->prmc, pArgs->prmvs);

   }

   else if (strcmp(pArgs->name, "fapi-GSW-QoS-SchedulerCfgSet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-QoS-SchedulerCfgSet nQueueId=<QId> eType=<Type> nWeight=<We>\n");
         printf ("QId:  Queue index\n");
         printf ("Type:  Scheduler Type\n");
         printf ("We:  Weight in Token. Parameter used for WFQ configuration\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_QoS_SchedulerCfgSet(pArgs->prmc, pArgs->prmvs);

   }

   else if (strcmp(pArgs->name, "fapi-GSW-QoS-WredCfgGet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_QoS_WredCfgGet(pArgs->prmc, pArgs->prmvs);

   }

   else if (strcmp(pArgs->name, "fapi-GSW-QoS-WredCfgSet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-QoS-WredCfgSet eProfile=<Prof> eMode=<Md> eThreshMode=<THR> nRed_Min=<RMin> nRed_Max=<RMax> nYellow_Min=<YMin> nYellow_Max=<YMax> nGreen_Min=<GMin> nGreen_Max=<GMax>\n");
         printf ("Prof: Drop Probability Profile\n");
         printf ("Md:   Automatic or Manual Mode of Thresholds Config\n");
         printf ("THR:  WRED Threshold Mode Config\n");
         printf ("RMin: WRED Red Threshold Min\n");
         printf ("RMax: WRED Red Threshold Max\n");
         printf ("YMin: WRED Yellow Threshold Min\n");
         printf ("YMax: WRED Yellow Threshold Max\n");
         printf ("GMin: WRED Green Threshold Min\n");
         printf ("GMax: WRED Green Threshold Max\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_QoS_WredCfgSet(pArgs->prmc, pArgs->prmvs);

   }

   else if (strcmp(pArgs->name, "fapi-GSW-QoS-WredQueueCfgGet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;
      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-QoS-WredQueueCfgSet nQueueId=<QId>\n");
         printf ("QId:  Queue Index\n");
         goto goto_end_help;
      }

      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_QoS_WredQueueCfgGet(pArgs->prmc, pArgs->prmvs);

   }

   else if (strcmp(pArgs->name, "fapi-GSW-QoS-WredQueueCfgSet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-QoS-WredQueueCfgSet nQueueId=<QId> nRed_Min=<RMin> nRed_Max=<RMax> nYellow_Min=<YMin> nYellow_Max=<YMax> nGreen_Min=<GMin> nGreen_Max=<GMax>\n");
         printf ("QId:  Queue Index\n");
         printf ("RMin: WRED Red Threshold Min\n");
         printf ("RMax: WRED Red Threshold Max\n");
         printf ("YMin: WRED Yellow Threshold Min\n");
         printf ("YMax: WRED Yellow Threshold Max\n");
         printf ("GMin: WRED Green Threshold Min\n");
         printf ("GMax: WRED Green Threshold Max\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_QoS_WredQueueCfgSet(pArgs->prmc, pArgs->prmvs);

   }

   else if (strcmp(pArgs->name, "fapi-GSW-QoS-WredPortCfgGet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;
      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-QoS-WredPortCfgGet nPortId=<port>\n");
         printf ("port:  Port Index\n");
         goto goto_end_help;
      }

      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_QoS_WredPortCfgGet(pArgs->prmc, pArgs->prmvs);

   }

   else if (strcmp(pArgs->name, "fapi-GSW-QoS-WredPortCfgSet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-QoS-WredPortCfgSet nPortId=<port> nRed_Min=<RMin> nRed_Max=<RMax> nYellow_Min=<YMin> nYellow_Max=<YMax> nGreen_Min=<GMin> nGreen_Max=<GMax>\n");
         printf ("port:  Port Index\n");
         printf ("RMin: WRED Red Threshold Min\n");
         printf ("RMax: WRED Red Threshold Max\n");
         printf ("YMin: WRED Yellow Threshold Min\n");
         printf ("YMax: WRED Yellow Threshold Max\n");
         printf ("GMin: WRED Green Threshold Min\n");
         printf ("GMax: WRED Green Threshold Max\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_QoS_WredPortCfgSet(pArgs->prmc, pArgs->prmvs);

   }

   else if (strcmp(pArgs->name, "fapi-GSW-TrunkingCfgGet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_TrunkingCfgGet(pArgs->prmc, pArgs->prmvs);

   }

   else if (strcmp(pArgs->name, "fapi-GSW-TrunkingCfgSet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-TrunkingCfgSet bIP_Src=<bIP_Src> bIP_Dst=<bIP_Dst> bMAC_Src=<bMAC_Src> bMAC_Dst=<bMAC_Dst> bSrc_Port=<bSrc_Port> bDst_Port=<bDst_Port>\n");
         printf ("bIP_Src:  MAC source address Use\n");
         printf ("bIP_Dst:  MAC destination address Use\n");
         printf ("bMAC_Src: MAC source address Use\n");
         printf ("bMAC_Dst: MAC destination address Use\n");
         printf ("bSrc_Port:  TCP/UDP source Port Use\n");
         printf ("bDst_Port:  TCP/UDP Destination Port Use\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_TrunkingCfgSet(pArgs->prmc, pArgs->prmvs);

   }
   else if (strcmp(pArgs->name, "fapi-GSW-MAC-TableClear") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_MAC_TableClear(pArgs->prmc, pArgs->prmvs);

   }

   else if (strcmp(pArgs->name, "fapi-GSW-MAC-TableClear-Cond") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      {
         printf ("Usage: fapi-GSW-MAC-TableClear-Cond eType=<> nPortId=<>\n");
         printf ("eType: MAC table clear type\n");
         printf ("nPortId: Physical port id\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);

      ret = fapi_GSW_MAC_TableCondClear(pArgs->prmc, pArgs->prmvs);

   }

   else if (strcmp(pArgs->name, "fapi-GSW-CfgGet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;


      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_CfgGet(pArgs->prmc, pArgs->prmvs);

   }
   else if (strcmp(pArgs->name, "fapi-GSW-CfgSet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-CfgSet bIP_Src=<bIP_Src> bIP_Dst=<bIP_Dst> bMAC_Src=<bMAC_Src> bMAC_Dst=<bMAC_Dst> bSrc_Port=<bSrc_Port> bDst_Port=<bDst_Port>\n");
         printf ("eMAC_TableAgeTimer: MAC table aging timer\n");
         printf ("nAgeTimer:  MAC table aging timer in seconds\n");
         printf ("nMaxPacketLen:  Maximum Ethernet packet length\n");
         printf ("bLearningLimitAction: Automatic MAC address table learning limitation {False: Drop/True: Forward}\n");
         printf ("bMAC_LockingAction: Accept or discard MAC port locking violation packets {False: Drop/True: Forward}\n");
         printf ("bMAC_SpoofingAction:  Accept or discard MAC spoofing and port MAC locking violation packets {False: Drop/True: Forward}\n");
         printf ("bDst_bPauseMAC_ModeSrcPort: Pause frame MAC source address mode\n");
         printf ("nPauseMAC_Src: Pause frame MAC source address\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_CfgSet(pArgs->prmc, pArgs->prmvs);

   }
   else if (strcmp(pArgs->name, "fapi-GSW-MAC-TableEntryRemove") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-MAC-TableEntryRemove nFId=<FId> nMAC=<MAC> nTci=<Tci>\n");
         printf ("FId: Filtering Identifier (FID)\n");
         printf ("MAC:  MAC Address to be removed from the table\n");
         printf ("Tci:  TCI for (GSWIP-3.2) B-Step\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_MAC_TableEntryRemove(pArgs->prmc, pArgs->prmvs);

   }
   else if (strcmp(pArgs->name, "fapi-GSW-MAC-TableEntryQuery") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 2)
      { 
         printf ("Usage: fapi-GSW-TableEntryQuery nFId=<FId> nMAC=<MAC> nTci=<Tci> nFilterFlag=<Flag>\n");
         printf ("FId: Filtering Identifier (FID)\n");
         printf ("MAC:  MAC Address to be removed from the table\n");
         printf ("Tci:  TCI for (GSWIP-3.2) B-Step\n");
         printf ("Flag: Source/Destination MAC address filtering flag {Value 0 - not filter, 1 - source address filter, 2 - destination address filter, 3 - both source and destination filter}\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_MAC_TableEntryQuery(pArgs->prmc, pArgs->prmvs);

   }



   else if (strcmp(pArgs->name, "fapi-GSW-QoS-FlowctrlCfgGet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_QoS_FlowctrlCfgGet(pArgs->prmc, pArgs->prmvs);

   }
   else if (strcmp(pArgs->name, "fapi-GSW-QoS-FlowctrlCfgSet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-QoS-FlowctrlCfgSet nFlowCtrlNonConform_Min=<NCMin> nFlowCtrlNonConform_Max=<NCMax> nFlowCtrlConform_Min=<CMin> nFlowCtrlConform_Max=<CMax>\n");
         printf ("NCMin: Global Buffer Non Conforming Flow Control Threshold Minimum\n");
         printf ("NCMax: Global Buffer Non Conforming Flow Control Threshold Maximum\n");
         printf ("CMin:  Global Buffer Conforming Flow Control Threshold Minimum\n");
         printf ("CMax:  Global Buffer Conforming Flow Control Threshold Maximum\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_QoS_FlowctrlCfgSet(pArgs->prmc, pArgs->prmvs);

   }

   else if (strcmp(pArgs->name, "fapi-GSW-QoS-FlowctrlPortCfgGet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;
      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-QoS-FlowctrlPortCfgGet nPortId=<port>\n");
         printf ("port: Ethernet Port number\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_QoS_FlowctrlPortCfgGet(pArgs->prmc, pArgs->prmvs);

   }
   else if (strcmp(pArgs->name, "fapi-GSW-QoS-FlowctrlPortCfgSet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-QoS-FlowctrlPortCfgSet nPortId=<port> nFlowCtrl_Min=<Min> nFlowCtrl_Max=<Max>\n");
         printf ("port: Ethernet Port number\n");
         printf ("Min: Ingress Port occupied Buffer Flow Control Threshold Minimum\n");
         printf ("Max: Ingress Port occupied Buffer Flow Control Threshold Maximum\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_QoS_FlowctrlPortCfgSet(pArgs->prmc, pArgs->prmvs);

   }

   else if (strcmp(pArgs->name, "fapi-GSW-MAC-TableEntryAdd") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-MAC-TableEntryAdd\n");
         printf ("nFId=<> : Filtering Identifier (FID)\n");
         printf ("nPortId=<> : Ethernet Port number (zero-based counting)\n");
         printf ("nAgeTimer=<> : Aging Time, given in multiples of 1 second in a range\n");
         printf ("bStaticEntry=<> : Static Entry (value will be aged out if the entry is not set to static)\n");
         printf ("nTrafficClass=<> : Egress queue traffic class\n");
         printf ("bIgmpControlled=<> : Packet is marked as IGMP controlled if destination MAC address matches MAC in this entry\n");
         printf ("nFilterFlag=<> : Source/Destination MAC address filtering flag\n");
         printf ("nSVLAN_Id=<> : STAG VLAN Id. Only applicable in case SVLAN support is enabled on the device\n");
         printf ("nSubIfId=<> : In GSWIP-3.1, this field is sub interface ID for WLAN logical port\n");
         printf ("nMAC=<> : MAC Address to add to the table\n");
         printf ("nAssociatedMAC=<> : Associated Mac address\n");
         printf ("nTci=<> : TCI for (GSWIP-3.2) B-Step\n");
         printf ("nPortMapValueIndex0=<> : Bridge Port Map 0\n");
         printf ("nPortMapValueIndex1=<> : Bridge Port Map 1\n");
         printf ("nPortMapValueIndex2=<> : Bridge Port Map 2\n");
         printf ("nPortMapValueIndex3=<> : Bridge Port Map 3\n");
         printf ("nPortMapValueIndex4=<> : Bridge Port Map 4\n");
         printf ("nPortMapValueIndex5=<> : Bridge Port Map 5\n");
         printf ("nPortMapValueIndex6=<> : Bridge Port Map 6\n");
         printf ("nPortMapValueIndex7=<> : Bridge Port Map 7\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_MAC_TableEntryAdd(pArgs->prmc, pArgs->prmvs);

   }
   else if (strcmp(pArgs->name, "fapi-GSW-MAC-TableEntryRead") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_MAC_TableEntryRead(pArgs->prmc, pArgs->prmvs);

   }


   else if (strcmp(pArgs->name, "fapi-GSW-QoS-QueuePortGet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-QoS-QueuePortGet nPortId=<> nTrafficClassId=<> bRedirectionBypass=<> bExtrationEnable=<>\n");
         printf ("nPortId=<> : Ethernet Port number (zero-based counting)\n");
         printf ("nTrafficClassId=<> : Traffic Class index\n");
         printf ("bRedirectionBypass=<> : Queue Redirection bypass Option\n");
         printf ("bExtrationEnable=<> : Forward CPU (extraction) before external QoS queueing \n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_QoS_QueuePortGet(pArgs->prmc, pArgs->prmvs);

   }
   else if (strcmp(pArgs->name, "fapi-GSW-QoS-QueuePortSet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-QoS-QueuePortSet nPortId=<> nTrafficClassId=<> bRedirectionBypass=<> bExtrationEnable=<> eQMapMode=<> nQueueId=<> nRedirectPortId=<> bEnableIngressPceBypass=<> bReservedPortMode=<>\n");
         printf ("nPortId=<> : Ethernet Port number (zero-based counting)\n");
         printf ("nTrafficClassId=<> : Traffic Class index\n");
         printf ("bRedirectionBypass=<> : Queue Redirection bypass Option\n");
         printf ("bExtrationEnable=<> : Forward CPU (extraction) before external QoS queueing \n");
         printf ("eQMapMode=<> : Ethernet Port number (zero-based counting)\n");
         printf ("nQueueId=<> : Traffic Class index\n");
         printf ("nRedirectPortId=<> : Queue Redirection bypass Option\n");
         printf ("bEnableIngressPceBypass=<> : Forward CPU (extraction) before external QoS queueing \n");
         printf ("bReservedPortMode=<> : Ethernet Port number (zero-based counting)\n");



         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_QoS_QueuePortSet(pArgs->prmc, pArgs->prmvs);

   }
   else if (strcmp(pArgs->name, "fapi-GSW-BridgePortConfigGet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-BridgePortConfigGet nBridgePortId=<> eMask=<>\n");
         printf ("nBridgePortId=<> : Bridge ID\n");
         printf ("eMask=<> : Mask for updating/retrieving fields\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_BridgePortConfigGet(pArgs->prmc, pArgs->prmvs);

   }
   else if (strcmp(pArgs->name, "fapi-GSW-BridgePortConfigSet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-BridgePortConfigGet\n");
         printf ("nBridgePortId=<> : Bridge ID\n");
         printf ("nBridgeId=<> : Bridge ID\n");
         printf ("bIngressExtendedVlanEnable=<>\n");
         printf ("nIngressExtendedVlanBlockId=<>\n");
         printf ("bEgressExtendedVlanEnable=<>\n");
         printf ("nEgressExtendedVlanBlockId=<>\n");
         printf ("eIngressMarkingMode=<>\n");
         printf ("eEgressRemarkingMode=<>\n");
         printf ("bIngressMeteringEnable=<>\n");
         printf ("nIngressTrafficMeterId=<>\n");
         printf ("bEgressMeteringEnable=<>\n");
         printf ("nEgressTrafficMeterId=<>\n");
         printf ("bEgressBroadcastSubMeteringEnable=<>\n");
         printf ("bEgressMulticastSubMeteringEnable=<>\n");
         printf ("bEgressUnknownMulticastIPSubMeteringEnable=<>\n");
         printf ("bEgressUnknownMulticastNonIPSubMeteringEnable=<>\n");
         printf ("bEgressUnknownUnicastSubMeteringEnable=<>\n");
         printf ("nEgressBroadcastSubMeteringId=<>\n");
         printf ("nEgressMulticastSubMeteringId=<>\n");
         printf ("nEgressUnknownMulticastIPSubMeteringId=<>\n");
         printf ("nEgressUnknownMulticastNonIPSubMeteringId=<>\n");
         printf ("nEgressUnknownUnicastSubMeteringId=<>\n");
         printf ("nDestLogicalPortId=<>\n");
         printf ("nDestSubIfIdGroup=<>\n");
         printf ("bPmapperEnable=<>\n");
         printf ("ePmapperMappingMode=<>\n");
         printf ("nPmapperDestSubIfIdGroup=<>\n");
         printf ("bBridgePortMapEnable=<>\n");
         printf ("Index=<>\n");
         printf ("MapValue=<>\n");
         printf ("bMcDestIpLookupDisable=<>\n");
         printf ("bMcSrcIpLookupEnable=<>\n");
         printf ("bDestMacLookupDisable=<>\n");
         printf ("bSrcMacLearningDisable=<>\n");
         printf ("bMacSpoofingDetectEnable=<>\n");
         printf ("bPortLockEnable=<>\n");
         printf ("bMacLearningLimitEnable=<>\n");
         printf ("nMacLearningLimit=<>\n");
         printf ("bIngressVlanFilterEnable=<>\n");
         printf ("nIngressVlanFilterBlockId=<>\n");
         printf ("bBypassEgressVlanFilter1=<>\n");
         printf ("bEgressVlanFilter1Enable=<>\n");
         printf ("nEgressVlanFilter1BlockId=<>\n");
         printf ("bEgressVlanFilter2Enable=<>\n");
         printf ("nEgressVlanFilter2BlockId=<>\n");
         printf ("bVlanTagSelection=<>\n");
         printf ("bVlanSrcMacPriorityEnable=<>\n");
         printf ("bVlanSrcMacDEIEnable=<>\n");
         printf ("bVlanSrcMacVidEnable=<>\n");
         printf ("bVlanDstMacPriorityEnable=<>\n");
         printf ("bVlanDstMacDEIEnable=<>\n");
         printf ("bVlanDstMacVidEnable=<>\n");
         printf ("bVlanBasedMultiCastLookup=<>\n");
         printf ("bVlanMulticastPriorityEnable=<>\n");
         printf ("bVlanMulticastDEIEnable=<>\n");
         printf ("bVlanMulticastVidEnable=<>\n");
         printf ("bForce=<>\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_BridgePortConfigSet(pArgs->prmc, pArgs->prmvs);

   }

   else if (strcmp(pArgs->name, "fapi-GSW-CtpPortConfigGet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-CtpPortConfigGet nLogicalPortId=<> nSubIfIdGroup=<> eMask=<>\n");
         printf ("nLogicalPortId=<> : Bridge ID\n");
         printf ("nSubIfIdGroup=<> : Sub interface ID group\n");
         printf ("eMask=<> : Mask for updating/retrieving fields\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_CtpPortConfigGet(pArgs->prmc, pArgs->prmvs);

   }

   else if (strcmp(pArgs->name, "fapi-GSW-CtpPortConfigSet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-CtpPortConfigSet\n");
         printf ("nLogicalPortId=<>\n");
         printf ("nSubIfIdGroup=<>\n");
         printf( "nBridgePortId=<>\n");
         printf( "bForcedTrafficClass=<>\n");
         printf( "nDefaultTrafficClass=<>\n");
         printf( "bIngressExtendedVlanEnable=<>\n");
         printf( "nIngressExtendedVlanBlockId=<>\n");
         printf( "bIngressExtendedVlanIgmpEnable=<>\n");
         printf( "nIngressExtendedVlanBlockIdIgmp=<>\n");
         printf( "bEgressExtendedVlanEnable=<>\n");
         printf( "nEgressExtendedVlanBlockId=<>\n");
         printf( "bEgressExtendedVlanIgmpEnable=<>\n");
         printf( "nEgressExtendedVlanBlockIdIgmp=<>\n");
         printf( "bIngressNto1VlanEnable=<>\n");
         printf( "bEgressNto1VlanEnable=<>\n");
         printf( "eIngressMarkingMode=<>\n");
         printf( "eEgressMarkingMode=<>\n");
         printf( "bEgressMarkingOverrideEnable=<>\n");
         printf( "eEgressMarkingModeOverride=<>\n");
         printf( "eEgressRemarkingMode=<>\n");
         printf( "bIngressMeteringEnable=<>\n");
         printf( "nIngressTrafficMeterId=<>\n");
         printf( "bEgressMeteringEnable=<>\n");
         printf( "nEgressTrafficMeterId=<>\n");
         printf( "bBridgingBypass=<>\n");
         printf( "nDestLogicalPortId=<>\n");
         printf( "nDestSubIfIdGroup=<>\n");
         printf( "bPmapperEnable=<>\n");
         printf( "ePmapperMappingMode=<>\n");
         printf( "nFirstFlowEntryIndex=<>\n");
         printf( "nNumberOfFlowEntries=<>\n");
         printf( "bIngressLoopbackEnable=<>\n");
         printf( "bIngressDaSaSwapEnable=<>\n");
         printf( "bEgressLoopbackEnable=<>\n");
         printf( "bEgressDaSaSwapEnable=<>\n");
         printf( "bIngressMirrorEnable=<>\n");
         printf( "bEgressMirrorEnable=<>\n");

         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_CtpPortConfigSet(pArgs->prmc, pArgs->prmvs);

   }
   else if (strcmp(pArgs->name, "fapi-GSW-BridgeAlloc") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_BridgeAlloc(pArgs->prmc, pArgs->prmvs);

   }

   else if (strcmp(pArgs->name, "fapi-GSW-BridgeFree") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-BridgeFree nBridgeId=<>\n");
         printf ("nBridgeId: Bridge ID (FID) to which this bridge port is associated\n");
  
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_BridgeFree(pArgs->prmc, pArgs->prmvs);

   }

   else if (strcmp(pArgs->name, "fapi-GSW-BridgeConfigGet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-BridgeConfigGet nBridgeId=<> eMask=<>\n");
         printf ("nBridgeId: Bridge ID (FID) to which this bridge port is associated\n");
         printf ("eMask: Mask\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_BridgeConfigGet(pArgs->prmc, pArgs->prmvs);

   }

   else if (strcmp(pArgs->name, "fapi-GSW-BridgeConfigSet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-BridgeConfigSet\n");
         printf("nBridgeId=<>\n");
         printf("bMacLearningLimitEnable=<>\n");
         printf("nMacLearningLimit=<>\n");
         printf("eForwardBroadcast=<>\n");
         printf("eForwardUnknownMulticastIp=<>\n");
         printf("eForwardUnknownMulticastNonIp=<>\n");
         printf("eForwardUnknownUnicast=<>\n");
         printf("bBroadcastMeterEnable=<>\n");
         printf("nBroadcastMeterId=<>\n");
         printf("bMulticastMeterEnable=<>\n");
         printf("nMulticastMeterId=<>\n");
         printf("bUnknownMulticastIpMeterEnable=<>\n");
         printf("nUnknownMulticastIpMeterId=<>\n");
         printf("bUnknownMulticastNonIpMeterEnable=<>\n");
         printf("nUnknownMulticastNonIpMeterId=<>\n");
         printf("bUnknownUniCastMeterEnable=<>\n");
         printf("nUnknownUniCastMeterId=<>\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_BridgeConfigSet(pArgs->prmc, pArgs->prmvs);

   }



   // ###################


   else if (strcmp(pArgs->name, "fapi-GSW-ExtendedVlanAlloc") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;
      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-ExtendedVlanAlloc nNumberOfEntries=<>\n");
         printf ("nNumberOfEntries: Total number of extended VLAN entries are requested\n");
  
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_ExtendedVlanAlloc(pArgs->prmc, pArgs->prmvs);

   }

   else if (strcmp(pArgs->name, "fapi-GSW-ExtendedVlanFree") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-ExtendedVlanFree nExtendedVlanBlockId=<>\n");
         printf ("nExtendedVlanBlockId: Block Id\n");
  
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_ExtendedVlanFree(pArgs->prmc, pArgs->prmvs);

   }

   else if (strcmp(pArgs->name, "fapi-GSW-ExtendedVlanGet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-ExtendedVlanGet\n");
         printf ("nExtendedVlanBlockId=<>\n");
         printf ("nEntryIndex=<>\n");
         printf ("bOriginalPacketFilterMode=<>\n");
         printf ("eFilter_4_Tpid_Mode=<>\n");
         printf ("eTreatment_4_Tpid_Mode=<>\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_ExtendedVlanGet(pArgs->prmc, pArgs->prmvs);

   }

   else if (strcmp(pArgs->name, "fapi-GSW-ExtendedVlanSet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-ExtendedVlanSet\n");
         printf("nExtendedVlanBlockId=<>\n");
         printf("nEntryIndex=<>\n");
         printf("eOuterVlanFilterVlanType=<>\n");
         printf("bOuterVlanFilterPriorityEnable=<>\n");
         printf("nOuterVlanFilterPriorityVal=<>\n");
         printf("bOuterVlanFilterVidEnable=<>\n");
         printf("nOuterVlanFilterVidVal=<>\n");
         printf("eOuterVlanFilterTpid=<>\n");
         printf("eOuterVlanFilterDei=<>\n");
         printf("eInnerVlanFilterVlanType=<>\n");
         printf("bInnerVlanFilterPriorityEnable=<>\n");
         printf("nInnerVlanFilterPriorityVal=<>\n");
         printf("bInnerVlanFilterVidEnable=<>\n");
         printf("nInnerVlanFilterVidVal=<>\n");
         printf("eInnerVlanFilterTpid=<>\n");
         printf("eInnerVlanFilterDei=<>\n");
         printf("eEtherType=<>\n");
         printf("eRemoveTagAction=<>\n");
         printf("bOuterVlanActionEnable=<>\n");
         printf("eOuterVlanActionPriorityMode=<>\n");
         printf("eOuterVlanActionPriorityVal=<>\n");
         printf("eOuterVlanActionVidMode=<>\n");
         printf("eOuterVlanActionVidVal=<>\n");
         printf("eOuterVlanActionTpid=<>\n");
         printf("eOuterVlanActioneDei=<>\n");
         printf("bInnerVlanActionEnable=<>\n");
         printf("eInnerVlanActionPriorityMode=<>\n");
         printf("eInnerVlanActionPriorityVal=<>\n");
         printf("eInnerVlanActionVidMode=<>\n");
         printf("eInnerVlanActionVidVal=<>\n");
         printf("eInnerVlanActionTpid=<>\n");
         printf("eInnerVlanActioneDei=<>\n");
         printf("bReassignBridgePortEnable=<>\n");
         printf("nNewBridgePortId=<>\n");
         printf("bNewDscpEnable=<>\n");
         printf("nNewDscp=<>\n");
         printf("bNewTrafficClassEnable=<>\n");
         printf("nNewTrafficClass=<>\n");
         printf("bNewMeterEnable=<>\n");
         printf("sNewTrafficMeterId=<>\n");
         printf("bLoopbackEnable=<>\n");
         printf("bDaSaSwapEnable=<>\n");
         printf("bMirrorEnable=<>\n");
         printf("bDscp2PcpMapEnable=<>\n");
         printf("nDscp2PcpMapValue=<>\n");
         printf("bOriginalPacketFilterMode=<>\n");
         printf("eFilter_4_Tpid_Mode=<>\n");
         printf("eTreatment_4_Tpid_Mode=<>\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_ExtendedVlanSet(pArgs->prmc, pArgs->prmvs);

   }

   else if (strcmp(pArgs->name, "fapi-GSW-VlanFilterAlloc") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;
      if (pArgs->prmc < 3)
      { 
         printf ("Usage: fapi-GSW-VlanFilterAlloc nNumberOfEntries=<> bDiscardUntagged=<> bDiscardUnmatchedTagged=<>\n");
         printf ("nNumberOfEntries: Total number of extended VLAN entries are requested\n");
         printf ("bDiscardUntagged: Discard packet without VLAN tag\n");
         printf ("bDiscardUnmatchedTagged: Discard packet not matching\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_VlanFilterAlloc(pArgs->prmc, pArgs->prmvs);

   }

   else if (strcmp(pArgs->name, "fapi-GSW-VlanFilterFree") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-VlanFilterFree nVlanFilterBlockId=<>\n");
         printf ("nVlanFilterBlockId: Block Id\n");
  
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_VlanFilterFree(pArgs->prmc, pArgs->prmvs);

   }

   else if (strcmp(pArgs->name, "fapi-GSW-VlanFilterGet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-VlanFilterGet nVlanFilterBlockId=<> nEntryIndex=<>\n");
         printf ("nVlanFilterBlockId: Block Id\n");
         printf ("nEntryIndex: Entry Index\n");
  
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_VlanFilterGet(pArgs->prmc, pArgs->prmvs);

   }

   else if (strcmp(pArgs->name, "fapi-GSW-VlanFilterSet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-VlanFilterSet nVlanFilterBlockId=<> nEntryIndex=<> eVlanFilterMask=<> nVal=<> bDiscardMatched=<>\n");
         printf("nVlanFilterBlockId=<>\n");
         printf("nEntryIndex=<>\n");
         printf("eVlanFilterMask=<>\n");
         printf("nVal=<>\n");
         printf("bDiscardMatched=<>\n");
  
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_VlanFilterSet(pArgs->prmc, pArgs->prmvs);

   }

   else if (strcmp(pArgs->name, "fapi-GSW-STP-PortCfgGet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-STP-PortCfgGet nPortId=<>\n");
         printf ("nPortId: Port Id\n");
  
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_STP_PortCfgGet(pArgs->prmc, pArgs->prmvs);

   }

   else if (strcmp(pArgs->name, "fapi-GSW-STP-PortCfgSet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 2)
      { 
         printf ("Usage: fapi-GSW-STP-PortCfgSet nPortId=<> ePortState=<>\n");
         printf("nPortId=<>\n");
         printf("ePortState=<>\n");
  
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_STP_PortCfgSet(pArgs->prmc, pArgs->prmvs);

   }

   else if (strcmp(pArgs->name, "fapi-GSW-STP-BPDU-RuleGet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_STP_BPDU_RuleGet(pArgs->prmc, pArgs->prmvs);

   }

   else if (strcmp(pArgs->name, "fapi-GSW-STP-BPDU-RuleSet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-STP-BPDU-RuleSet eForwardPort=<> nForwardPortId=<>\n");
         printf("eForwardPort=<>\n");
         printf("nForwardPortId=<>\n");
  
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_STP_BPDU_RuleSet(pArgs->prmc, pArgs->prmvs);

   }
   else if (strcmp(pArgs->name, "fapi-GSW-Debug-RMON-Port-Get") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 2)
      { 
         printf ("Usage: fapi-GSW-Debug-RMON-Port-Get nPortId=<> ePortType=<> b64BitMode=<>\n");
         printf("nPortId=<>\n");
	      printf("ePortType=<>\n");
	      printf("b64BitMode=<>\n");
  
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_Debug_RMON_Port_Get(pArgs->prmc, pArgs->prmvs);

   }

   else if (strcmp(pArgs->name, "fapi-GSW-QoS-MeterCfgGet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-QoS-MeterCfgGet nMeterId=<> \n");
         printf("nMeterId: Meter index (zero-based counting)\n");
  
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_QoS_MeterCfgGet(pArgs->prmc, pArgs->prmvs);

   }


else if (strcmp(pArgs->name, "fapi-GSW-QoS-MeterCfgSet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-QoS-MeterCfgSet\n");
         printf("nMeterId: Meter index (zero-based counting)\n");
         printf("bEnable=<>\n");
         printf("eMtrType=<>\n");
         printf("nCbs=<>\n");
         printf("nEbs=<>\n");
         printf("nCbs_ls=<>\n");
         printf("nEbs_ls=<>\n");
         printf("nRate=<>\n");
         printf("nPiRate=<>\n");
         printf("cMeterName=<>\n");
         printf("nColourBlindMode=<>\n");
         printf("bPktMode=<>\n");
         printf("bLocalOverhd=<>\n");
         printf("nLocaloverhd=<>\n");
  
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_QoS_MeterCfgSet(pArgs->prmc, pArgs->prmvs);

   }


   
else if (strcmp(pArgs->name, "fapi-GSW-MAC-DefaultFilterGet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-MAC-DefaultFilterGet eType=<>\n");
         printf("eType: MAC Address Filter Type\n");

  
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_MAC_DefaultFilterGet(pArgs->prmc, pArgs->prmvs);

   }
else if (strcmp(pArgs->name, "fapi-GSW-MAC-DefaultFilterSet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

  if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-MAC-DefaultFilterSet eType=<>\n");
         printf("eType: MAC Address Filter Type\n");
  
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_MAC_DefaultFilterSet(pArgs->prmc, pArgs->prmvs);

   }
else if (strcmp(pArgs->name, "fapi-GSW-CTP-PortAssignmentGet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-CTP-PortAssignmentGet nLogicalPortId=<>\n");
         printf("nLogicalPortId=<>\n");  
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_CTP_PortAssignmentGet(pArgs->prmc, pArgs->prmvs);

   }
else if (strcmp(pArgs->name, "fapi-GSW-CTP-PortAssignmentSet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-CTP-PortAssignmentSet\n");
         printf("nLogicalPortId=<>\n");
         printf("nFirstCtpPortId=<>\n");
         printf("nNumberOfCtpPort=<>\n");
         printf("eMode=<>\n");
  
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_CTP_PortAssignmentSet(pArgs->prmc, pArgs->prmvs);

   }
else if (strcmp(pArgs->name, "fapi-GSW-PMAC-GLBL-CfgSet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-PMAC-GLBL-CfgSet\n");
         printf("nPmacId=<>\n");
         printf("bRxFCSDis=<>\n");
         printf("eProcFlagsEgCfg=<>\n");
         printf("nBslThreshold0=<>\n");
         printf("nBslThreshold1=<>\n");
         printf("nBslThreshold2=<>\n");
         printf("bAPadEna=<>\n");
         printf("bPadEna=<>\n");
         printf("bVPadEna=<>\n");
         printf("bSVPadEna=<>\n");
         printf("bTxFCSDis=<>\n");
         printf("bIPTransChkRegDis=<>\n");
         printf("bIPTransChkVerDis=<>\n");
         printf("bJumboEna=<>\n");
         printf("nMaxJumboLen=<>\n");
         printf("nJumboThreshLen=<>\n");
         printf("bLongFrmChkDis=<>\n");
         printf("eShortFrmChkType=<>\n");
         printf("bProcFlagsEgCfgEna=<>\n");

  
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_PMAC_GLBL_CfgSet(pArgs->prmc, pArgs->prmvs);

   }
else if (strcmp(pArgs->name, "fapi-GSW-PMAC-GLBL-CfgGet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-PMAC-GLBL-CfgGet\n");
         printf("nPmacId=<>\n");
  
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_PMAC_GLBL_CfgGet(pArgs->prmc, pArgs->prmvs);

   }
else if (strcmp(pArgs->name, "fapi-GSW-PMAC-BM-CfgSet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 2)
      { 
         printf ("Usage: fapi-GSW-PMAC-BM-CfgSet\n");
	      printf("nTxDmaChanId=<>\n");
         printf("nPmacId=<>\n");
	      printf("txQMask=<>\n");
	      printf("rxPortMask=<>\n");
  
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_PMAC_BM_CfgSet(pArgs->prmc, pArgs->prmvs);

   }
else if (strcmp(pArgs->name, "fapi-GSW-PMAC-BM-CfgGet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 2)
      { 
         printf ("Usage: fapi-GSW-PMAC-BM-CfgGet\n");
         printf("nTxDmaChanId=<>\n");
         printf("nPmacId=<>\n");
  
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_PMAC_BM_CfgGet(pArgs->prmc, pArgs->prmvs);

   }
else if (strcmp(pArgs->name, "fapi-GSW-PMAC-EG-CfgSet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-PMAC-EG-CfgSet\n");
         printf("nPmacId=<>\n");
         printf("bRedirEnable=<>\n");
         printf("bBslSegmentDisable=<>\n");
         printf("nBslTrafficClass=<>\n");;
         printf("bResDW1Enable=<>\n");
         printf("bRes2DW0Enable=<>\n");
         printf("bRes1DW0Enable=<>\n");
         printf("bTCEnable=<>\n");
         printf("nDestPortId=<>\n");
         printf("bProcFlagsSelect=<>\n");
         printf("nTrafficClass=<>\n");
         printf("nFlowIDMsb=<>\n");
         printf("bMpe1Flag=<>\n");
         printf("bMpe2Flag=<>\n");
         printf("bEncFlag=<>\n");
         printf("bDecFlag=<>\n");
         printf("nRxDmaChanId=<>\n");
         printf("bRemL2Hdr=<>\n");
         printf("numBytesRem=<>\n");
         printf("bFcsEna=<>\n");
         printf("bPmacEna=<>\n");
         printf("nResDW1=<>\n");
         printf("nRes1DW0=<>\n");
         printf("nRes2DW0=<>\n");
  
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_PMAC_EG_CfgSet(pArgs->prmc, pArgs->prmvs);

   }
else if (strcmp(pArgs->name, "fapi-GSW-PMAC-EG-CfgGet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-PMAC-EG-CfgGet\n");
         printf("nPmacId=<>\n");
         printf("nDestPortId=<>\n");
         printf("bProcFlagsSelect=<>\n");
         printf("nTrafficClass=<>\n");
         printf("nFlowIDMsb=<>\n");
         printf("bMpe1Flag=<>\n");
         printf("bMpe2Flag=<>\n");
         printf("bEncFlag=<>\n");
         printf("bDecFlag=<>\n");
  
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_PMAC_EG_CfgGet(pArgs->prmc, pArgs->prmvs);

   }
else if (strcmp(pArgs->name, "fapi-GSW-PMAC-IG-CfgGet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-PMAC-IG-CfgGet\n");
         printf("nPmacId=<>\n");
	      printf("nTxDmaChanId=<>\n");
  
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_PMAC_IG_CfgGet(pArgs->prmc, pArgs->prmvs);

   }
else if (strcmp(pArgs->name, "fapi-GSW-PMAC-IG-CfgSet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-PMAC-IG-CfgSet\n");
         printf("nPmacId=<>\n");
         printf("nTxDmaChanId=<>\n");
         printf("bErrPktsDisc=<>\n");
         printf("bPmapDefault=<>\n");
         printf("bPmapEna=<>\n");
         printf("bClassDefault=<>\n");
         printf("bClassEna=<>\n");
         printf("eSubId=<>\n");
         printf("bSpIdDefault=<>\n");
         printf("bPmacPresent=<>\n");
         printf("defPmacHdr=<>\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_PMAC_IG_CfgSet(pArgs->prmc, pArgs->prmvs);

   }

else if (strcmp(pArgs->name, "fapi-GSW-PceRuleRead") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-PceRuleRead\n");
         printf("pattern.nIndex=<>\n");
         printf("nLogicalPortId=<>\n");
         printf("nSubIfIdGroup=<>\n");
         printf("region=<>\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_PceRuleRead(pArgs->prmc, pArgs->prmvs);

   }
else if (strcmp(pArgs->name, "fapi-GSW-PceRuleWrite") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-PceRuleWrite\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_PceRuleWrite(pArgs->prmc, pArgs->prmvs);

   }
else if (strcmp(pArgs->name, "fapi-GSW-PceRuleDelete") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-PceRuleDelete\n");
         printf("pattern.nIndex=<>\n");
         printf("nLogicalPortId=<>\n");
         printf("nSubIfIdGroup=<>\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_PceRuleDelete(pArgs->prmc, pArgs->prmvs);

   }
else if (strcmp(pArgs->name, "fapi-GSW-PceRuleAlloc") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-PceRuleAlloc\n");
         printf("num_of_rules=<>\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_PceRuleAlloc(pArgs->prmc, pArgs->prmvs);

   }
else if (strcmp(pArgs->name, "fapi-GSW-PceRuleFree") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-PceRuleFree\n");
         printf("blockid=<>\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_PceRuleFree(pArgs->prmc, pArgs->prmvs);

   }
else if (strcmp(pArgs->name, "fapi-GSW-PceRuleEnable") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-PceRuleEnable\n");
         printf("nLogicalPortId=<>\n");
         printf("nSubIfIdGroup=<>\n");
         printf("region=<>\n");
         printf("pattern.nIndex=<>\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_PceRuleEnable(pArgs->prmc, pArgs->prmvs);

   }
else if (strcmp(pArgs->name, "fapi-GSW-PceRuleDisable") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-PceRuleDisable\n");
         printf("nLogicalPortId=<>\n");
         printf("nSubIfIdGroup=<>\n");
         printf("region=<>\n");
         printf("pattern.nIndex=<>\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_PceRuleDisable(pArgs->prmc, pArgs->prmvs);

   }

else if (strcmp(pArgs->name, "fapi-GSW-MulticastRouterPortAdd") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-MulticastRouterPortAdd\n");
         printf("nPortId=<>\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_MulticastRouterPortAdd(pArgs->prmc, pArgs->prmvs);

   }
else if (strcmp(pArgs->name, "fapi-GSW-MulticastRouterPortRemove") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-MulticastRouterPortRemove\n");
         printf("nPortId=<>\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_MulticastRouterPortRemove(pArgs->prmc, pArgs->prmvs);

   }
else if (strcmp(pArgs->name, "fapi-GSW-MulticastSnoopCfgGet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_MulticastSnoopCfgGet(pArgs->prmc, pArgs->prmvs);

   }
else if (strcmp(pArgs->name, "fapi-GSW-MulticastSnoopCfgSet") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-MulticastSnoopCfgSet\n");
         printf("eIGMP_Mode=<>\n");
         printf("bCrossVLAN=<>\n");
         printf("eForwardPort=<>\n");
         printf("nForwardPortId=<>\n");
         printf("nClassOfService=<>\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_MulticastSnoopCfgSet(pArgs->prmc, pArgs->prmvs);

   }
else if (strcmp(pArgs->name, "fapi-GSW-MulticastRouterPortRead") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;


      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_MulticastRouterPortRead(pArgs->prmc, pArgs->prmvs);

   }

else if (strcmp(pArgs->name, "fapi-GSW-MulticastTableEntryAdd") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      {
         printf ("Usage: fapi-GSW-MulticastTableEntryRemove\n");
         printf ("nPortId=<> : Bridge Port ID\n");
         printf ("nSubIfId=<> : Sub-Interface Id\n");
         printf ("eIPVersion=<> : Selection to use IPv4 or IPv6.\n");
         printf ("uIP_Gda=<> : Group Destination IP address (GDA).\n");
         printf ("uIP_Gsa=<> : Group Source IP address.\n");
         printf ("nFID=<> : Filtering Identifier (FID)\n");
         printf ("bExclSrcIP=<> : Includes or Excludes Source IP.\n");
         printf ("eModeMember=<> : Group member filter mode.\n");
         printf ("nTci=<> : TCI for (GSWIP-3.2) B-Step\n");
         goto goto_end_help;
      }

      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_MulticastTableEntryAdd(pArgs->prmc, pArgs->prmvs);

   }
else if (strcmp(pArgs->name, "fapi-GSW-MulticastTableEntryRead") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;


      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_MulticastTableEntryRead(pArgs->prmc, pArgs->prmvs);

   }
else if (strcmp(pArgs->name, "fapi-GSW-MulticastTableEntryRemove") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      {
         printf ("Usage: fapi-GSW-MulticastTableEntryRemove\n");
         printf ("nPortId=<> : Bridge Port ID\n");
         printf ("nSubIfId=<> : Sub-Interface Id\n");
         printf ("eIPVersion=<> : Selection to use IPv4 or IPv6.\n");
         printf ("uIP_Gda=<> : Group Destination IP address (GDA).\n");
         printf ("uIP_Gsa=<> : Group Source IP address.\n");
         printf ("nFID=<> : Filtering Identifier (FID)\n");
         printf ("bExclSrcIP=<> : Includes or Excludes Source IP.\n");
         printf ("eModeMember=<> : Group member filter mode.\n");
         printf ("nTci=<> : TCI for (GSWIP-3.2) B-Step\n");
         goto goto_end_help;
      }

      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_MulticastTableEntryRemove(pArgs->prmc, pArgs->prmvs);

   }
   else if (strcmp(pArgs->name, "fapi-GSW-FW-Update") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;


      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_FW_Update(pArgs->prmc, pArgs->prmvs);

   }
   else if (strcmp(pArgs->name, "fapi-GSW-FW-Version") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;


      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_FW_Version(pArgs->prmc, pArgs->prmvs);

   }
   else if (strcmp(pArgs->name, "fapi-GSW-PVT-Meas") == 0)
   {
      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;


      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_PVT_Meas(pArgs->prmc, pArgs->prmvs);
   }
   else if (strcmp(pArgs->name, "fapi-GSW-Delay") == 0)
   {
      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      {
         printf ("Usage: fapi-GSW-Delay\n");
         printf ("nMsec=<> : Delay Time in milliseconds\n");
         goto goto_end_help;
      }

      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_Delay(pArgs->prmc, pArgs->prmvs);
   }
   else if (strcmp(pArgs->name, "fapi-GSW-GPIO-Configure") == 0)
   {
      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      {
         printf ("Usage: fapi-GSW-GPIO-Configure\n");
         printf ("nEnableMaskIndex0=<> : GPIO Enable Mask Index 0\n");
         printf ("nEnableMaskIndex1=<> : GPIO Enable Mask Index 1\n");
         printf ("nEnableMaskIndex2=<> : GPIO Enable Mask Index 2\n");
         printf ("nAltSel0Index0=<> : GPIO Alt Select 0 Index 0\n");
         printf ("nAltSel0Index1=<> : GPIO Alt Select 0 Index 1\n");
         printf ("nAltSel0Index2=<> : GPIO Alt Select 0 Index 2\n");
         printf ("nAltSel1Index0=<> : GPIO Alt Select 1 Index 0\n");
         printf ("nAltSel1Index1=<> : GPIO Alt Select 1 Index 1\n");
         printf ("nAltSel1Index2=<> : GPIO Alt Select 1 Index 2\n");
         printf ("nDirIndex0=<> : GPIO Direction Index 0\n");
         printf ("nDirIndex1=<> : GPIO Direction Index 1\n");
         printf ("nDirIndex2=<> : GPIO Direction Index 2\n");
         printf ("nOutValueIndex0=<> : GPIO Out Value Index 0\n");
         printf ("nOutValueIndex1=<> : GPIO Out Value Index 1\n");
         printf ("nOutValueIndex2=<> : GPIO Out Value Index 2\n");
         printf("nTimeoutValue=<> : GPIO Timeout in milliseconds\n");
         goto goto_end_help;
      }

      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_GPIO_Configure(pArgs->prmc, pArgs->prmvs);
   }
   else if (strcmp(pArgs->name, "fapi-GSW-Reboot") == 0)
   {
      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;


      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_Reboot(pArgs->prmc, pArgs->prmvs);
   }
   else if (strcmp(pArgs->name, "fapi-GSW-SysReg-Rd") == 0)
   {
      char* slib ="";
      uint16_t reg = 0;
      GSW_Device_t*   gsw_dev;

      if (pArgs->prmc < 1)
      {
         printf ("Usage: fapi-GSW-SysReg-Rd addr=<addr>\n");
         printf ("reg: register address\n");
         goto goto_end_help;
      }

      slib      = slif_lib;

      api_gsw_get_links(slib);
      ret = fapi_GSW_SysReg_Rd(pArgs->prmc, pArgs->prmvs);
   }

   else if (strcmp(pArgs->name, "fapi-GSW-VlanCounterMapSet") == 0)
   {
      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      {
         printf ("Usage: fapi-GSW-VlanCounterMapSet\n");
         printf("nCounterIndex=<>\n");
         printf("nCtpPortId=<>\n");
         printf("bPriorityEnable=<>\n");
         printf("nPriorityVal=<>\n");
         printf("bVidEnable=<>\n");
         printf("nVidVal=<>\n");
         printf("bVlanTagSelectionEnable=<>\n");
         printf("eVlanCounterMappingType=<>\n");
         printf("eVlanCounterMappingFilterType=<>\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_VlanCounterMapSet(pArgs->prmc, pArgs->prmvs);
   }
   else if (strcmp(pArgs->name, "fapi-GSW-VlanCounterMapGet") == 0)
   {
      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      {
         printf ("Usage: fapi-GSW-VlanCounterMapGet\n");
         printf("nCounterIndex=<>\n");
         printf("eVlanCounterMappingType=<>\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_VlanCounterMapGet(pArgs->prmc, pArgs->prmvs);
   }
   else if (strcmp(pArgs->name, "fapi-GSW-Vlan-RMON-Get") == 0)
   {
      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      {
         printf ("Usage: fapi-GSW-Vlan-RMON-Get\n");
         printf("nVlanCounterIndex=<>\n");
         printf("eVlanRmonType=<>\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_Vlan_RMON_Get(pArgs->prmc, pArgs->prmvs);
   }
   else if (strcmp(pArgs->name, "fapi-GSW-Vlan-RMON-Clear") == 0)
   {
      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      {
         printf ("Usage: fapi-GSW-Vlan-RMON-Clear\n");
         printf("nVlanCounterIndex=<>\n");
         printf("eVlanRmonType=<>\n");
         printf("eClearAll=<>\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_Vlan_RMON_Clear(pArgs->prmc, pArgs->prmvs);
   }
   else if (strcmp(pArgs->name, "fapi-GSW-Vlan-RMONControl-Set") == 0)
   {
      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      {
         printf ("Usage: fapi-GSW-Vlan-RMONControl-Set\n");
         printf("bVlanRmonEnable=<>\n");
         printf("bIncludeBroadCastPktCounting=<>\n");
         printf("nVlanLastEntry=<>\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_Vlan_RMONControl_Set(pArgs->prmc, pArgs->prmvs);
   }
   else if (strcmp(pArgs->name, "fapi-GSW-Vlan-RMONControl-Get") == 0)
   {
      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_Vlan_RMONControl_Get(pArgs->prmc, pArgs->prmvs);
   }

   else if (strcmp(pArgs->name, "fapi-GSW-PBB-TunnelTempate-Config-Get") == 0)
   {
      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-PBB-TunnelTempate-Config-Get\n");
         printf ("nTunnelTemplateId:\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_PBB_TunnelTempate_Config_Get(pArgs->prmc, pArgs->prmvs);

   }
   else if (strcmp(pArgs->name, "fapi-GSW-PBB-TunnelTempate-Config-Set") == 0)
   {
      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-PBB-TunnelTempate-Config-Set\n");
         printf ("nTunnelTemplateId:\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_PBB_TunnelTempate_Config_Set(pArgs->prmc, pArgs->prmvs);

   }
   else if (strcmp(pArgs->name, "fapi-GSW-PBB-TunnelTempate-Alloc") == 0)
   {
      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_PBB_TunnelTempate_Alloc(pArgs->prmc, pArgs->prmvs);

   }
   else if (strcmp(pArgs->name, "fapi-GSW-PBB-TunnelTempate-Free") == 0)
   {
      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-PBB-TunnelTempate-Free\n");
         printf ("nTunnelTemplateId:\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_PBB_TunnelTempate_Free(pArgs->prmc, pArgs->prmvs);

   }

   else if (strcmp(pArgs->name, "fapi-GSW-CPU-PortCfgGet") == 0)
   {
      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-CPU-PortCfgGet\n");
         printf ("nPortId:\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_CPU_PortCfgGet(pArgs->prmc, pArgs->prmvs);

   }
   else if (strcmp(pArgs->name, "fapi-GSW-CPU-PortCfgSet") == 0)
   {
      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-CPU-PortCfgSet\n");
         printf ("nPortId:\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_CPU_PortCfgSet(pArgs->prmc, pArgs->prmvs);

   }
   else if (strcmp(pArgs->name, "fapi-GSW-RMON-MeterGet") == 0)
   {
      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-RMON-MeterGet\n");
         printf ("nMeterId:\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_RMON_MeterGet(pArgs->prmc, pArgs->prmvs);

   }
   else if (strcmp(pArgs->name, "fapi-GSW-RMON-FlowGet") == 0)
   {
      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-RMON-FlowGet\n");
         printf("bIndexd=<>\n");
         printf("nIndex=<>\n");
         printf("nPortId=<>\n");
         printf("nFlowId=<>\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_RMON_FlowGet(pArgs->prmc, pArgs->prmvs);

   }

   else if (strcmp(pArgs->name, "fapi-GSW-RMON-TFlowClear") == 0)
   {
      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-RMON-TFlowClear\n");
         printf("bIndexd=<>\n");
         printf("nIndex=<>\n");
         printf("nPortId=<>\n");
         printf("nFlowId=<>\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_RMON_FlowGet(pArgs->prmc, pArgs->prmvs);

   }
   else if (strcmp(pArgs->name, "fapi-GSW-BridgePortAlloc") == 0)
   {
      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_BridgePortAlloc(pArgs->prmc, pArgs->prmvs);

   }
   else if (strcmp(pArgs->name, "fapi-GSW-BridgePortFree") == 0)
   {
      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-BridgePortFree\n");
         printf("nBridgePortId:<>\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_BridgePortFree(pArgs->prmc, pArgs->prmvs);

   }
   else if (strcmp(pArgs->name, "fapi-GSW-UnFreeze") == 0)
   {
      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_UnFreeze(pArgs->prmc, pArgs->prmvs);

   }
   else if (strcmp(pArgs->name, "fapi-GSW-Freeze") == 0)
   {
      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;


      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_Freeze(pArgs->prmc, pArgs->prmvs);

   }

else if (strcmp(pArgs->name, "fapi-GSW-QoS-MeterFree") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-QoS-MeterFree nMeterId=<>\n");
         printf("nMeterId: Meter index (zero-based counting)\n");  
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_QoS_MeterFree(pArgs->prmc, pArgs->prmvs);

   }
else if (strcmp(pArgs->name, "fapi-GSW-QoS-MeterAlloc") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_QoS_MeterAlloc(pArgs->prmc, pArgs->prmvs);

   }

else if (strcmp(pArgs->name, "fapi-GSW-PMAC-RMON-Get") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-PMAC-RMON-Get\n");
         printf("nPmacId=<>\n");
         printf("nPortId=<>\n");
         printf("b64BitMode=<>\n");
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_PMAC_RMON_Get(pArgs->prmc, pArgs->prmvs);

   }
else if (strcmp(pArgs->name, "fapi-GSW-Debug-PMAC-RMON-Get-All") == 0)
   {

      char* slib ="";
      uint16_t phy = 0;
      GSW_Device_t*   gsw_dev;
      uint16_t myval;

      if (pArgs->prmc < 1)
      { 
         printf ("Usage: fapi-GSW-Debug-PMAC-RMON-Get-All\n");
         printf("nPmacId=<>\n");
         printf("Start=<>\n");
         printf("End=<>\n");
 
         goto goto_end_help;
      }
      slib      = slif_lib;
      api_gsw_get_links(slib);
      ret = fapi_GSW_Debug_PMAC_RMON_Get_All(pArgs->prmc, pArgs->prmvs);

   }
   // else if (strcmp(pArgs->name, "fapi-GSW-Debug-RMON-Port-GetAll") == 0)
   // {
   //    char* slib ="";
   //    uint16_t phy = 0;
   //    GSW_Device_t*   gsw_dev;
   //    uint16_t myval;

   //    if (pArgs->prmc < 1)
   //    { 
   //       printf ("Usage: fapi-GSW-Debug-RMON-Port-GetAll ePortType=<> Start=<> End=<>\n");
   //       printf ("ePortType: Port Type\n");
   //       printf ("Start: Start Port\n");
   //       printf ("End: End Port\n");
   //       goto goto_end_help;
   //    }
   //    slib      = slif_lib;
   //    api_gsw_get_links(slib);
   //    ret = fapi_GSW_DEBUG_RMON_Port_Get_All(pArgs->prmc, pArgs->prmvs);

   // }


   /***************
   *  No command  *
   ***************/
   else 
   {
      api_executed = OS_FALSE;
   }

goto_end_help:
   *err = ( int)ret;
   return api_executed;   
}




int cmds_fapi_symlink_set (void)
{  
   system ("ln -sf ./ethswbox fapi-GSW-RegisterGet");
   system ("ln -sf ./ethswbox fapi-GSW-RegisterMod");
   system ("ln -sf ./ethswbox fapi-GSW-RegisterSet");
   system ("ln -sf ./ethswbox fapi-GSW-PortLinkCfgGet");
   system ("ln -sf ./ethswbox fapi-GSW-PortLinkCfgSet");
   system ("ln -sf ./ethswbox fapi-GSW-RMON-Clear");
   system ("ln -sf ./ethswbox fapi-GSW-MonitorPortCfgGet");
   system ("ln -sf ./ethswbox fapi-GSW-MonitorPortCfgSet");
   system ("ln -sf ./ethswbox fapi-GSW-QoS-PortCfgSet");
   system ("ln -sf ./ethswbox fapi-GSW-QoS-PortCfgGet");
   system ("ln -sf ./ethswbox fapi-GSW-QoS-DSCP-ClassGet");
   system ("ln -sf ./ethswbox fapi-GSW-QoS-DSCP-ClassSet");
   system ("ln -sf ./ethswbox fapi-GSW-QoS-PCP-ClassGet");
   system ("ln -sf ./ethswbox fapi-GSW-QoS-PCP-ClassSet");
   system ("ln -sf ./ethswbox fapi-GSW-QoS-SVLAN-PCP-ClassGet");
   system ("ln -sf ./ethswbox fapi-GSW-QoS-SVLAN-PCP-ClassSet");
   system ("ln -sf ./ethswbox fapi-GSW-QoS-ShaperCfgGet");
   system ("ln -sf ./ethswbox fapi-GSW-QoS-ShaperCfgSet");
   system ("ln -sf ./ethswbox fapi-GSW-QoS-ShaperQueueGet");
   system ("ln -sf ./ethswbox fapi-GSW-QoS-ShaperQueueAssign");
   system ("ln -sf ./ethswbox fapi-GSW-QoS-ShaperQueueDeassign");
   system ("ln -sf ./ethswbox fapi-GSW-QoS-SchedulerCfgSet");
   system ("ln -sf ./ethswbox fapi-GSW-QoS-SchedulerCfgGet");
   system ("ln -sf ./ethswbox fapi-GSW-QoS-WredCfgSet");
   system ("ln -sf ./ethswbox fapi-GSW-QoS-WredCfgGet");
   system ("ln -sf ./ethswbox fapi-GSW-QoS-WredQueueCfgGet");
   system ("ln -sf ./ethswbox fapi-GSW-QoS-WredQueueCfgSet");
   system ("ln -sf ./ethswbox fapi-GSW-QoS-WredPortCfgSet");
   system ("ln -sf ./ethswbox fapi-GSW-QoS-WredPortCfgGet");
   system ("ln -sf ./ethswbox fapi-GSW-TrunkingCfgSet");
   system ("ln -sf ./ethswbox fapi-GSW-TrunkingCfgGet");
   system ("ln -sf ./ethswbox fapi-GSW-MAC-TableClear");
   system ("ln -sf ./ethswbox fapi-GSW-MAC-TableClear-Cond");
   system ("ln -sf ./ethswbox fapi-GSW-CfgGet");
   system ("ln -sf ./ethswbox fapi-GSW-CfgSet");
   system ("ln -sf ./ethswbox fapi-GSW-MAC-TableEntryRemove");
   system ("ln -sf ./ethswbox fapi-GSW-MAC-TableEntryQuery");
   system ("ln -sf ./ethswbox fapi-GSW-QoS-FlowctrlCfgGet");
   system ("ln -sf ./ethswbox fapi-GSW-QoS-FlowctrlCfgSet");
   system ("ln -sf ./ethswbox fapi-GSW-QoS-FlowctrlPortCfgGet");
   system ("ln -sf ./ethswbox fapi-GSW-QoS-FlowctrlPortCfgSet");
   system ("ln -sf ./ethswbox fapi-GSW-MAC-TableEntryAdd");
   system ("ln -sf ./ethswbox fapi-GSW-MAC-TableEntryRead");
   system ("ln -sf ./ethswbox fapi-GSW-QoS-QueuePortSet");
   system ("ln -sf ./ethswbox fapi-GSW-QoS-QueuePortGet");
   system ("ln -sf ./ethswbox fapi-GSW-BridgePortConfigGet");
   system ("ln -sf ./ethswbox fapi-GSW-BridgePortConfigSet");
   system ("ln -sf ./ethswbox fapi-GSW-CtpPortConfigGet");
   system ("ln -sf ./ethswbox fapi-GSW-CtpPortConfigSet");
   system ("ln -sf ./ethswbox fapi-int-gphy-read");
   system ("ln -sf ./ethswbox fapi-int-gphy-write");
   system ("ln -sf ./ethswbox fapi-ext-mdio-read");
   system ("ln -sf ./ethswbox fapi-ext-mdio-write");
   system ("ln -sf ./ethswbox fapi-int-gphy-mod");
   system ("ln -sf ./ethswbox fapi-ext-mdio-mod");
   system ("ln -sf ./ethswbox fapi-GSW-BridgeConfigGet");
   system ("ln -sf ./ethswbox fapi-GSW-BridgeConfigSet");
   system ("ln -sf ./ethswbox fapi-GSW-BridgeFree");
   system ("ln -sf ./ethswbox fapi-GSW-BridgeAlloc");
   system ("ln -sf ./ethswbox fapi-GSW-ExtendedVlanGet");
   system ("ln -sf ./ethswbox fapi-GSW-ExtendedVlanSet");
   system ("ln -sf ./ethswbox fapi-GSW-ExtendedVlanFree");
   system ("ln -sf ./ethswbox fapi-GSW-ExtendedVlanAlloc");
   system ("ln -sf ./ethswbox fapi-GSW-VlanFilterGet");
   system ("ln -sf ./ethswbox fapi-GSW-VlanFilterSet");
   system ("ln -sf ./ethswbox fapi-GSW-VlanFilterFree");
   system ("ln -sf ./ethswbox fapi-GSW-VlanFilterAlloc");
   system ("ln -sf ./ethswbox fapi-GSW-STP-PortCfgGet");
   system ("ln -sf ./ethswbox fapi-GSW-STP-PortCfgSet");
   system ("ln -sf ./ethswbox fapi-GSW-STP-BPDU-RuleSet");
   system ("ln -sf ./ethswbox fapi-GSW-STP-BPDU-RuleGet");
   system ("ln -sf ./ethswbox fapi-GSW-Debug-RMON-Port-Get");

   system ("ln -sf ./ethswbox fapi-GSW-QoS-MeterCfgGet");
   system ("ln -sf ./ethswbox fapi-GSW-QoS-MeterCfgSet");
   system ("ln -sf ./ethswbox fapi-GSW-MAC-DefaultFilterGet");
   system ("ln -sf ./ethswbox fapi-GSW-MAC-DefaultFilterSet");
   system ("ln -sf ./ethswbox fapi-GSW-CTP-PortAssignmentGet");
   system ("ln -sf ./ethswbox fapi-GSW-CTP-PortAssignmentSet");

   system ("ln -sf ./ethswbox fapi-GSW-PMAC-GLBL-CfgSet");
   system ("ln -sf ./ethswbox fapi-GSW-PMAC-GLBL-CfgGet");
   system ("ln -sf ./ethswbox fapi-GSW-PMAC-BM-CfgSet");
   system ("ln -sf ./ethswbox fapi-GSW-PMAC-BM-CfgGet");
   system ("ln -sf ./ethswbox fapi-GSW-PMAC-EG-CfgSet");
   system ("ln -sf ./ethswbox fapi-GSW-PMAC-EG-CfgGet");
   system ("ln -sf ./ethswbox fapi-GSW-PMAC-IG-CfgGet");
   system ("ln -sf ./ethswbox fapi-GSW-PMAC-IG-CfgSet");

   system ("ln -sf ./ethswbox fapi-GSW-PceRuleDelete");
   system ("ln -sf ./ethswbox fapi-GSW-PceRuleRead");
   system ("ln -sf ./ethswbox fapi-GSW-PceRuleWrite");
   system ("ln -sf ./ethswbox fapi-GSW-PceRuleAlloc");
   system ("ln -sf ./ethswbox fapi-GSW-PceRuleFree");
   system ("ln -sf ./ethswbox fapi-GSW-PceRuleEnable");
   system ("ln -sf ./ethswbox fapi-GSW-PceRuleDisable");

   system ("ln -sf ./ethswbox fapi-GSW-MulticastRouterPortAdd");
   system ("ln -sf ./ethswbox fapi-GSW-MulticastRouterPortRemove");
   system ("ln -sf ./ethswbox fapi-GSW-MulticastSnoopCfgGet");
   system ("ln -sf ./ethswbox fapi-GSW-MulticastSnoopCfgSet");
   system ("ln -sf ./ethswbox fapi-GSW-MulticastRouterPortRead");
   system ("ln -sf ./ethswbox fapi-GSW-MulticastTableEntryAdd");
   system ("ln -sf ./ethswbox fapi-GSW-MulticastTableEntryRead");
   system ("ln -sf ./ethswbox fapi-GSW-MulticastTableEntryRemove");

   system ("ln -sf ./ethswbox fapi-GSW-FW-Update");
   system ("ln -sf ./ethswbox fapi-GSW-FW-Version");
   system ("ln -sf ./ethswbox fapi-GSW-PVT-Meas");
   system ("ln -sf ./ethswbox fapi-GSW-Delay");
   system ("ln -sf ./ethswbox fapi-GSW-GPIO-Configure");
   system ("ln -sf ./ethswbox fapi-GSW-Reboot");
   system ("ln -sf ./ethswbox fapi-GSW-SysReg-Rd");

   system ("ln -sf ./ethswbox fapi-GSW-VlanCounterMapSet");
   system ("ln -sf ./ethswbox fapi-GSW-VlanCounterMapGet");
   system ("ln -sf ./ethswbox fapi-GSW-Vlan-RMON-Get");
   system ("ln -sf ./ethswbox fapi-GSW-Vlan-RMON-Clear");
   system ("ln -sf ./ethswbox fapi-GSW-Vlan-RMONControl-Set");
   system ("ln -sf ./ethswbox fapi-GSW-Vlan-RMONControl-Get");

   system ("ln -sf ./ethswbox fapi-GSW-PBB-TunnelTempate-Config-Set");
   system ("ln -sf ./ethswbox fapi-GSW-PBB-TunnelTempate-Config-Get");
   system ("ln -sf ./ethswbox fapi-GSW-PBB-TunnelTempate-Alloc");
   system ("ln -sf ./ethswbox fapi-GSW-PBB-TunnelTempate-Free");
   // system ("ln -sf ./ethswbox fapi-GSW-Debug-RMON-Port-GetAll");
   system ("ln -sf ./ethswbox fapi-GSW-CPU-PortCfgSet");
   system ("ln -sf ./ethswbox fapi-GSW-CPU-PortCfgGet");

   system ("ln -sf ./ethswbox fapi-GSW-RMON-MeterGet");
   system ("ln -sf ./ethswbox fapi-GSW-RMON-FlowGet");
   system ("ln -sf ./ethswbox fapi-GSW-RMON-TFlowClear");

   system ("ln -sf ./ethswbox fapi-GSW-BridgePortAlloc");
   system ("ln -sf ./ethswbox fapi-GSW-BridgePortFree");
   system ("ln -sf ./ethswbox fapi-GSW-Freeze");
   system ("ln -sf ./ethswbox fapi-GSW-UnFreeze");

   system ("ln -sf ./ethswbox fapi-GSW-QoS-MeterFree");
   system ("ln -sf ./ethswbox fapi-GSW-QoS-MeterAlloc");

   system ("ln -sf ./ethswbox fapi-GSW-PMAC-RMON-Get");
   system ("ln -sf ./ethswbox fapi-GSW-Debug-PMAC-RMON-Get-All");

   return OS_SUCCESS;
}


static void cmds_fapi_help (void)
{
   printf ("+-----------------------------------------------------------------------+\n");
   printf ("|                           HELP !                                      |\n");
   printf ("|                        CMDS - gsw                                     |\n");
   printf ("|                                                                       |\n");
   printf ("+-----------------------------------------------------------------------+\n");
   printf ("| fapi_GSW_RegisterGet                                                  |\n");
   printf ("\n");
}
