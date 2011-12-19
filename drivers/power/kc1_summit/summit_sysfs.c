/*
 * summit_smb347.c
 * Summit SMB347 Charger Driver for KC1 Board
 *
 * Copyright (C) Quanta Computer Inc. All rights reserved.
 * 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "smb347.h"

extern int aicl_results[];
extern int pre_current[];
extern int fast_current[];
//====================================================================================================
static ssize_t summit_version_show(struct device *dev, struct device_attribute *attr,char *buf)
{
    struct summit_smb347_info *di = i2c_get_clientdata(to_i2c_client(dev));
    ssize_t result=0;
    result=sprintf(buf, "%s\n", "2011/7/9--1.1");
    return result;
}
static ssize_t summit_ondemand_show(struct device *dev, struct device_attribute *attr,char *buf)
{
    struct summit_smb347_info *di = i2c_get_clientdata(to_i2c_client(dev));
    ssize_t result=0;
    result=sprintf(buf, "%s\n", fsm_state_string(di->current_state));
    return result;
}

static ssize_t summit_ondemand_store(struct device *dev, struct device_attribute *attr,const char *buf,size_t len)
{
    struct summit_smb347_info *di = i2c_get_clientdata(to_i2c_client(dev));
    char *buffer;
    int enable=0;
    enable = (int)simple_strtoul(buf, NULL, 10);
    if(enable){
        //di->events=EVENT_CHANGE_TO_ONDEMAND;   
        summit_fsm_stateTransform(di,EVENT_CHANGE_TO_ONDEMAND);
        summit_fsm_doAction(di,EVENT_CHANGE_TO_ONDEMAND);
    }else{
        //di->event=EVENT_CHANGE_TO_INTERNAL_FSM;
        summit_fsm_stateTransform(di,EVENT_CHANGE_TO_INTERNAL_FSM);
        summit_fsm_doAction(di,EVENT_CHANGE_TO_INTERNAL_FSM);     
    } 
    return len;
}

static ssize_t summit_apsd_setting_show(struct device *dev, struct device_attribute *attr,char *buf)
{
    struct summit_smb347_info *di = i2c_get_clientdata(to_i2c_client(dev));
    ssize_t result=0;
    int config=0;
    config=i2c_smbus_read_byte_data(di->client,SUMMIT_SMB347_CHARGE_CONTROL);
    if(IS_APSD_ENABLE(config))
        result=sprintf(buf, "%s\n", "APSD_ENABLE");
    else
        result=sprintf(buf, "%s\n", "APSD_DISABLE");
    
    return result;
}

static ssize_t summit_apsd_setting_store(struct device *dev, struct device_attribute *attr,const char *buf,size_t len)
{
    struct summit_smb347_info *di = i2c_get_clientdata(to_i2c_client(dev));
    char *buffer;
    int enable=0;
    enable = (int)simple_strtoul(buf, NULL, 10);
    summit_config_apsd(di,enable);
    return len;
}

static ssize_t summit_apsd_status_show(struct device *dev, struct device_attribute *attr,char *buf,size_t len)
{
    struct summit_smb347_info *di = i2c_get_clientdata(to_i2c_client(dev));
    ssize_t result=0;
    int status=0;
    int apsd=0;
    status=i2c_smbus_read_byte_data(di->client,SUMMIT_SMB347_INTSTAT_REG_D);
    apsd=status;
    if(APSD_STATUS(status)){
        switch(APSD_RESULTS(apsd)){
            case APSD_NOT_RUN:
                result=sprintf(buf, "%s \n", "APSD Not run");
            break;
            case APSD_CHARGING_DOWNSTREAM_PORT:
                result=sprintf(buf, "%s", "Charging Downstream Port\n");
            break;
            case APSD_DEDICATED_DOWNSTREAM_PORT:
                result=sprintf(buf, "%s", "Dedicated Charging Port\n");
            break;
            case APSD_OTHER_DOWNSTREAM_PORT:
                result=sprintf(buf, "%s", "Other Charging Port\n");
            break;
            case APSD_STANDARD_DOWNSTREAM_PORT:
                result=sprintf(buf, "%s", "Standard Downstream Port\n");
            break;
            case APSD_ACA_CHARGER:
                result=sprintf(buf, "%s", "ACA Charger\n");
            break;
            case APSD_TBD:
                result=sprintf(buf, "%s", "TBD\n");
            break;
            default:
                result=sprintf(buf, "%s", "");
            break;
        }
    }else
        result=sprintf(buf, "%s\n", "APSD NOt Completed\n");
    return result;
}

static ssize_t summit_aicl_setting_show(struct device *dev, struct device_attribute *attr,char *buf)
{
    struct summit_smb347_info *di = i2c_get_clientdata(to_i2c_client(dev));
    ssize_t result=0;
    int config=0;
    config=i2c_smbus_read_byte_data(di->client,SUMMIT_SMB347_FUNCTIONS);
    if(IS_AICL_ENABLE(config))
        result=sprintf(buf, "%s\n", "AICL ENABLE");
    else
        result=sprintf(buf, "%s\n", "AICL DISABLE");
    return result;
}

static ssize_t summit_aicl_setting_store(struct device *dev, struct device_attribute *attr,const char *buf,size_t len)
{
    struct summit_smb347_info *di = i2c_get_clientdata(to_i2c_client(dev));
    int config=0;
    char *buffer;
    int enable=0;
    config=i2c_smbus_read_byte_data(di->client,2);
    enable = (int)simple_strtoul(buf, NULL, 10);
    summit_config_aicl(di,enable,4200);
    return len;
}

static ssize_t summit_aicl_status_show(struct device *dev, struct device_attribute *attr,char *buf,size_t len)
{
    struct summit_smb347_info *di = i2c_get_clientdata(to_i2c_client(dev));
    ssize_t result=0;
    int status=0;
    int aicl=0;
    status=i2c_smbus_read_byte_data(di->client,SUMMIT_SMB347_STATUS_REG_E);
    aicl=status;
    if(AICL_STATUS(status)){
        result=sprintf(buf, "AICL Completed:%d mA\n", aicl_results[AICL_RESULT(aicl)]);
    }else
        result=sprintf(buf, "%s", "AICL NOt Completed\n");
    return result;
}

static ssize_t summit_mode_show(struct device *dev, struct device_attribute *attr,char *buf)
{
    struct summit_smb347_info *di = i2c_get_clientdata(to_i2c_client(dev));
    ssize_t result=0;
    int command=0;
    command=i2c_smbus_read_byte_data(di->client,SUMMIT_SMB347_COMMAND_REG_B);
    if(IS_BIT_SET(command,0))
        result=sprintf(buf, "%s\n", "HC_MODE\n");
    else{
        if(IS_BIT_SET(command,1))
            result=sprintf(buf, "%s\n", "USB5_MODE\n");
        else
            result=sprintf(buf, "%s\n", "HC_MODE\n");
    }
    return result;
}

static ssize_t summit_mode_store(struct device *dev, struct device_attribute *attr,const char *buf,size_t len)
{
    struct summit_smb347_info *di = i2c_get_clientdata(to_i2c_client(dev));
    int mode = (int)simple_strtoul(buf, NULL, 10);
    summit_switch_mode(di,mode);
    return len;
}

static ssize_t summit_mode_status_show(struct device *dev, struct device_attribute *attr,char *buf,size_t len)
{
    struct summit_smb347_info *di = i2c_get_clientdata(to_i2c_client(dev));
    ssize_t result=0;
    int status=0;
    status=i2c_smbus_read_byte_data(di->client,SUMMIT_SMB347_STATUS_REG_E);
    if(USB15_HC_MODE(status)==USB1_OR_15_MODE)
        result=sprintf(buf, "%s\n", "USB1_MODE\n");
    if(USB15_HC_MODE(status)==USB5_OR_9_MODE)
        result=sprintf(buf, "%s\n", "USB5_MODE\n");
    if(USB15_HC_MODE(status)==HC_MODE)
        result=sprintf(buf, "%s\n", "HC_MODE\n");
    return result;
}

static ssize_t summit_charge_show(struct device *dev, struct device_attribute *attr,char *buf)
{
	struct summit_smb347_info *di = i2c_get_clientdata(to_i2c_client(dev));
	int value = 0;

	value = i2c_smbus_read_byte_data(di->client, SUMMIT_SMB347_STATUS_REG_C);

	if (value < 0) {
		return sprintf(buf, "-1\n");
	} else {
		return sprintf(buf, "%d\n", IS_BIT_SET(value, 0));
	}
}

static ssize_t summit_charge_store(struct device *dev, struct device_attribute *attr,const char *buf,size_t len)
{
	int value = 0;
	struct summit_smb347_info *di = i2c_get_clientdata(to_i2c_client(dev));

	/* Don't perform anything for anything below DVT */
	if (di->mbid < 4)
		return len;

	value = simple_strtoul(buf, NULL, 10);

	if (value) {
		if (di->mbid >= 5) {
			/* Unit is PVT, disable charging if battery is bad */
			if (di->bad_battery) {
				summit_charge_enable(di, 0);
			} else {
				summit_charge_enable(di, 1);
			}
		} else {
			summit_charge_enable(di, 1);
		}
	} else {
		summit_charge_enable(di, 0);
	}

	return len;
}

