
//**************************************************************************************
//	����:	23:2:2004   
//	����:	tiamo	
//	����:   exbus
//**************************************************************************************

#include "stdafx.h"
#include "..\public.h"

#pragma alloc_text(INIT,DriverEntry)
#pragma alloc_text(PAGE,DriverUnload)
#pragma alloc_text(PAGE,DispatchCreateClose)
#pragma alloc_text(PAGE,AddDevice)

// driver entry
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriver,PUNICODE_STRING pRegPath)
{
	pDriver->DriverUnload = DriverUnload;
	pDriver->DriverExtension->AddDevice = AddDevice;
	pDriver->MajorFunction[IRP_MJ_CREATE] = DispatchCreateClose;
	pDriver->MajorFunction[IRP_MJ_CLOSE] = DispatchCreateClose;
	pDriver->MajorFunction[IRP_MJ_PNP] = DispatchPnP;
	pDriver->MajorFunction[IRP_MJ_POWER] = DispatchPower;
	pDriver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchIoControl;
	devDebugPrint(DRIVER_NAME"*******DriverEntry called\n");
	return STATUS_SUCCESS;
}

// driver unload
void DriverUnload(PDRIVER_OBJECT pDriver)
{
	devDebugPrint(DRIVER_NAME"*******DriverUnload called\n");
	return;
}

// create close for device control
NTSTATUS DispatchCreateClose(PDEVICE_OBJECT pDevice,PIRP pIrp)
{
	NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;

	PCommonExt pCommonExt = static_cast<PCommonExt>(pDevice->DeviceExtension);
	if(pCommonExt->m_bFdo)
	{
		PIO_STACK_LOCATION pIoStack = IoGetCurrentIrpStackLocation(pIrp);
		switch(pIoStack->MajorFunction)
		{
		case IRP_MJ_CREATE:
			devDebugPrint(DRIVER_NAME"*******IRP_MJ_CREATE for bus fdo device \n");
			break;
		case IRP_MJ_CLOSE:
			devDebugPrint(DRIVER_NAME"*******IRP_MJ_CLOSE for bus fdo device \n");
			break;
		}
		status = STATUS_SUCCESS;
	}

	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = status;
	IoCompleteRequest(pIrp,IO_NO_INCREMENT);

	return status;
}

// device io control for bus fdo
NTSTATUS DispatchIoControl(PDEVICE_OBJECT pDevice,PIRP pIrp)
{
	NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;

	PFdoExt pFdoExt = static_cast<PFdoExt>(pDevice->DeviceExtension);

	if(pFdoExt->m_bFdo)
	{
		BOOLEAN bCallNext = TRUE;
		IncIoCount(pFdoExt);
		
		PIO_STACK_LOCATION pIoStack = IoGetCurrentIrpStackLocation(pIrp);
		switch(pIoStack->Parameters.DeviceIoControl.IoControlCode)
		{
			// add a scsi device,input is a miniport config struct
		case IOCTL_TIAMO_BUS_PLUGIN:
			status = STATUS_INVALID_PARAMETER;

			bCallNext = FALSE;

			if(pIoStack->Parameters.DeviceIoControl.InputBufferLength == sizeof(MiniportConfig))
			{
				PMiniportConfig pMiniportConfig = static_cast<PMiniportConfig>(pIrp->AssociatedIrp.SystemBuffer);

				if(pMiniportConfig && pMiniportConfig->m_ulDevices > 0 && pMiniportConfig->m_ulDevices < 5)
				{
					ExAcquireFastMutex(&pFdoExt->m_mutexEnumPdo);

					// create the only one pdo
					if(!pFdoExt->m_pEnumPdo)
					{
						PDEVICE_OBJECT pdo;
						status = IoCreateDevice(pDevice->DriverObject,sizeof(PdoExt),NULL,FILE_DEVICE_BUS_EXTENDER,
												FILE_AUTOGENERATED_DEVICE_NAME | FILE_DEVICE_SECURE_OPEN,FALSE,&pdo);

						if(NT_SUCCESS(status))
						{
							// set the pdo ext
							PPdoExt pPdoExt = static_cast<PPdoExt>(pdo->DeviceExtension);

							RtlZeroMemory(pPdoExt,sizeof(PdoExt));

							pPdoExt->m_bFdo = FALSE;
							// save the fdo here
							pPdoExt->m_pParentFdo = pDevice;
							pPdoExt->m_ulCurrentPnpState = pPdoExt->m_ulPrevPnpState = -1;

							pPdoExt->m_sysPowerState = PowerSystemWorking;
							pPdoExt->m_devPowerState = PowerDeviceD3;
							pPdoExt->m_bPresent = TRUE;

							//pdo->Flags |= DO_POWER_PAGABLE;
							
							pFdoExt->m_pEnumPdo = pdo;

							RtlMoveMemory(pPdoExt->m_szImageFileName,pMiniportConfig->m_szImageFileName,1024 * sizeof(WCHAR));

							pPdoExt->m_ulDevices = pMiniportConfig->m_ulDevices;

							pdo->Flags &= ~DO_DEVICE_INITIALIZING;

							devDebugPrint(DRIVER_NAME"*******IOCTL_TIAMO_BUS_PLUGIN detected new device and created a pdo.\n");
						}
					}
					else
					{
						status = STATUS_DEVICE_ALREADY_ATTACHED;
					}

					ExReleaseFastMutex(&pFdoExt->m_mutexEnumPdo);

					if(NT_SUCCESS(status))
						IoInvalidateDeviceRelations(pFdoExt->m_pPhysicalDevice,BusRelations);
				}
			}
			break;

			// miniport get config
		case IOCTL_TIAMO_BUS_MINIPORT_GET_CONFIG:
			status = STATUS_INVALID_PARAMETER;

			bCallNext = FALSE;

			if(pIoStack->Parameters.DeviceIoControl.OutputBufferLength == sizeof(MiniportConfig))
			{
				ExAcquireFastMutex(&pFdoExt->m_mutexEnumPdo);
				if(pFdoExt->m_pEnumPdo)
				{
					PPdoExt pPdoExt = static_cast<PPdoExt>(pFdoExt->m_pEnumPdo->DeviceExtension);
					if(pPdoExt)
					{
						RtlMoveMemory(pIrp->AssociatedIrp.SystemBuffer,&pPdoExt->m_ulDevices,sizeof(MiniportConfig));
						pIrp->IoStatus.Information = sizeof(MiniportConfig);
						status = STATUS_SUCCESS;
						devDebugPrint(DRIVER_NAME"*******IOCTL_TIAMO_BUS_MINIPORT_GET_CONFIG upper miniport fdo get it's pdo configuration\n");
					}
				}
				else
				{
					status = STATUS_NO_SUCH_DEVICE;
				}

				ExReleaseFastMutex(&pFdoExt->m_mutexEnumPdo);
			}
			
			break;
		}

		DecIoCount(pFdoExt);

		if(bCallNext)
			return IoCallDriver(pFdoExt->m_pLowerDevice,pIrp);
	}
		
	pIrp->IoStatus.Status = status;
	IoCompleteRequest(pIrp,IO_NO_INCREMENT);
	
	return status;
}

