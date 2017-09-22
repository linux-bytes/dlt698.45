//
// Created by 周立海 on 2017/4/21.
//

#include <stdio.h>

#include "class23.h"
#include "dlt698.h"
#include "PublicFunction.h"

int class23_selector(int index, int attr_act, INT8U *data,
		Action_result *act_ret) {
	switch (attr_act) {
	case 1:
		class23_act1(index);
		break;
	case 3:
		class23_act3(index, data);
		break;
	}
	return 0;
}

int class23_act1(int index) {
	ProgramInfo *shareAddr = getShareAddr();
	asyslog(LOG_WARNING, "清除所有配置单元(%d)", index);
	memset(&shareAddr->class23[index], 0x00, sizeof(CLASS23));
	return 0;
}

int class23_act3(int index, INT8U* data) {
	AL_UNIT al_unit;
	if (data[0] != 0x02 || data[1] != 0x03 || data[2] != 0x55) {
		return 0;
	}

	int tsa_len = data[3];
	int data_index = 4;

	if (tsa_len > 17) {
		return 0;
	}
	al_unit.tsa.addr[0] = data[3];

	for (int i = 0; i < tsa_len; ++i) {
		al_unit.tsa.addr[1 + i] = data[data_index];
		data_index++;
	}

	if (data[data_index] != 0x16 || data[data_index + 2] != 0x16) {
		return 0;
	}

	al_unit.al_flag = data[data_index + 1];
	al_unit.cal_flag = data[data_index + 3];

	asyslog(LOG_WARNING, "添加一个配置单元(%d)", index);
	ProgramInfo *shareAddr = getShareAddr();
	for (int i = 0; i < MAX_AL_UNIT; i++) {
		if (shareAddr->class23[index].allist[i].tsa.addr[0] == 0x00) {
			memcpy(&shareAddr->class23[index].allist[i], &al_unit,
					sizeof(AL_UNIT));
			asyslog(LOG_WARNING, "添加一个配置单元，地址(%d)", i);
			break;
		}
	}

	return 0;
}

int class23_set(int index, OAD oad, INT8U *data, INT8U *DAR) {
	ProgramInfo *shareAddr = getShareAddr();
	asyslog(LOG_WARNING, "修改总加组属性(%d)", oad.attflg);

	switch (oad.attflg) {
	case 13:
		if (data[0] != 0x17) {
			return 0;
		}
		shareAddr->class23[index].aveCircle = data[1];
		break;
	case 14:
		if (data[0] != 0x04) {
			return 0;
		}
		shareAddr->class23[index].pConfig = data[1];
		break;
	case 15:
		if (data[0] != 0x04) {
			return 0;
		}
		shareAddr->class23[index].eConfig = data[1];
		break;
	}
	return 0;
}

int class23_get_2(OAD oad, INT8U index, INT8U *buf, int *len)
{
	ProgramInfo *shareAddr = getShareAddr();
	INT8U	unit = 1;	//总加组配置单元个数//TODO：总加组实际个数 取何值？？？
	INT8U	i=0;

	*len = 0;
	fprintf(stderr,"attrindex = %d \n",oad.attrindex);
	if(oad.attrindex == 0) {
		*len += create_array(&buf[*len],unit);
		for(i=0;i<unit;i++) {
			*len += create_struct(&buf[*len],3);
			*len += fill_TSA(&buf[*len],&shareAddr->class23[index].allist[i].tsa.addr[1],shareAddr->class23[index].allist[i].tsa.addr[0]);
			*len += fill_enum(&buf[*len],shareAddr->class23[index].allist[i].al_flag);
			*len += fill_enum(&buf[*len],shareAddr->class23[index].allist[i].cal_flag);
		}
	}
	fprintf(stderr," len = %d\n",*len);
	return 1;
}

int class23_get_long64(OAD oad, INT64U val, INT8U *buf, int *len)
{
	*len = 0;
	if(oad.attrindex == 0) {
		*len += fill_double_long64(&buf[*len], val);
	}
	return 1;
}

int class23_get_unsigned(OAD oad, INT8U val, INT8U *buf, int *len)
{
	*len = 0;
	if(oad.attrindex == 0) {
		*len += fill_unsigned(&buf[*len], val);
	}
	return 1;
}

int class23_get_bitstring(OAD oad, INT8U val, INT8U *buf, int *len)
{
	*len = 0;
	if(oad.attrindex == 0) {
		*len += fill_bit_string(&buf[*len],1,&val);
	}
	return 1;
}

