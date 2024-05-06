/******************************************************************************

   Copyright 2023-2024 MaxLinear, Inc.

   For licensing information, see the file 'LICENSE' in the root folder of
   this software module.

******************************************************************************/

#ifndef MXL_GSW_API_H_
#define MXL_GSW_API_H_

#include "gsw.h"

GSW_return_t GSW_RegisterGet(const GSW_Device_t *dev, GSW_register_t *);
GSW_return_t GSW_RegisterSet(const GSW_Device_t *dev, GSW_register_t *);
GSW_return_t GSW_RegisterMod(const GSW_Device_t *dev, GSW_register_mod_t *);
GSW_return_t GSW_CPU_PortCfgSet(const GSW_Device_t *dev, GSW_CPU_PortCfg_t *);
GSW_return_t GSW_CPU_PortCfgGet(const GSW_Device_t *dev, GSW_CPU_PortCfg_t *);
GSW_return_t GSW_PortLinkCfgSet(const GSW_Device_t *dev, GSW_portLinkCfg_t *);
GSW_return_t GSW_PortLinkCfgGet(const GSW_Device_t *dev, GSW_portLinkCfg_t *);
GSW_return_t GSW_PortCfgSet(const GSW_Device_t *dev, GSW_portCfg_t *);
GSW_return_t GSW_PortCfgGet(const GSW_Device_t *dev, GSW_portCfg_t *);
GSW_return_t GSW_CfgSet(const GSW_Device_t *dev, GSW_cfg_t *);
GSW_return_t GSW_CfgGet(const GSW_Device_t *dev, GSW_cfg_t *);
GSW_return_t GSW_MonitorPortCfgGet(const GSW_Device_t *dev, GSW_monitorPortCfg_t *);
GSW_return_t GSW_MonitorPortCfgSet(const GSW_Device_t *dev, GSW_monitorPortCfg_t *);
GSW_return_t GSW_Freeze(const GSW_Device_t *dev);
GSW_return_t GSW_UnFreeze(const GSW_Device_t *dev);

/* TFLOW */
GSW_return_t GSW_PceRuleAlloc(const GSW_Device_t *dev, GSW_PCE_rule_alloc_t *parm);
GSW_return_t GSW_PceRuleFree(const GSW_Device_t *dev, GSW_PCE_rule_alloc_t *parm);
GSW_return_t GSW_PceRuleEnable(const GSW_Device_t *dev, GSW_PCE_ruleEntry_t *parm);
GSW_return_t GSW_PceRuleDisable(const GSW_Device_t *dev, GSW_PCE_ruleEntry_t *parm);
GSW_return_t GSW_PceRuleRead(const GSW_Device_t *dev, GSW_PCE_rule_t *);
GSW_return_t GSW_PceRuleWrite(const GSW_Device_t *dev, GSW_PCE_rule_t *);
GSW_return_t GSW_PceRuleDelete(const GSW_Device_t *dev, GSW_PCE_ruleEntry_t *);
GSW_return_t GSW_PceRuleAlloc(const GSW_Device_t *dev, GSW_PCE_rule_alloc_t *);
GSW_return_t GSW_PceRuleFree(const GSW_Device_t *dev, GSW_PCE_rule_alloc_t *);
GSW_return_t GSW_PceRuleEnable(const GSW_Device_t *dev, GSW_PCE_ruleEntry_t *);
GSW_return_t GSW_PceRuleDisable(const GSW_Device_t *dev, GSW_PCE_ruleEntry_t *);
GSW_return_t GSW_DumpTable(const GSW_Device_t *dev, GSW_table_t *);

/* BM Table */

/* Bridge */
GSW_return_t GSW_BridgeAlloc(const GSW_Device_t *dev, GSW_BRIDGE_alloc_t *);
GSW_return_t GSW_BridgeFree(const GSW_Device_t *dev, GSW_BRIDGE_alloc_t *);
GSW_return_t GSW_BridgeConfigSet(const GSW_Device_t *dev, GSW_BRIDGE_config_t *);
GSW_return_t GSW_BridgeConfigGet(const GSW_Device_t *dev, GSW_BRIDGE_config_t *);

