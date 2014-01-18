#include "DiskFilter.h"

IO_COMPLETION_ROUTINE __CompletionRoutine;
static NTSTATUS
__CompletionRoutine(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context)
{
	if (Irp->PendingReturned == TRUE)
	{
		KeSetEvent ((PKEVENT) Context, IO_NO_INCREMENT, FALSE);
	}
	return STATUS_MORE_PROCESSING_REQUIRED;
}

static NTSTATUS
ForwardIrpSynchronously(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	KEVENT		event;
	NTSTATUS	status;

	IoCopyCurrentIrpStackLocationToNext(Irp);
	KeInitializeEvent(&event, NotificationEvent, FALSE);
	IoSetCompletionRoutine (Irp,
							__CompletionRoutine,
							&event,
							TRUE, TRUE, TRUE);
	status = IoCallDriver(DeviceObject, Irp);

	if (status == STATUS_PENDING)
	{
		KeWaitForSingleObject (&event,
								Executive,// WaitReason
								KernelMode,// must be Kernelmode to prevent the stack getting paged out
								FALSE,
								NULL// indefinite wait
								);
		status = Irp->IoStatus.Status;
	}
	return status;
}

VOID DF_ReadWriteThread(PVOID Context)
{
	PIRP					Irp;
	PLIST_ENTRY				ReqEntry;
	LONGLONG				Offset;
	ULONG					Length;
	PUCHAR					SysBuf;
	PIO_STACK_LOCATION		IrpSp;
	PDF_DEVICE_EXTENSION	DevExt;
	DevExt = (PDF_DEVICE_EXTENSION)Context;

	// set thread priority.
	KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY);
	KdPrint(("%u-%u: Read Write Thread Start...\n", DevExt->DiskNumber, DevExt->PartitionNumber));
	for (;;)
	{
		KeWaitForSingleObject(&DevExt->RwThreadEvent,
			Executive, KernelMode, FALSE, NULL);
		if (DevExt->bTerminalThread)
		{
			PsTerminateSystemThread(STATUS_SUCCESS);
			KdPrint(("Read Write Thread Exit...\n"));
			return;
		}

		while (NULL != (ReqEntry = ExInterlockedRemoveHeadList(
						&DevExt->RwList, &DevExt->RwSpinLock)))
		{
			Irp = CONTAINING_RECORD(ReqEntry, IRP, Tail.Overlay.ListEntry);
			IrpSp = IoGetCurrentIrpStackLocation(Irp);
			// Get system buffer address
			if (Irp->MdlAddress != NULL)
				SysBuf = (PUCHAR)MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
			else
				SysBuf = (PUCHAR)Irp->UserBuffer;
			if (SysBuf == NULL)
				SysBuf = (PUCHAR)Irp->AssociatedIrp.SystemBuffer;

			// Get offset and length
			if (IRP_MJ_READ == IrpSp->MajorFunction)
			{
				Offset = IrpSp->Parameters.Read.ByteOffset.QuadPart;
				Length = IrpSp->Parameters.Read.Length;
			}
			else if (IRP_MJ_WRITE == IrpSp->MajorFunction)
			{
				Offset = IrpSp->Parameters.Write.ByteOffset.QuadPart;
				Length = IrpSp->Parameters.Write.Length;
			}
			else
			{
				Offset = 0;
				Length = 0;
			}

			if (!SysBuf || !Length) // Ignore this IRP.
			{
				IoSkipCurrentIrpStackLocation(Irp);
				IoCallDriver(DevExt->LowerDeviceObject, Irp);
				continue;
			}

			if (IrpSp->MajorFunction == IRP_MJ_READ)
			{
				DevExt->ReadCount++;
				DBG_PRINT(DBG_TRACE_RW, ("%u-%u: R off(%I64d) len(%x)\n", DevExt->DiskNumber, DevExt->PartitionNumber, Offset, Length));
			}
			else
			{
				DevExt->WriteCount++;
				DBG_PRINT(DBG_TRACE_RW, ("%u-%u: W off(%I64d) len(%x)\n", DevExt->DiskNumber, DevExt->PartitionNumber, Offset, Length));
			}

			// Read Request
			if (IrpSp->MajorFunction == IRP_MJ_READ)
			{
				// Cache Full Hitted
				if (QueryAndCopyFromCachePool(
						&DevExt->CachePool,
						SysBuf,
						Offset,
						Length) == TRUE)
				{
					KdPrint(("^^^^cache hit^^^^\n"));
					DevExt->CacheHit++;
					Irp->IoStatus.Status = STATUS_SUCCESS;
					Irp->IoStatus.Information = Length;
					IoCompleteRequest(Irp, IO_DISK_INCREMENT);
					continue;
				}
				else
				{
					Irp->IoStatus.Information = 0;
					Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
					ForwardIrpSynchronously(DevExt->LowerDeviceObject, Irp);
					if (NT_SUCCESS(Irp->IoStatus.Status))
					{
						UpdataCachePool(&DevExt->CachePool,
										SysBuf,
										Offset,
										Length,
										_READ_);
						IoCompleteRequest(Irp, IO_DISK_INCREMENT);
						continue;
					}
					else
					{
						Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
						Irp->IoStatus.Information = 0;
						IoCompleteRequest(Irp, IO_DISK_INCREMENT);
						continue;
					}
				}
			}
			// Write Request
			else
			{
				Irp->IoStatus.Information = 0;
				Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
				ForwardIrpSynchronously(DevExt->LowerDeviceObject, Irp);
				if (NT_SUCCESS(Irp->IoStatus.Status))
				{
					UpdataCachePool(&DevExt->CachePool,
									SysBuf,
									Offset,
									Length,
									_WRITE_);
					IoCompleteRequest(Irp, IO_DISK_INCREMENT);
					continue;
				}
				else
				{
					Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
					Irp->IoStatus.Information = 0;
					IoCompleteRequest(Irp, IO_DISK_INCREMENT);
					continue;
				}
			}
		} // while list not empty
	} // forever loop
}