static ssize_t summit_charge_status_show(struct device *dev, struct device_attribute *attr,char *buf,size_t len)
{
    struct summit_smb347_info *di = i2c_get_clientdata(to_i2c_client(dev));
    ssize_t result=0;
    int status=0;
    status=i2c_smbus_read_byte_data(di->client,SUMMIT_SMB347_STATUS_REG_C);
    result=sprintf(buf, "%s %s %s %s %s %s\n", CHARGER_ERROR_STATUS(status)?"Charger error\n":"Charger no error\n",
                                CHARGEING_ENABLE_DISABLE_STATUS(status)?"Charger Enabled\n":"Charger Disabled\n",
                                CHARGEING_STATUS(status)==NO_CHARGING_STATUS?"No charging\n":"",
                                CHARGEING_STATUS(status)==PRE_CHARGING_STATUS?"Pre-charging\n":"",
                                CHARGEING_STATUS(status)==FAST_CHARGING_STATUS?"Fast-charging\n":"",
                                CHARGEING_STATUS(status)==TAPER_CHARGING_STATUS?"Taper-charging\n":"");
    return result;
}

static ssize_t summit_discharge_show(struct device *dev, struct device_attribute *attr,char *buf)
{
    struct summit_smb347_info *di = i2c_get_clientdata(to_i2c_client(dev));
    return sprintf(buf, "%d\n", di->flag_discharge);
}

