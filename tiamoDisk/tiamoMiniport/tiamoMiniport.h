
//**************************************************************************************
//	����:	23:2:2004   
//	����:	tiamo	
//	����:	tiamoMiniport
//**************************************************************************************

#pragma once

extern "C"
{
	NTSTATUS DriverEntry(PDRIVER_OBJECT pDriver,PUNICODE_STRING pRegPath);
	void DriverUnload(PDRIVER_OBJECT pDriver);
}
