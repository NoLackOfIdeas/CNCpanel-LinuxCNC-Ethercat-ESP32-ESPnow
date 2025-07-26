#ifndef CUSTOM_PDO_NAME_H
#define CUSTOM_PDO_NAME_H

//-------------------------------------------------------------------//
//                                                                   //
//     This file has been created by the Easy Configurator tool      //
//                                                                   //
//     Easy Configurator project SM_Test_myCustomSlave.prj
//                                                                   //
//-------------------------------------------------------------------//

#define CUST_BYTE_NUM_OUT 14
#define CUST_BYTE_NUM_IN 9
#define TOT_BYTE_NUM_ROUND_OUT 16
#define TOT_BYTE_NUM_ROUND_IN 12

typedef union //---- output buffer ----
{
	uint8_t Byte[TOT_BYTE_NUM_ROUND_OUT];
	struct
	{
		uint64_t myOutputVariable4;
		uint32_t myOutputVariable2;
		uint8_t myOutputVariable2;
		uint8_t myOutputVariable1;
	} Cust;
} PROCBUFFER_OUT;

typedef union //---- input buffer ----
{
	uint8_t Byte[TOT_BYTE_NUM_ROUND_IN];
	struct
	{
		double myInputVariable2;
		uint8_t myInputVariable1;
	} Cust;
} PROCBUFFER_IN;

#endif