static ssize_t summit_discharge_store(struct device *dev, struct device_attribute *attr,const char *buf,size_t len)
{
    struct summit_smb347_info *di = i2c_get_clientdata(to_i2c_client(dev));
    di->flag_discharge = simple_strtoul(buf, NULL, 10);
    if(di->flag_discharge==1){
    	summit_suspend_mode(di,1);
    }else{
    	summit_suspend_mode(di,0);
    }
    return len;
}

static ssize_t summit_status_show(struct device *dev, struct device_attribute *attr,char *buf)
{
    struct summit_smb347_info *di = i2c_get_clientdata(to_i2c_client(dev));
    int index=0;
    int setting=0;    
    for(index=0;index<=0x0d;index++){
        setting=i2c_smbus_read_byte_data(di->client,index);
        dev_dbg(di->dev,"%s: R%d : setting = 0x%x ",__func__,index,setting);
        mdelay(1);
    }    
    summit_read_fault_interrupt_setting(di);
    summit_read_status_interrupt_setting(di);
    summit_read_interrupt_a(di);
    summit_read_interrupt_b(di);
    summit_read_interrupt_c(di);
    summit_read_interrupt_d(di);
    summit_read_interrupt_e(di);
    summit_read_status_a(di);
    summit_read_status_b(di);
    summit_read_status_c(di);
    summit_read_status_d(di);
    summit_read_status_e(di);
    return sprintf(buf, "%d\n", di->flag_discharge);
}