/*Bridge Port*/
GSW_return_t GSW_BridgePortAlloc(const GSW_Device_t *dev, GSW_BRIDGE_portAlloc_t *);
GSW_return_t GSW_BridgePortConfigSet(const GSW_Device_t *dev, GSW_BRIDGE_portConfig_t *);
GSW_return_t GSW_BridgePortConfigGet(const GSW_Device_t *dev, GSW_BRIDGE_portConfig_t *);
GSW_return_t GSW_BridgePortFree(const GSW_Device_t *dev, GSW_BRIDGE_portAlloc_t *);

/* CTP Port */
GSW_return_t GSW_CTP_PortAssignmentAlloc(const GSW_Device_t *, GSW_CTP_portAssignment_t *);
GSW_return_t GSW_CTP_PortAssignmentFree(const GSW_Device_t *, GSW_CTP_portAssignment_t *);
GSW_return_t GSW_CTP_PortAssignmentSet(const GSW_Device_t *, GSW_CTP_portAssignment_t *);
GSW_return_t GSW_CTP_PortAssignmentGet(const GSW_Device_t *, GSW_CTP_portAssignment_t *);
GSW_return_t GSW_CtpPortConfigSet(const GSW_Device_t *, GSW_CTP_portConfig_t *);
GSW_return_t GSW_CtpPortConfigGet(const GSW_Device_t *, GSW_CTP_portConfig_t *);
GSW_return_t GSW_CtpPortConfigReset(const GSW_Device_t *, GSW_CTP_portConfig_t *);

/* QoS */
GSW_return_t GSW_QoS_MeterCfgSet(const GSW_Device_t *, GSW_QoS_meterCfg_t *);
GSW_return_t GSW_QoS_MeterCfgGet(const GSW_Device_t *, GSW_QoS_meterCfg_t *);
GSW_return_t GSW_QoS_DSCP_ClassGet(const GSW_Device_t *, GSW_QoS_DSCP_ClassCfg_t *);
GSW_return_t GSW_QoS_DSCP_ClassSet(const GSW_Device_t *, GSW_QoS_DSCP_ClassCfg_t *);
GSW_return_t GSW_QoS_DSCP_DropPrecedenceCfgSet(const GSW_Device_t *,
		GSW_QoS_DSCP_DropPrecedenceCfg_t *);
GSW_return_t GSW_QoS_DSCP_DropPrecedenceCfgGet(const GSW_Device_t *,
		GSW_QoS_DSCP_DropPrecedenceCfg_t *);
GSW_return_t GSW_QoS_PortRemarkingCfgSet(const GSW_Device_t *,
		GSW_QoS_portRemarkingCfg_t *);
GSW_return_t GSW_QoS_PortRemarkingCfgGet(const GSW_Device_t *,
		GSW_QoS_portRemarkingCfg_t *);

GSW_return_t GSW_QoS_PCP_ClassGet(const GSW_Device_t *,
				  GSW_QoS_PCP_ClassCfg_t *);
GSW_return_t GSW_QoS_PCP_ClassSet(const GSW_Device_t *,
				  GSW_QoS_PCP_ClassCfg_t *);
GSW_return_t GSW_QoS_PortCfgGet(const GSW_Device_t *, GSW_QoS_portCfg_t *);
GSW_return_t GSW_QoS_PortCfgSet(const GSW_Device_t *, GSW_QoS_portCfg_t *);
GSW_return_t GSW_QoS_SchedulerCfgSet(const GSW_Device_t *,
				     GSW_QoS_schedulerCfg_t *);
GSW_return_t GSW_QoS_SchedulerCfgGet(const GSW_Device_t *,
				     GSW_QoS_schedulerCfg_t *);
GSW_return_t GSW_QoS_ShaperCfgSet(const GSW_Device_t *,
				  GSW_QoS_ShaperCfg_t *);
GSW_return_t GSW_QoS_ShaperCfgGet(const GSW_Device_t *,
				  GSW_QoS_ShaperCfg_t *);
GSW_return_t GSW_QoS_ShaperQueueAssign(const GSW_Device_t *,
				       GSW_QoS_ShaperQueue_t *);
GSW_return_t GSW_QoS_ShaperQueueDeassign(const GSW_Device_t *,
		GSW_QoS_ShaperQueue_t *);
GSW_return_t GSW_QoS_ShaperQueueGet(const GSW_Device_t *,
				    GSW_QoS_ShaperQueueGet_t *);
GSW_return_t GSW_QoS_StormCfgSet(const GSW_Device_t *,
				 GSW_QoS_stormCfg_t *);