int class23_get_7_8_9_10(OAD oad, INT64U energy_all,INT64U *energy,INT8U *buf, int *len){
	INT64U total_energy[MAXVAL_RATENUM + 1];
	INT8U	unit=0,i=0;
	///TODO: 总电量分相累加？？？还是内存energy_all
	*len = 0;
	total_energy[0] = 0;
	fprintf(stderr, "class23_get_7 %lld\n", total_energy[0]);

	for (i = 0; i < MAXVAL_RATENUM; i++){
		total_energy[0] += energy[i];
	}
	for (i = 0; i < MAXVAL_RATENUM; i++){
		total_energy[i+1] = energy[i];
	}
	fprintf(stderr, "class23_get_7 %lld\n", total_energy[0]);

	if (oad.attrindex == 0) {
		unit = MAXVAL_RATENUM + 1;	//总及n个费率
		*len = 0;
		*len += create_array(&buf[*len],unit);
		for(i=0;i<unit;i++) {
			*len += fill_double_long64(&buf[*len], total_energy[i]);
		}
	}else {
		unit = oad.attrindex - 1;
		unit = rangeJudge("电能量",unit,1,MAXVAL_RATENUM + 1);
		if(unit != -1) {
			*len = 0;
			*len += fill_double_long64(&buf[*len], total_energy[unit]);
		}
		return 1;
	}
	return 0;
}

int class23_get_17(OAD oad, INT8U index, INT8U *buf, int *len) {
	ProgramInfo *shareAddr = getShareAddr();

	*len = 0;
	*len += create_struct(&buf[*len],7);
	*len += fill_double_long64(&buf[*len],shareAddr->class23[index].alCtlState.v);
	*len += fill_integer(&buf[*len],shareAddr->class23[index].alCtlState.Downc);
	*len += fill_bit_string(&buf[*len],8,&shareAddr->class23[index].alCtlState.OutputState);
	*len += fill_bit_string(&buf[*len],8,&shareAddr->class23[index].alCtlState.MonthOutputState);
	*len += fill_bit_string(&buf[*len],8,&shareAddr->class23[index].alCtlState.BuyOutputState);
	*len += fill_bit_string(&buf[*len],8,&shareAddr->class23[index].alCtlState.PCAlarmState);
	*len += fill_bit_string(&buf[*len],8,&shareAddr->class23[index].alCtlState.ECAlarmState);
	return 1;
}

int class23_get(OAD oad, INT8U *sourcebuf, INT8U *buf, int *len) {
	asyslog(LOG_WARNING, "召唤总加组属性(%d)", oad.attflg);
	ProgramInfo *shareAddr = getShareAddr();
	int index = oad.OI - 0x2301;

	index = rangeJudge("总加组",index,0,7);
	if(index == -1) return 0;

	switch (oad.attflg) {
	case 2:
		return class23_get_2(oad, index, buf, len);
	case 3:
		return class23_get_long64(oad, shareAddr->class23[index].p, buf, len);
	case 4:
		return class23_get_long64(oad, shareAddr->class23[index].q, buf, len);
	case 5:
		return class23_get_long64(oad, shareAddr->class23[index].TaveP, buf, len);
	case 6:
		return class23_get_long64(oad, shareAddr->class23[index].TaveQ, buf, len);
	case 7:

		return class23_get_7_8_9_10(oad, shareAddr->class23[index].DayPALL, shareAddr->class23[index].DayP, buf, len);
	case 8:
		return class23_get_7_8_9_10(oad, shareAddr->class23[index].DayQALL, shareAddr->class23[index].DayQ, buf, len);
	case 9:
		return class23_get_7_8_9_10(oad, shareAddr->class23[index].MonthPALL, shareAddr->class23[index].MonthP, buf, len);
	case 10:
		return class23_get_7_8_9_10(oad, shareAddr->class23[index].MonthQALL, shareAddr->class23[index].MonthQ, buf, len);
	case 11:
		return class23_get_long64(oad, shareAddr->class23[index].remains, buf, len);
	case 12:
		return class23_get_long64(oad, shareAddr->class23[index].DownFreeze, buf, len);
	case 13:
		return class23_get_unsigned(oad, shareAddr->class23[index].aveCircle, buf, len);
	case 14:
		return class23_get_bitstring(oad, shareAddr->class23[index].pConfig, buf, len);
	case 15:
		return class23_get_bitstring(oad, shareAddr->class23[index].eConfig, buf, len);
	case 17:
		return class23_get_17(oad, index, buf, len);

	}
	return 1;
}