static ssize_t summit_present_show(struct device *dev, struct device_attribute *attr,char *buf)
{
    struct summit_smb347_info *di = i2c_get_clientdata(to_i2c_client(dev));
    int config=0;
    int result=0;
    config=i2c_smbus_read_byte_data(di->client,0xe);
    dev_dbg(di->dev,"config=x0%x \n",config);
    if((config>>1) == 0x06)
        result=1;
    else
        result=0;
    return sprintf(buf, "%d\n", result);
}
static ssize_t summit_fake_disconnect_store(struct device *dev, struct device_attribute *attr,const char *buf,size_t len)
{
    struct summit_smb347_info *di = i2c_get_clientdata(to_i2c_client(dev));
    char *buffer;
    int value = simple_strtoul(buf, NULL, 10);
    if(value==1){
        di->fake_disconnect=1;
    	blocking_notifier_call_chain(&di->xceiv->notifier,USB_EVENT_NONE, di->xceiv->gadget);
    }
    
    return len;
}
static ssize_t summit_protect_enable_show(struct device *dev, struct device_attribute *attr,char *buf)
{
    struct summit_smb347_info *di = i2c_get_clientdata(to_i2c_client(dev));
    ssize_t result=0;
    result=sprintf(buf, " Over charge protection:%s(%d)\n Gas gauge i2c error protection:%s(%d)\n Battery recognize protection:%s(%d)\n NTC error protection:%s(%d)\n Battert hard cold charge protection:%s(%d)\n Battert hard cold discharge protection:%s(%d)\n Battert hard hot charge protection:%s(%d)\n Battert charge current adjust protection:%s(%d)\n Battert no suit charge current protection:%s(%d)\n Battert weak protection:%s(%d)\n", 
				IS_BIT_SET(di->protect_enable,0)?"enable":"disable",IS_BIT_SET(di->protect_event,0),
                                IS_BIT_SET(di->protect_enable,1)?"enable":"disable",IS_BIT_SET(di->protect_event,1),
                                IS_BIT_SET(di->protect_enable,2)?"enable":"disable",IS_BIT_SET(di->protect_event,2),
                                IS_BIT_SET(di->protect_enable,3)?"enable":"disable",IS_BIT_SET(di->protect_event,3),
                                IS_BIT_SET(di->protect_enable,4)?"enable":"disable",IS_BIT_SET(di->protect_event,4),
                                IS_BIT_SET(di->protect_enable,5)?"enable":"disable",IS_BIT_SET(di->protect_event,5),
                                IS_BIT_SET(di->protect_enable,6)?"enable":"disable",IS_BIT_SET(di->protect_event,6),
                                IS_BIT_SET(di->protect_enable,7)?"enable":"disable",IS_BIT_SET(di->protect_event,7),
                                IS_BIT_SET(di->protect_enable,8)?"enable":"disable",IS_BIT_SET(di->protect_event,8),
                                IS_BIT_SET(di->protect_enable,9)?"enable":"disable",IS_BIT_SET(di->protect_event,9));
    return result;
}
/*
Bit0 : Over charge protection
Bit1 : Gas gauge i2c error protection
Bit2 : Battery recognize protection
Bit3 : NTC error protection
Bit4 : Battert hard cold charge protection
Bit5 : Battert hard cold discharge protection
Bit6 : Battert hard hot charge protection
Bit7 : Battert charge current adjust protection
Bit8 : Battert no suit charge current protection
Bit9 : Battert too weak protection
*/
static ssize_t summit_protect_enable_store(struct device *dev, struct device_attribute *attr,const char *buf,size_t len)
{
    struct summit_smb347_info *di = i2c_get_clientdata(to_i2c_client(dev));
    char *buffer;
    int value = simple_strtoul(buf, NULL, 10);
    printk("value=%d\n",value);
    if(value>=1 && value <=10){
        value=value-1;
        SET_BIT(di->protect_enable,value);
    }
    summit_fsm_stateTransform(di,EVENT_RECHECK_PROTECTION); 
    return len;
}
static ssize_t summit_protect_disable_store(struct device *dev, struct device_attribute *attr,const char *buf,size_t len)
{
    struct summit_smb347_info *di = i2c_get_clientdata(to_i2c_client(dev));
    char *buffer;
    int value = simple_strtoul(buf, NULL, 10);
    printk("value=%d\n",value);
    if(value>=1 && value <=10){
       value=value-1;
    	CLEAR_BIT(di->protect_enable,value);
    }
    if(di->protect_enable==0){
        summit_config_charge_voltage(di,CONFIG_NO_CHANGE,FLOAT_VOLTAGE_4_2_0);
        summit_config_charge_current(di,FCC_2500mA,PCC_150mA,TC_250mA);
        summit_charge_enable(di,1);
    }
    summit_fsm_stateTransform(di,EVENT_RECHECK_PROTECTION);
    return len;
}
static ssize_t summit_float_voltage_show(struct device *dev, struct device_attribute *attr,char *buf)
{
	int config = 0;
	struct summit_smb347_info *di = i2c_get_clientdata(to_i2c_client(dev));

	config = i2c_smbus_read_byte_data(di->client, SUMMIT_SMB347_FLOAT_VOLTAGE);

	if (config < 0) {
		return sprintf(buf, "-1\n");
	} else {
		config &= 0x3f;

		if (config >= 50)
			config = 50;

		return sprintf(buf, "%d\n", 3500 + config * 20);
	}
}

static ssize_t summit_float_voltage_store(struct device *dev, struct device_attribute *attr,const char *buf,size_t len)
{
	struct summit_smb347_info *di = i2c_get_clientdata(to_i2c_client(dev));
	int value = simple_strtoul(buf, NULL, 10);

	if (value < 3500 || value > 4500)
		return len;

	/* Compute the right value to modify in register */
	value = (value - 3500) / 20;

	summit_config_charge_voltage(di, CONFIG_NO_CHANGE, value);

	return len;
}