GSW_return_t GSW_QoS_StormCfgGet(const GSW_Device_t *,
				 GSW_QoS_stormCfg_t *);
GSW_return_t GSW_QoS_WredCfgSet(const GSW_Device_t *,
				GSW_QoS_WRED_Cfg_t *);
GSW_return_t GSW_QoS_WredCfgGet(const GSW_Device_t *,
				GSW_QoS_WRED_Cfg_t *);
GSW_return_t GSW_QoS_WredQueueCfgSet(const GSW_Device_t *,
				     GSW_QoS_WRED_QueueCfg_t *);
GSW_return_t GSW_QoS_WredQueueCfgGet(const GSW_Device_t *,
				     GSW_QoS_WRED_QueueCfg_t *);
GSW_return_t GSW_QoS_WredPortCfgSet(const GSW_Device_t *,
				    GSW_QoS_WRED_PortCfg_t *);
GSW_return_t GSW_QoS_WredPortCfgGet(const GSW_Device_t *,
				    GSW_QoS_WRED_PortCfg_t *);
GSW_return_t GSW_QoS_FlowctrlCfgSet(const GSW_Device_t *,
				    GSW_QoS_FlowCtrlCfg_t *);
GSW_return_t GSW_QoS_FlowctrlCfgGet(const GSW_Device_t *,
				    GSW_QoS_FlowCtrlCfg_t *);

GSW_return_t GSW_QoS_FlowctrlPortCfgSet(const GSW_Device_t *,
					GSW_QoS_FlowCtrlPortCfg_t *);
GSW_return_t GSW_QoS_FlowctrlPortCfgGet(const GSW_Device_t *,
					GSW_QoS_FlowCtrlPortCfg_t *);
GSW_return_t GSW_QoS_QueueBufferReserveCfgSet(const GSW_Device_t *,
		GSW_QoS_QueueBufferReserveCfg_t *);
GSW_return_t GSW_QoS_QueueBufferReserveCfgGet(const GSW_Device_t *,
		GSW_QoS_QueueBufferReserveCfg_t *);
GSW_return_t GSW_QOS_ColorMarkingTableSet(const GSW_Device_t *, GSW_QoS_colorMarkingEntry_t *);
GSW_return_t GSW_QOS_ColorMarkingTableGet(const GSW_Device_t *, GSW_QoS_colorMarkingEntry_t *);
GSW_return_t GSW_QOS_ColorReMarkingTableSet(const GSW_Device_t *, GSW_QoS_colorRemarkingEntry_t *);
GSW_return_t GSW_QOS_ColorReMarkingTableGet(const GSW_Device_t *, GSW_QoS_colorRemarkingEntry_t *);
GSW_return_t GSW_QOS_MeterAlloc(const GSW_Device_t *, GSW_QoS_meterCfg_t *);
GSW_return_t GSW_QOS_MeterFree(const GSW_Device_t *, GSW_QoS_meterCfg_t *);
GSW_return_t GSW_QOS_Dscp2PcpTableSet(const GSW_Device_t *, GSW_DSCP2PCP_map_t *);
GSW_return_t GSW_QOS_Dscp2PcpTableGet(const GSW_Device_t *, GSW_DSCP2PCP_map_t *);
GSW_return_t GSW_QOS_PmapperTableSet(const GSW_Device_t *, GSW_PMAPPER_t *);
GSW_return_t GSW_QOS_PmapperTableGet(const GSW_Device_t *, GSW_PMAPPER_t *);
GSW_return_t GSW_QoS_SVLAN_PCP_ClassGet(const GSW_Device_t *,
					GSW_QoS_SVLAN_PCP_ClassCfg_t *);
GSW_return_t GSW_QoS_SVLAN_PCP_ClassSet(const GSW_Device_t *,
					GSW_QoS_SVLAN_PCP_ClassCfg_t *);
GSW_return_t GSW_QOS_PmapperTableSet(const GSW_Device_t *, GSW_PMAPPER_t *);
GSW_return_t GSW_QOS_PmapperTableGet(const GSW_Device_t *, GSW_PMAPPER_t *);
GSW_return_t GSW_QoS_QueuePortSet(const GSW_Device_t *, GSW_QoS_queuePort_t *);
GSW_return_t GSW_QoS_QueuePortGet(const GSW_Device_t *, GSW_QoS_queuePort_t *);

