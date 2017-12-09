/**************************************************
this is the ZWO filter wheel EFW SDK
any question feel free contact us:yang.zhou@zwoptical.com

here is the suggested procedure.

--> EFWGetNum
--> EFWGetID for each camera

--> EFWGetProperty
--> EFWOpen
--> EFWGetPosition
--> EFWSetPosition
	...
--> EFWClose

***************************************************/
#ifndef EFW_FILTER_H
#define EFW_FILTER_H


#ifdef _WINDOWS
#define EFW_API __declspec(dllexport)
#else
#define EFW_API 
#endif

#define EFW_ID_MAX 128

typedef struct _EFW_INFO
{
	int ID;
	char Name[64];
	int slotNum;
} EFW_INFO;


typedef enum _EFW_ERROR_CODE{
	EFW_SUCCESS = 0,
	EFW_ERROR_INVALID_INDEX,
	EFW_ERROR_INVALID_ID,
	EFW_ERROR_INVALID_VALUE,
	EFW_ERROR_REMOVED, //failed to find the filter wheel, maybe the filter wheel has been removed
	EFW_ERROR_MOVING,//filter wheel is moving
	EFW_ERROR_ERROR_STATE,//filter wheel is in error state
	EFW_ERROR_GENERAL_ERROR,//other error
	EFW_ERROR_NOT_SUPPORTED,
	EFW_ERROR_CLOSED,
	EFW_ERROR_END = -1
}EFW_ERROR_CODE;

#ifdef __cplusplus
extern "C" {
#endif
/***************************************************************************
Descriptions:
this should be the first API to be called
get number of connected EFW filter wheel, call this API to refresh device list if EFW is connected
or disconnected

Return: number of connected EFW filter wheel. 1 means 1 filter wheel is connected.
***************************************************************************/
EFW_API int EFWGetNum();

/***************************************************************************
Descriptions:
get the product ID of each wheel, at first set pPIDs as 0 and get length and then malloc a buffer to load the PIDs

Paras:
int* pPIDs: pointer to array of PIDs

Return: length of the array.
***************************************************************************/
EFW_API int EFWGetProductIDs(int* pPIDs);

/***************************************************************************
Descriptions:
get ID of filter wheel

Paras:
int index: the index of filter wheel, from 0 to N - 1, N is returned by GetNum()

int* ID: pointer to ID. the ID is a unique integer, between 0 to EFW_ID_MAX - 1, after opened,
all the operation is base on this ID, the ID will not change.


Return: 
EFW_ERROR_INVALID_INDEX: index value is invalid
EFW_SUCCESS:  operation succeeds

***************************************************************************/
EFW_API EFW_ERROR_CODE EFWGetID(int index, int* ID);

/***************************************************************************
Descriptions:
open filter wheel

Paras:
int ID: the ID of filter wheel

Return: 
EFW_ERROR_INVALID_ID: invalid ID value
EFW_ERROR_GENERAL_ERROR: number of opened filter wheel reaches the maximum value.
EFW_ERROR_REMOVED: the filter wheel is removed.
EFW_SUCCESS: operation succeeds
***************************************************************************/
EFW_API	EFW_ERROR_CODE EFWOpen(int ID);

/***************************************************************************
Descriptions:
get property of filter wheel. SlotNum is 0 if not opened.

Paras:
int ID: the ID of filter wheel

EFW_INFO *pInfo:  pointer to structure containing the property of EFW

Return: 
EFW_ERROR_INVALID_ID: invalid ID value
EFW_ERROR_MOVING: slot number detection is in progress, generally this error will happen soon after filter wheel is connected.
EFW_SUCCESS: operation succeeds
EFW_ERROR_REMOVED: filter wheel is removed

***************************************************************************/
EFW_API	EFW_ERROR_CODE EFWGetProperty(int ID, EFW_INFO *pInfo); 

/***************************************************************************
Descriptions:
get position of slot

Paras:
int ID: the ID of filter wheel

int *pPosition:  pointer to slot position, this value is between 0 to M - 1, M is slot number
this value is -1 if filter wheel is moving

Return: 
EFW_ERROR_INVALID_ID: invalid ID value
EFW_ERROR_CLOSED: not opened
EFW_SUCCESS: operation succeeds
EFW_ERROR_ERROR_STATE: filter wheel is in error state
EFW_ERROR_REMOVED: filter wheel is removed

***************************************************************************/	
EFW_API	EFW_ERROR_CODE EFWGetPosition(int ID, int *pPosition);
	
/***************************************************************************
Descriptions:
set position of slot

Paras:
int ID: the ID of filter wheel

int Position:  slot position, this value is between 0 to M - 1, M is slot number

Return: 
EFW_ERROR_INVALID_ID: invalid ID value
EFW_ERROR_CLOSED: not opened
EFW_SUCCESS: operation succeeds
EFW_ERROR_INVALID_VALUE: Position value is invalid
EFW_ERROR_MOVING: filter wheel is moving, should wait until idle
EFW_ERROR_ERROR_STATE: filter wheel is in error state
EFW_ERROR_REMOVED: filter wheel is removed

***************************************************************************/
EFW_API	EFW_ERROR_CODE EFWSetPosition(int ID, int Position);

/***************************************************************************
Descriptions:
set unidirection of filter wheel

Paras:
int ID: the ID of filter wheel

bool bUnidirectional: if set as true, the filter wheel will rotate along one direction

Return: 
EFW_ERROR_INVALID_ID: invalid ID value
EFW_ERROR_CLOSED: not opened
EFW_SUCCESS: operation succeeds
***************************************************************************/
EFW_API	EFW_ERROR_CODE EFWSetDirection(int ID, bool bUnidirectional);

/***************************************************************************
Descriptions:
get unidirection of filter wheel

Paras:
int ID: the ID of filter wheel

bool *bUnidirectional: pointer to unidirection value .

Return: 
EFW_ERROR_INVALID_ID: invalid ID value
EFW_ERROR_CLOSED: not opened
EFW_SUCCESS: operation succeeds
***************************************************************************/
EFW_API	EFW_ERROR_CODE EFWGetDirection(int ID, bool *bUnidirectional);

/***************************************************************************
Descriptions:
calibrate filter wheel

Paras:
int ID: the ID of filter wheel

Return: 
EFW_ERROR_INVALID_ID: invalid ID value
EFW_ERROR_CLOSED: not opened
EFW_SUCCESS: operation succeeds
EFW_ERROR_MOVING: filter wheel is moving, should wait until idle
EFW_ERROR_ERROR_STATE: filter wheel is in error state
EFW_ERROR_REMOVED: filter wheel is removed
***************************************************************************/
EFW_API	EFW_ERROR_CODE EFWCalibrate(int ID);

/***************************************************************************
Descriptions:
close filter wheel

Paras:
int ID: the ID of filter wheel

Return: 
EFW_ERROR_INVALID_ID: invalid ID value
EFW_SUCCESS: operation succeeds
***************************************************************************/
EFW_API	EFW_ERROR_CODE EFWClose(int ID);


#ifdef __cplusplus
}
#endif

#endif