static ssize_t summit_fast_current_show(struct device *dev, struct device_attribute *attr,char *buf)
{
    int config=0;
    struct summit_smb347_info *di = i2c_get_clientdata(to_i2c_client(dev));
    config=i2c_smbus_read_byte_data(di->client,SUMMIT_SMB347_CHARGE_CURRENT);
    config=(config >> 5)& 7;
    return sprintf(buf, "fast_current=%d mA\n", fast_current[config]);
}

static ssize_t summit_fast_current_store(struct device *dev, struct device_attribute *attr,const char *buf,size_t len)
{
    struct summit_smb347_info *di = i2c_get_clientdata(to_i2c_client(dev));
    char *buffer;
    int value=0;
    value = (int)simple_strtoul(buf, NULL, 10);
    if(value>=0 && value <=7){
    	summit_config_charge_current(di,value<<5,PCC_150mA,TC_250mA);
    }
    return len;
}

static ssize_t summit_charge_current_show(struct device *dev,
		struct device_attribute *attr, char *buf, size_t len)
{
	int val = 0;
	struct summit_smb347_info *di = i2c_get_clientdata(to_i2c_client(dev));
	const int fast_charge_current_list[] = { 700, 900, 1200, 1500, 1800, 2000, 2200, 2500 };
	const int precharge_current_list[] = { 100, 150, 200, 250 };

	val = i2c_smbus_read_byte_data(di->client, SUMMIT_SMB347_STATUS_REG_B);

	if (val < 0)
		return sprintf(buf, "-1\n");

	if ((val & (1 << 5))) {
		/* Fast charge current */
		return sprintf(buf, "%d\n",
				fast_charge_current_list[val & 0x7]);
	} else {
		/* Pre-charge current */
		return sprintf(buf, "%d\n",
				precharge_current_list[(val >> 3) & 0x3]);
	}
}

static ssize_t summit_charge_current_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
	int i = 0;
	struct summit_smb347_info *di = i2c_get_clientdata(to_i2c_client(dev));
	int value = simple_strtoul(buf, NULL, 10);
	const int fast_charge_current_list[] = { 700, 900, 1200, 1500, 1800, 2000, 2200, 2500 };
	const int precharge_current_list[] = { 100, 150, 200, 250 };

	/* Don't perform anything for anything below DVT */
	if (di->mbid < 4)
		return len;

	if (i < 0 || i > 2500) {
		dev_err(dev, "Invalid charge current setting\n");
		return len;
	}

	/* Locate the first smaller current in fast charge current first */
	for (i = ARRAY_SIZE(fast_charge_current_list) - 1; i >= 0; i--)
		if (fast_charge_current_list[i] <= value)
			/* Found, stop */
			break;

	if (i >= 0) {
		/* Disable force precharge, set fast charge current */
		dev_info(dev, "Found fast charge current: %d\n", i);
		summit_force_precharge(di, 0);
		summit_config_charge_current(di, i << 5,
			CONFIG_NO_CHANGE, CONFIG_NO_CHANGE);
		return len;
	}

	/* Locate the first smaller current in pre-charge current */
	for (i = ARRAY_SIZE(precharge_current_list) - 1; i >= 0; i--)
		if (precharge_current_list[i] <= value)
			break;

	if (i >= 0) {
		/* Force pre-charge, set pre-charge current */
		dev_info(dev, "Found pre-charge current: %d\n", i);
		summit_force_precharge(di, 1);
		summit_config_charge_current(di, CONFIG_NO_CHANGE,
			i << 3, CONFIG_NO_CHANGE);
		return len;
	}

	/* Something wrong here */
	dev_warn(dev, "Unable to find a charge current setting for %d mA\n",
		value);
	return len;
}
static ssize_t summit_bad_battery_show(struct device *dev,
		struct device_attribute *attr, char *buf, size_t len)
{
	int val = 0;
	struct summit_smb347_info *di = i2c_get_clientdata(to_i2c_client(dev));
	return sprintf(buf, "%d\n",di->bad_battery);
}
/*Android code can read the battery ID and write ‘1’ to bad_battery if the battery is invalid*/
static ssize_t summit_bad_battery_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
	int i = 0;
	struct summit_smb347_info *di = i2c_get_clientdata(to_i2c_client(dev));
	int value = simple_strtoul(buf, NULL, 10);
	if(value ==1){
		di->bad_battery=1;
	}else  if(value==0){
		di->bad_battery=0;
	}
	/* Don't perform anything for anything below PVT */
	if (di->mbid < 5)
		return len;
	else{/*If userspace writes ‘1’ to bad_battery, and unit is PVT and beyond, disable charging.*/
		if(di->bad_battery==1)
			summit_charge_enable(di,0);
	}
	return len;
}
static DEVICE_ATTR(version,S_IRUGO, summit_version_show, NULL);
static DEVICE_ATTR(ondemand,S_IRUGO|S_IWUSR, summit_ondemand_show, summit_ondemand_store);
static DEVICE_ATTR(apsd_setting, S_IRUGO | S_IWUSR, summit_apsd_setting_show, summit_apsd_setting_store);
static DEVICE_ATTR(apsd_status, S_IRUGO , summit_apsd_status_show, NULL);
static DEVICE_ATTR(aicl_setting, S_IRUGO | S_IWUSR, summit_aicl_setting_show, summit_aicl_setting_store);
static DEVICE_ATTR(aicl_status, S_IRUGO , summit_aicl_status_show, NULL);
static DEVICE_ATTR(mode_command, S_IRUGO | S_IWUSR, summit_mode_show, summit_mode_store);
static DEVICE_ATTR(mode_status, S_IRUGO, summit_mode_status_show, NULL);
static DEVICE_ATTR(charge_command, S_IRUGO | S_IWUSR, summit_charge_show, summit_charge_store);
static DEVICE_ATTR(charge_status, S_IRUGO , summit_charge_status_show, NULL);
static DEVICE_ATTR(discharge, S_IRUGO | S_IWUSR , summit_discharge_show, summit_discharge_store);
static DEVICE_ATTR(status, S_IRUGO , summit_status_show, NULL);
static DEVICE_ATTR(present, S_IRUGO, summit_present_show, NULL);
static DEVICE_ATTR(fake_disconnect, S_IRUGO, NULL, summit_fake_disconnect_store);
static DEVICE_ATTR(protect_enable, S_IRUGO, summit_protect_enable_show, summit_protect_enable_store);
static DEVICE_ATTR(protect_disable, S_IRUGO, summit_protect_enable_show, summit_protect_disable_store);
static DEVICE_ATTR(float_voltage, S_IRUGO | S_IWUSR,
	summit_float_voltage_show, summit_float_voltage_store);