/* RMON */
GSW_return_t GSW_RMON_Port_Get(const GSW_Device_t *, GSW_RMON_Port_cnt_t *);
GSW_return_t GSW_RMON_Mode_Set(const GSW_Device_t *, GSW_RMON_mode_t *);
GSW_return_t GSW_RMON_Meter_Get(const GSW_Device_t *, GSW_RMON_Meter_cnt_t *);
GSW_return_t GSW_RMON_Clear(const GSW_Device_t *, GSW_RMON_clear_t *);
GSW_return_t GSW_RmonTflowClear(const GSW_Device_t *, GSW_RMON_flowGet_t *);
GSW_return_t GSW_RMON_FlowGet(const GSW_Device_t *, GSW_RMON_flowGet_t *);
GSW_return_t GSW_RmonTflowClear(const GSW_Device_t *, GSW_RMON_flowGet_t *);
GSW_return_t GSW_TflowCountModeSet(const GSW_Device_t *, GSW_TflowCmodeConf_t *);
GSW_return_t GSW_TflowCountModeGet(const GSW_Device_t *, GSW_TflowCmodeConf_t *);

/*Debug */
GSW_return_t GSW_Debug_RMON_Port_Get(const GSW_Device_t *, GSW_Debug_RMON_Port_cnt_t *);
GSW_return_t GSW_Debug_MeterTableStatus(const GSW_Device_t *, GSW_debug_t *);

/*PMAC */
GSW_return_t GSW_PMAC_CountGet(const GSW_Device_t *, GSW_PMAC_Cnt_t *);
GSW_return_t GSW_PMAC_GLBL_CfgSet(const GSW_Device_t *, GSW_PMAC_Glbl_Cfg_t *);
GSW_return_t GSW_PMAC_GLBL_CfgGet(const GSW_Device_t *, GSW_PMAC_Glbl_Cfg_t *);
GSW_return_t GSW_PMAC_BM_CfgSet(const GSW_Device_t *, GSW_PMAC_BM_Cfg_t *);
GSW_return_t GSW_PMAC_BM_CfgGet(const GSW_Device_t *, GSW_PMAC_BM_Cfg_t *);
GSW_return_t GSW_PMAC_IG_CfgSet(const GSW_Device_t *, GSW_PMAC_Ig_Cfg_t *);
GSW_return_t GSW_PMAC_IG_CfgGet(const GSW_Device_t *, GSW_PMAC_Ig_Cfg_t *);
GSW_return_t GSW_PMAC_EG_CfgSet(const GSW_Device_t *, GSW_PMAC_Eg_Cfg_t *);
GSW_return_t GSW_PMAC_EG_CfgGet(const GSW_Device_t *, GSW_PMAC_Eg_Cfg_t *);

// #ifdef CONFIG_GSWIP_MAC
GSW_return_t GSW_MAC_TableClear(const GSW_Device_t *);
GSW_return_t GSW_MAC_TableClearCond(const GSW_Device_t *dev,
				    GSW_MAC_tableClearCond_t *parm);
GSW_return_t GSW_MAC_TableEntryAdd(const GSW_Device_t *,
				   GSW_MAC_tableAdd_t *);
GSW_return_t GSW_MAC_TableEntryQuery(const GSW_Device_t *,
				     GSW_MAC_tableQuery_t *);
GSW_return_t GSW_MAC_TableEntryRead(const GSW_Device_t *,
				    GSW_MAC_tableRead_t *);
GSW_return_t GSW_MAC_TableEntryRemove(const GSW_Device_t *,
				      GSW_MAC_tableRemove_t *);
GSW_return_t GSW_DefaultMacFilterSet(const GSW_Device_t *, GSW_MACFILTER_default_t *);
GSW_return_t GSW_DefaultMacFilterGet(const GSW_Device_t *, GSW_MACFILTER_default_t *);
// #endif