// add device
NTSTATUS AddDevice(PDRIVER_OBJECT pDriver,PDEVICE_OBJECT pPhysicalDeviceObject)
{
	NTSTATUS status;
	PDEVICE_OBJECT pDevice = NULL;
	PFdoExt pFdoExt = NULL;
	__try
	{
		UNICODE_STRING busFdoName;
		RtlInitUnicodeString(&busFdoName,BUS_FDO_NAME);
		status = IoCreateDevice(pDriver,sizeof(FdoExt),&busFdoName,FILE_DEVICE_BUS_EXTENDER,FILE_DEVICE_SECURE_OPEN,
								FALSE,&pDevice);

		if(!NT_SUCCESS(status))
			ExRaiseStatus(status);

		pFdoExt = static_cast<PFdoExt>(pDevice->DeviceExtension);
		RtlZeroMemory(pFdoExt,sizeof(FdoExt));

		status = IoRegisterDeviceInterface(pPhysicalDeviceObject,&GUID_TIAMO_BUS,NULL,&pFdoExt->m_symbolicName);
		if(!NT_SUCCESS(status))
			ExRaiseStatus(status);

		KeInitializeEvent(&pFdoExt->m_evRemove,SynchronizationEvent,FALSE);

		KeInitializeEvent(&pFdoExt->m_evStop,SynchronizationEvent,TRUE);

		ExInitializeFastMutex (&pFdoExt->m_mutexEnumPdo);

		pFdoExt->m_sysPowerState = PowerSystemWorking;
		pFdoExt->m_devPowerState = PowerDeviceD3;

		pFdoExt->m_bFdo = TRUE;
		pFdoExt->m_pPhysicalDevice = pPhysicalDeviceObject;

		pFdoExt->m_pLowerDevice = IoAttachDeviceToDeviceStack(pDevice,pPhysicalDeviceObject);

		pFdoExt->m_ulCurrentPnpState = -1;
		pFdoExt->m_ulPrevPnpState = -1;

		pFdoExt->m_ulOutstandingIO = 1;

		//pDevice->Flags |= DO_POWER_PAGABLE;

		POWER_STATE state;
		state.DeviceState = PowerDeviceD0;
		PoSetPowerState(pDevice,DevicePowerState,state);

		pDevice->Flags &= ~DO_DEVICE_INITIALIZING;

		devDebugPrint(DRIVER_NAME"*******AddDevice called,fdo createed.\n");
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		if(pFdoExt && pFdoExt->m_pLowerDevice)
		{
			IoDetachDevice(pFdoExt->m_pLowerDevice);
			RtlFreeUnicodeString(&pFdoExt->m_symbolicName);
		}

		if(pDevice)
		{
			IoDeleteDevice(pDevice);
		}
	}

	return status;
}

// here outstandingio indicates that how many irps are outstanding.1 is the base
//
// 2 means only one irp is outstanding,when you want to stop the device,you process the xx_STOP IRP
// then if outstandingio is 2 means that,the irp that you are currently processing is the only one,
// so you can safely stop it(first Dec by 1,then the stopEvent will become signaled,wait for that object,you can get it,
// then do not forget inc it,because of the fact that when the dispatch function return ,it will dec it again)
//
// 1 means no irp is outstanding,so you can safely remove it(first dec it by 2,then the value become 0,so the removeEvent
// become signaled,wait for that object,you can get it,but now no need to inc it,because the device will disappear soon)
void IncIoCount(PFdoExt pExt)
{
	LONG ulRet;
	ulRet = InterlockedIncrement(reinterpret_cast<volatile LONG *>(&pExt->m_ulOutstandingIO));

	if(ulRet == 2) 
	{
		KeClearEvent(&pExt->m_evStop);
	}

	return;
}

void DecIoCount(PFdoExt pExt)   
{

	LONG ulRet = InterlockedDecrement(reinterpret_cast<volatile LONG *>(&pExt->m_ulOutstandingIO));

	if(ulRet == 1) 
	{
		KeSetEvent(&pExt->m_evStop,IO_NO_INCREMENT,FALSE);        
	}

	if(ulRet == 0) 
	{
		KeSetEvent(&pExt->m_evRemove,IO_NO_INCREMENT,FALSE);
	}

	return;
}