static DEVICE_ATTR(fast_current, S_IRUGO, summit_fast_current_show, summit_fast_current_store);
static DEVICE_ATTR(charge_current, S_IRUGO | S_IWUSR,
	summit_charge_current_show, summit_charge_current_store);
static DEVICE_ATTR(bad_battery, S_IRUGO | S_IWUSR,
	summit_bad_battery_show, summit_bad_battery_store);
static struct attribute *summit_attrs[] = {
    &dev_attr_version.attr,
    &dev_attr_charge_command.attr,
    &dev_attr_charge_status.attr,
    &dev_attr_mode_command.attr,
    &dev_attr_mode_status.attr,
    &dev_attr_apsd_setting.attr,
    &dev_attr_apsd_status.attr,
    &dev_attr_aicl_setting.attr,
    &dev_attr_aicl_status.attr,
    &dev_attr_discharge.attr,
    &dev_attr_ondemand.attr,
    &dev_attr_present.attr,
    &dev_attr_status.attr,
    &dev_attr_fake_disconnect.attr,
    &dev_attr_protect_enable.attr,
    &dev_attr_protect_disable.attr,
    &dev_attr_float_voltage.attr,
    &dev_attr_fast_current.attr,
	&dev_attr_charge_current.attr,
    &dev_attr_bad_battery.attr,
    NULL
};

static struct attribute_group summit_attr_grp = {
    .attrs = summit_attrs,
};

//====================================================================================================
int __init create_summit_sysfs(struct summit_smb347_info *di)
{
    int error;
    /* Create a sysfs node */
    error = sysfs_create_group(&di->dev->kobj, &summit_attr_grp);
    if (error) {
    	dev_dbg(di->dev,"could not create sysfs_create_group\n");
    	return -1;
    }

    dev_dbg(di->dev,KERN_DEBUG"%s:init done\n",__func__);
    return 0;
}

int remove_summit_sysfs(struct summit_smb347_info *di)
{
    /* Remove sysfs node */
    sysfs_remove_group(&di->dev->kobj, &summit_attr_grp);
    return 0;
}