// #ifdef CONFIG_GSWIP_EVLAN
GSW_return_t GSW_ExtendedVlanAlloc(const GSW_Device_t *, GSW_EXTENDEDVLAN_alloc_t *);
GSW_return_t GSW_ExtendedVlanSet(const GSW_Device_t *, GSW_EXTENDEDVLAN_config_t *);
GSW_return_t GSW_ExtendedVlanGet(const GSW_Device_t *, GSW_EXTENDEDVLAN_config_t *);
GSW_return_t GSW_ExtendedVlanFree(const GSW_Device_t *, GSW_EXTENDEDVLAN_alloc_t *);
GSW_return_t GSW_Debug_ExvlanTableStatus(const GSW_Device_t *, GSW_debug_t *);
GSW_return_t GSW_Debug_VlanFilterTableStatus(const GSW_Device_t *, GSW_debug_t *);
GSW_return_t GSW_VlanFilterAlloc(const GSW_Device_t *, GSW_VLANFILTER_alloc_t *);
GSW_return_t GSW_VlanFilterSet(const GSW_Device_t *, GSW_VLANFILTER_config_t *);
GSW_return_t GSW_VlanFilterGet(const GSW_Device_t *, GSW_VLANFILTER_config_t *);
GSW_return_t GSW_VlanFilterFree(const GSW_Device_t *, GSW_VLANFILTER_alloc_t *);
GSW_return_t GSW_Vlan_RMONControl_Set(const GSW_Device_t *, GSW_VLAN_RMON_control_t *);
GSW_return_t GSW_Vlan_RMONControl_Get(const GSW_Device_t *, GSW_VLAN_RMON_control_t *);
GSW_return_t GSW_Vlan_RMON_Get(const GSW_Device_t *, GSW_VLAN_RMON_cnt_t *);
GSW_return_t GSW_Vlan_RMON_Clear(const GSW_Device_t *, GSW_VLAN_RMON_cnt_t *);
GSW_return_t GSW_VlanCounterMapSet(const GSW_Device_t *, GSW_VlanCounterMapping_config_t *);
GSW_return_t GSW_VlanCounterMapGet(const GSW_Device_t *, GSW_VlanCounterMapping_config_t *);
// #endif

// #ifdef CONFIG_GSWIP_MCAST
GSW_return_t GSW_MulticastRouterPortAdd(const GSW_Device_t *, GSW_multicastRouter_t *);
GSW_return_t GSW_MulticastRouterPortRead(const GSW_Device_t *, GSW_multicastRouterRead_t *);
GSW_return_t GSW_MulticastRouterPortRemove(const GSW_Device_t *, GSW_multicastRouter_t *);
GSW_return_t GSW_MulticastSnoopCfgGet(const GSW_Device_t *, GSW_multicastSnoopCfg_t *);
GSW_return_t GSW_MulticastSnoopCfgSet(const GSW_Device_t *, GSW_multicastSnoopCfg_t *);
GSW_return_t GSW_MulticastTableEntryAdd(const GSW_Device_t *, GSW_multicastTable_t *);
GSW_return_t GSW_MulticastTableEntryRead(const GSW_Device_t *, GSW_multicastTableRead_t *);
GSW_return_t GSW_MulticastTableEntryRemove(const GSW_Device_t *, GSW_multicastTable_t *);
// #endif

// #ifdef CONFIG_GSWIP_STP
GSW_return_t GSW_STP_PortCfgGet(const GSW_Device_t *cdev, GSW_STP_portCfg_t *parm);
GSW_return_t GSW_STP_PortCfgSet(const GSW_Device_t *cdev, GSW_STP_portCfg_t *parm);
GSW_return_t GSW_STP_BPDU_RuleGet(const GSW_Device_t *cdev, GSW_STP_BPDU_Rule_t *parm);
GSW_return_t GSW_STP_BPDU_RuleSet(const GSW_Device_t *cdev, GSW_STP_BPDU_Rule_t *parm);
// #endif

GSW_return_t GSW_TrunkingCfgSet(const GSW_Device_t *, GSW_trunkingCfg_t *);
GSW_return_t GSW_TrunkingCfgGet(const GSW_Device_t *, GSW_trunkingCfg_t *);

GSW_return_t GSW_PBB_TunnelTempate_Alloc(const GSW_Device_t *, GSW_PBB_Tunnel_Template_Config_t *);
GSW_return_t GSW_PBB_TunnelTempate_Free(const GSW_Device_t *, GSW_PBB_Tunnel_Template_Config_t *);
GSW_return_t GSW_PBB_TunnelTempate_Config_Set(const GSW_Device_t *, GSW_PBB_Tunnel_Template_Config_t *);
GSW_return_t GSW_PBB_TunnelTempate_Config_Get(const GSW_Device_t *, GSW_PBB_Tunnel_Template_Config_t *);

#endif /* MXL_GSW_API_H_ */

