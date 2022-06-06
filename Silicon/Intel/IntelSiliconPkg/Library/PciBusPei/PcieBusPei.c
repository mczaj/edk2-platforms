/** @file
  Source code file for PciBusPei module

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>
#include <Uefi.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/PciHostBridgeResourceAllocation.h>
#include <Ppi/PciDevicePpi.h>
#include <Library/PciHostBridgeLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PciExpressHelpersLib.h>
#include <Library/PcieHelperLib.h>
#include <Library/DebugLib.h>
#include <Library/PciSegmentLib.h>
#include <IndustryStandard/Pci.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/IoLib.h>
#include <Library/DevicePathLib.h>
#include <Library/TimerLib.h>
#include <Library/BaseLib.h>
#include <Ppi/IoMmu.h>
#include <IndustryStandard/Acpi.h>

#include "PcieBusPei.h"

GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gEfiPciIoProtocolGuid = EFI_PCI_IO_PROTOCOL_GUID;

/**
  Enable a PCI driver to access PCI controller registers in the PCI memory or I/O space.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Width                 Signifies the width of the memory or I/O operations.
  @param  BarIndex              The BAR index of the standard PCI Configuration header to use as the
                                base address for the memory or I/O operation to perform.
  @param  Offset                The offset within the selected BAR to start the memory or I/O operation.
  @param  Count                 The number of memory or I/O operations to perform.
  @param  Buffer                For read operations, the destination buffer to store the results. For write
                                operations, the source buffer to write data from.

  @retval EFI_SUCCESS           The data was read from or written to the PCI controller.
  @retval EFI_UNSUPPORTED       BarIndex not valid for this PCI controller.
  @retval EFI_UNSUPPORTED       The address range specified by Offset, Width, and Count is not
                                valid for the PCI BAR specified by BarIndex.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.

**/
EFI_STATUS
PciBusMemRead (
  IN EFI_PCI_IO_PROTOCOL              *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH    Width,
  IN     UINT8                        BarIndex,
  IN     UINT64                       Offset,
  IN     UINTN                        Count,
  IN OUT VOID                         *Buffer
  )
{
  PCI_DEVICE_PRIVATE_DATA  *PrivateData;
  UINT8                    *Uint8Buffer;
  UINT16                   *Uint16Buffer;
  UINT32                   *Uint32Buffer;
  UINT32                   BarValue;
  UINTN                    Index;

  PrivateData = (PCI_DEVICE_PRIVATE_DATA*) This;

  BarValue = PciSegmentRead32 (PrivateData->PciCfgBase + (R_BASE_ADDRESS_OFFSET_0 + (0x4 * BarIndex)));
  BarValue &= 0xFFFFFFF0;

  switch (Width) {
    case EfiPciIoWidthUint8:
      Uint8Buffer = (UINT8*) Buffer;
      for (Index = 0; Index < Count; Index++) {
        Uint8Buffer[Index] = MmioRead8 (BarValue + (UINT32)Offset);
      }
      break;
    case EfiPciIoWidthUint16:
      Uint16Buffer = (UINT16*) Buffer;
      for (Index = 0; Index < Count; Index++) {
        Uint16Buffer[Index] = MmioRead16 (BarValue + (UINT32)Offset);
      }
      break;
    case EfiPciIoWidthUint32:
      Uint32Buffer = (UINT32*) Buffer;
      for (Index = 0; Index < Count; Index++) {
        Uint32Buffer[Index] = MmioRead32 (BarValue + (UINT32)Offset);
      }
      break;
  }
  return EFI_SUCCESS;
}

/**
  Enable a PCI driver to access PCI controller registers in the PCI memory or I/O space.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Width                 Signifies the width of the memory or I/O operations.
  @param  BarIndex              The BAR index of the standard PCI Configuration header to use as the
                                base address for the memory or I/O operation to perform.
  @param  Offset                The offset within the selected BAR to start the memory or I/O operation.
  @param  Count                 The number of memory or I/O operations to perform.
  @param  Buffer                For read operations, the destination buffer to store the results. For write
                                operations, the source buffer to write data from.

  @retval EFI_SUCCESS           The data was read from or written to the PCI controller.
  @retval EFI_UNSUPPORTED       BarIndex not valid for this PCI controller.
  @retval EFI_UNSUPPORTED       The address range specified by Offset, Width, and Count is not
                                valid for the PCI BAR specified by BarIndex.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.

**/
EFI_STATUS
PciBusMemWrite (
  IN EFI_PCI_IO_PROTOCOL              *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH    Width,
  IN     UINT8                        BarIndex,
  IN     UINT64                       Offset,
  IN     UINTN                        Count,
  IN OUT VOID                         *Buffer
  )
{
  PCI_DEVICE_PRIVATE_DATA  *PrivateData;
  UINT8                    *Uint8Buffer;
  UINT16                   *Uint16Buffer;
  UINT32                   *Uint32Buffer;
  UINT32                   BarValue;
  UINTN                    Index;

  PrivateData = (PCI_DEVICE_PRIVATE_DATA*) This;

  BarValue = PciSegmentRead32 (PrivateData->PciCfgBase + (R_BASE_ADDRESS_OFFSET_0 + (0x4 * BarIndex)));
  BarValue &= 0xFFFFFFF0;

  switch (Width) {
    case EfiPciIoWidthUint8:
      Uint8Buffer = (UINT8*) Buffer;
      for (Index = 0; Index < Count; Index++) {
        MmioWrite8 (BarValue + (UINT32)Offset, Uint8Buffer[Index]);
      }
      break;
    case EfiPciIoWidthUint16:
      Uint16Buffer = (UINT16*) Buffer;
      for (Index = 0; Index < Count; Index++) {
        MmioWrite16 (BarValue + (UINT32)Offset, Uint16Buffer[Index]);
      }
      break;
    case EfiPciIoWidthUint32:
      Uint32Buffer = (UINT32*) Buffer;
      for (Index = 0; Index < Count; Index++) {
        MmioWrite32 (BarValue + (UINT32)Offset, Uint32Buffer[Index]);
      }
      break;
  }
  return EFI_SUCCESS;
}

/**
  Enable a PCI driver to access PCI controller registers in the PCI memory or I/O space.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Width                 Signifies the width of the memory or I/O operations.
  @param  BarIndex              The BAR index of the standard PCI Configuration header to use as the
                                base address for the memory or I/O operation to perform.
  @param  Offset                The offset within the selected BAR to start the memory or I/O operation.
  @param  Count                 The number of memory or I/O operations to perform.
  @param  Buffer                For read operations, the destination buffer to store the results. For write
                                operations, the source buffer to write data from.

  @retval EFI_SUCCESS           The data was read from or written to the PCI controller.
  @retval EFI_UNSUPPORTED       BarIndex not valid for this PCI controller.
  @retval EFI_UNSUPPORTED       The address range specified by Offset, Width, and Count is not
                                valid for the PCI BAR specified by BarIndex.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.

**/
EFI_STATUS
PciBusIoRead (
  IN EFI_PCI_IO_PROTOCOL              *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH    Width,
  IN     UINT8                        BarIndex,
  IN     UINT64                       Offset,
  IN     UINTN                        Count,
  IN OUT VOID                         *Buffer
  )
{
  PCI_DEVICE_PRIVATE_DATA  *PrivateData;
  UINT8                    *Uint8Buffer;
  UINT16                   *Uint16Buffer;
  UINT32                   *Uint32Buffer;
  UINT32                   BarValue;
  UINTN                    Index;

  PrivateData = (PCI_DEVICE_PRIVATE_DATA*) This;

  BarValue = PciSegmentRead32 (PrivateData->PciCfgBase + (R_BASE_ADDRESS_OFFSET_0 + (0x4 * BarIndex)));
  BarValue &= 0xFFFFFFFC;

  switch (Width) {
    case EfiPciIoWidthUint8:
      Uint8Buffer = (UINT8*) Buffer;
      for (Index = 0; Index < Count; Index++) {
        Uint8Buffer[Index] = IoRead8 (BarValue + (UINT32)Offset);
      }
      break;
    case EfiPciIoWidthUint16:
      Uint16Buffer = (UINT16*) Buffer;
      for (Index = 0; Index < Count; Index++) {
        Uint16Buffer[Index] = IoRead16 (BarValue + (UINT32)Offset);
      }
      break;
    case EfiPciIoWidthUint32:
      Uint32Buffer = (UINT32*) Buffer;
      for (Index = 0; Index < Count; Index++) {
        Uint32Buffer[Index] = IoRead32 (BarValue + (UINT32)Offset);
      }
      break;
  }
  return EFI_SUCCESS;
}

/**
  Enable a PCI driver to access PCI controller registers in the PCI memory or I/O space.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Width                 Signifies the width of the memory or I/O operations.
  @param  BarIndex              The BAR index of the standard PCI Configuration header to use as the
                                base address for the memory or I/O operation to perform.
  @param  Offset                The offset within the selected BAR to start the memory or I/O operation.
  @param  Count                 The number of memory or I/O operations to perform.
  @param  Buffer                For read operations, the destination buffer to store the results. For write
                                operations, the source buffer to write data from.

  @retval EFI_SUCCESS           The data was read from or written to the PCI controller.
  @retval EFI_UNSUPPORTED       BarIndex not valid for this PCI controller.
  @retval EFI_UNSUPPORTED       The address range specified by Offset, Width, and Count is not
                                valid for the PCI BAR specified by BarIndex.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.

**/
EFI_STATUS
PciBusIoWrite (
  IN EFI_PCI_IO_PROTOCOL              *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH    Width,
  IN     UINT8                        BarIndex,
  IN     UINT64                       Offset,
  IN     UINTN                        Count,
  IN OUT VOID                         *Buffer
  )
{
  PCI_DEVICE_PRIVATE_DATA  *PrivateData;
  UINT8                    *Uint8Buffer;
  UINT16                   *Uint16Buffer;
  UINT32                   *Uint32Buffer;
  UINT32                   BarValue;
  UINTN                    Index;

  PrivateData = (PCI_DEVICE_PRIVATE_DATA*) This;

  BarValue = PciSegmentRead32 (PrivateData->PciCfgBase + (R_BASE_ADDRESS_OFFSET_0 + (0x4 * BarIndex)));
  BarValue &= 0xFFFFFFFC;

  switch (Width) {
    case EfiPciIoWidthUint8:
      Uint8Buffer = (UINT8*) Buffer;
      for (Index = 0; Index < Count; Index++) {
        IoWrite8 (BarValue + (UINT32)Offset, Uint8Buffer[Index]);
      }
      break;
    case EfiPciIoWidthUint16:
      Uint16Buffer = (UINT16*) Buffer;
      for (Index = 0; Index < Count; Index++) {
        IoWrite16 (BarValue + (UINT32)Offset, Uint16Buffer[Index]);
      }
      break;
    case EfiPciIoWidthUint32:
      Uint32Buffer = (UINT32*) Buffer;
      for (Index = 0; Index < Count; Index++) {
        IoWrite32 (BarValue + (UINT32)Offset, Uint32Buffer[Index]);
      }
      break;
  }
  return EFI_SUCCESS;
}

/**
  Reads from the memory space of a PCI controller. Returns either when the polling exit criteria is
  satisfied or after a defined duration.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Width                 Signifies the width of the memory or I/O operations.
  @param  BarIndex              The BAR index of the standard PCI Configuration header to use as the
                                base address for the memory operation to perform.
  @param  Offset                The offset within the selected BAR to start the memory operation.
  @param  Mask                  Mask used for the polling criteria.
  @param  Value                 The comparison value used for the polling exit criteria.
  @param  Delay                 The number of 100 ns units to poll.
  @param  Result                Pointer to the last value read from the memory location.

  @retval EFI_SUCCESS           The last data returned from the access matched the poll exit criteria.
  @retval EFI_UNSUPPORTED       BarIndex not valid for this PCI controller.
  @retval EFI_UNSUPPORTED       Offset is not valid for the BarIndex of this PCI controller.
  @retval EFI_TIMEOUT           Delay expired before a match occurred.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.

**/
EFI_STATUS
PciBusPollMem (
  IN EFI_PCI_IO_PROTOCOL           *This,
  IN  EFI_PCI_IO_PROTOCOL_WIDTH    Width,
  IN  UINT8                        BarIndex,
  IN  UINT64                       Offset,
  IN  UINT64                       Mask,
  IN  UINT64                       Value,
  IN  UINT64                       Delay,
  OUT UINT64                       *Result
  )
{
  EFI_STATUS Status;

  Status = PciBusMemRead (This, Width, BarIndex, Offset, 1, Result);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (((*Result & Mask) == Value) || (Delay == 0)) {
    return EFI_SUCCESS;
  }

  do {
    MicroSecondDelay (10);
    Status = PciBusMemRead (This, Width, BarIndex, Offset, 1, Result);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if ((*Result & Mask) == Value) {
      return EFI_SUCCESS;
    }

    if (Delay <= 100) {
      return EFI_TIMEOUT;
    }
    Delay -= 100;
  } while (TRUE);
}

/**
  Reads from the I/O space of a PCI controller. Returns either when the polling exit criteria is
  satisfied or after a defined duration.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Width                 Signifies the width of the memory or I/O operations.
  @param  BarIndex              The BAR index of the standard PCI Configuration header to use as the
                                base address for the memory operation to perform.
  @param  Offset                The offset within the selected BAR to start the memory operation.
  @param  Mask                  Mask used for the polling criteria.
  @param  Value                 The comparison value used for the polling exit criteria.
  @param  Delay                 The number of 100 ns units to poll.
  @param  Result                Pointer to the last value read from the memory location.

  @retval EFI_SUCCESS           The last data returned from the access matched the poll exit criteria.
  @retval EFI_UNSUPPORTED       BarIndex not valid for this PCI controller.
  @retval EFI_UNSUPPORTED       Offset is not valid for the BarIndex of this PCI controller.
  @retval EFI_TIMEOUT           Delay expired before a match occurred.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.

**/
EFI_STATUS
PciBusPollIo (
  IN EFI_PCI_IO_PROTOCOL           *This,
  IN  EFI_PCI_IO_PROTOCOL_WIDTH    Width,
  IN  UINT8                        BarIndex,
  IN  UINT64                       Offset,
  IN  UINT64                       Mask,
  IN  UINT64                       Value,
  IN  UINT64                       Delay,
  OUT UINT64                       *Result
  )
{
  EFI_STATUS Status;

  Status = PciBusIoRead (This, Width, BarIndex, Offset, 1, Result);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (((*Result & Mask) == Value) || (Delay == 0)) {
    return EFI_SUCCESS;
  }

  do {
    MicroSecondDelay (10);
    Status = PciBusIoRead (This, Width, BarIndex, Offset, 1, Result);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if ((*Result & Mask) == Value) {
      return EFI_SUCCESS;
    }

    if (Delay <= 100) {
      return EFI_TIMEOUT;
    }
    Delay -= 100;
  } while (TRUE);
}

/**
  Enable a PCI driver to access PCI controller registers in PCI configuration space.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Width                 Signifies the width of the memory operations.
  @param  Offset                The offset within the PCI configuration space for the PCI controller.
  @param  Count                 The number of PCI configuration operations to perform.
  @param  Buffer                For read operations, the destination buffer to store the results. For write
                                operations, the source buffer to write data from.


  @retval EFI_SUCCESS           The data was read from or written to the PCI controller.
  @retval EFI_UNSUPPORTED       The address range specified by Offset, Width, and Count is not
                                valid for the PCI configuration header of the PCI controller.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_INVALID_PARAMETER Buffer is NULL or Width is invalid.

**/
EFI_STATUS
PciBusConfigRead (
  IN EFI_PCI_IO_PROTOCOL              *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH    Width,
  IN     UINT32                       Offset,
  IN     UINTN                        Count,
  IN OUT VOID                         *Buffer
  )
{
  PCI_DEVICE_PRIVATE_DATA  *PrivateData;
  UINT8                    *Uint8Buffer;
  UINT16                   *Uint16Buffer;
  UINT32                   *Uint32Buffer;
  UINTN                    Index;

  PrivateData = (PCI_DEVICE_PRIVATE_DATA*) This;

  switch (Width) {
    case EfiPciIoWidthUint8:
      Uint8Buffer = (UINT8*) Buffer;
      for (Index = 0; Index < Count; Index++) {
        Uint8Buffer[Index] = PciSegmentRead8 (PrivateData->PciCfgBase + Offset);
      }
      break;
    case EfiPciIoWidthUint16:
      Uint16Buffer = (UINT16*) Buffer;
      for (Index = 0; Index < Count; Index++) {
        Uint16Buffer[Index] = PciSegmentRead16 (PrivateData->PciCfgBase + Offset);
      }
      break;
    case EfiPciIoWidthUint32:
      Uint32Buffer = (UINT32*) Buffer;
      for (Index = 0; Index < Count; Index++) {
        Uint32Buffer[Index] = PciSegmentRead32 (PrivateData->PciCfgBase + Offset);
      }
      break;
  }
  return EFI_SUCCESS;
}

/**
  Enable a PCI driver to access PCI controller registers in PCI configuration space.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Width                 Signifies the width of the memory operations.
  @param  Offset                The offset within the PCI configuration space for the PCI controller.
  @param  Count                 The number of PCI configuration operations to perform.
  @param  Buffer                For read operations, the destination buffer to store the results. For write
                                operations, the source buffer to write data from.


  @retval EFI_SUCCESS           The data was read from or written to the PCI controller.
  @retval EFI_UNSUPPORTED       The address range specified by Offset, Width, and Count is not
                                valid for the PCI configuration header of the PCI controller.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_INVALID_PARAMETER Buffer is NULL or Width is invalid.

**/
EFI_STATUS
PciBusConfigWrite (
  IN EFI_PCI_IO_PROTOCOL              *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH    Width,
  IN     UINT32                       Offset,
  IN     UINTN                        Count,
  IN OUT VOID                         *Buffer
  )
{
  PCI_DEVICE_PRIVATE_DATA  *PrivateData;
  UINT8                    *Uint8Buffer;
  UINT16                   *Uint16Buffer;
  UINT32                   *Uint32Buffer;
  UINTN                    Index;

  PrivateData = (PCI_DEVICE_PRIVATE_DATA*) This;

  switch (Width) {
    case EfiPciIoWidthUint8:
      Uint8Buffer = (UINT8*) Buffer;
      for (Index = 0; Index < Count; Index++) {
        PciSegmentWrite8 (PrivateData->PciCfgBase + Offset, Uint8Buffer[Index]);
      }
      break;
    case EfiPciIoWidthUint16:
      Uint16Buffer = (UINT16*) Buffer;
      for (Index = 0; Index < Count; Index++) {
        PciSegmentWrite16 (PrivateData->PciCfgBase + Offset, Uint16Buffer[Index]);
      }
      break;
    case EfiPciIoWidthUint32:
      Uint32Buffer = (UINT32*) Buffer;
      for (Index = 0; Index < Count; Index++) {
        PciSegmentWrite32 (PrivateData->PciCfgBase + Offset, Uint32Buffer[Index]);
      }
      break;
  }
  return EFI_SUCCESS;
}

/**
  Enables a PCI driver to copy one region of PCI memory space to another region of PCI
  memory space.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Width                 Signifies the width of the memory operations.
  @param  DestBarIndex          The BAR index in the standard PCI Configuration header to use as the
                                base address for the memory operation to perform.
  @param  DestOffset            The destination offset within the BAR specified by DestBarIndex to
                                start the memory writes for the copy operation.
  @param  SrcBarIndex           The BAR index in the standard PCI Configuration header to use as the
                                base address for the memory operation to perform.
  @param  SrcOffset             The source offset within the BAR specified by SrcBarIndex to start
                                the memory reads for the copy operation.
  @param  Count                 The number of memory operations to perform. Bytes moved is Width
                                size * Count, starting at DestOffset and SrcOffset.

  @retval EFI_SUCCESS           The data was copied from one memory region to another memory region.
  @retval EFI_UNSUPPORTED       DestBarIndex not valid for this PCI controller.
  @retval EFI_UNSUPPORTED       SrcBarIndex not valid for this PCI controller.
  @retval EFI_UNSUPPORTED       The address range specified by DestOffset, Width, and Count
                                is not valid for the PCI BAR specified by DestBarIndex.
  @retval EFI_UNSUPPORTED       The address range specified by SrcOffset, Width, and Count is
                                not valid for the PCI BAR specified by SrcBarIndex.
  @retval EFI_INVALID_PARAMETER Width is invalid.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.

**/
EFI_STATUS
PciBusCopyMem (
  IN EFI_PCI_IO_PROTOCOL              *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH    Width,
  IN     UINT8                        DestBarIndex,
  IN     UINT64                       DestOffset,
  IN     UINT8                        SrcBarIndex,
  IN     UINT64                       SrcOffset,
  IN     UINTN                        Count
  )
{
  EFI_STATUS  Status;
  UINTN       Index;
  UINTN       Stride;
  UINT64      Result;
  BOOLEAN     Forward;

  Stride = (UINTN)(1 << Width);

  Forward = TRUE;
  if ((DestOffset > SrcOffset) &&
      (DestOffset < (SrcOffset + Count * Stride)))
  {
    Forward     = FALSE;
    SrcOffset  = SrcOffset + (Count - 1) * Stride;
    DestOffset = DestOffset + (Count - 1) * Stride;
  }

  for (Index = 0; Index < Count; Index++) {
    Status = PciBusMemRead (This, Width, SrcBarIndex, SrcOffset, 1, &Result);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = PciBusMemWrite (This, Width, DestBarIndex, DestOffset, 1, &Result);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (Forward) {
      SrcOffset  += Stride;
      DestOffset += Stride;
    } else {
      SrcOffset  -= Stride;
      DestOffset -= Stride;
    }
  }
  return EFI_SUCCESS;
}

/**
  Provides the PCI controller-specific addresses needed to access system memory.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Operation             Indicates if the bus master is going to read or write to system memory.
  @param  HostAddress           The system memory address to map to the PCI controller.
  @param  NumberOfBytes         On input the number of bytes to map. On output the number of bytes
                                that were mapped.
  @param  DeviceAddress         The resulting map address for the bus master PCI controller to use to
                                access the hosts HostAddress.
  @param  Mapping               A resulting value to pass to Unmap().

  @retval EFI_SUCCESS           The range was mapped for the returned NumberOfBytes.
  @retval EFI_UNSUPPORTED       The HostAddress cannot be mapped as a common buffer.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_DEVICE_ERROR      The system hardware could not map the requested address.

**/
EFI_STATUS
PciBusMap (
  IN EFI_PCI_IO_PROTOCOL                *This,
  IN     EFI_PCI_IO_PROTOCOL_OPERATION  Operation,
  IN     VOID                           *HostAddress,
  IN OUT UINTN                          *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS           *DeviceAddress,
  OUT    VOID                           **Mapping
  )
{
  EFI_STATUS       Status;
  EDKII_IOMMU_PPI  *IoMmu;
  UINT64           IoMmuAttribute;

  Status = PeiServicesLocatePpi (
             &gEdkiiIoMmuPpiGuid,
             0,
             NULL,
             (VOID **) &IoMmu
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = IoMmu->Map (
                    IoMmu,
                    Operation,
                    HostAddress,
                    NumberOfBytes,
                    DeviceAddress,
                    Mapping
                    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  switch (Operation) {
    case EfiPciIoOperationBusMasterRead:
      IoMmuAttribute = EDKII_IOMMU_ACCESS_READ;
      break;
    case EfiPciIoOperationBusMasterWrite:
      IoMmuAttribute = EDKII_IOMMU_ACCESS_WRITE;
      break;
    case EfiPciIoOperationBusMasterCommonBuffer:
      IoMmuAttribute = EDKII_IOMMU_ACCESS_READ | EDKII_IOMMU_ACCESS_WRITE;
       break;
    default:
      ASSERT (FALSE);
      return EFI_INVALID_PARAMETER;
  }
  Status = IoMmu->SetAttribute (
                    IoMmu,
                    *Mapping,
                    IoMmuAttribute
                    );
  return Status;
}

/**
  Completes the Map() operation and releases any corresponding resources.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Mapping               The mapping value returned from Map().

  @retval EFI_SUCCESS           The range was unmapped.
  @retval EFI_DEVICE_ERROR      The data was not committed to the target system memory.

**/
EFI_STATUS
PciBusUnmap (
  IN EFI_PCI_IO_PROTOCOL           *This,
  IN  VOID                         *Mapping
  )
{
  EFI_STATUS       Status;
  EDKII_IOMMU_PPI  *IoMmu;

  Status = PeiServicesLocatePpi (
             &gEdkiiIoMmuPpiGuid,
             0,
             NULL,
             (VOID **) &IoMmu
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = IoMmu->SetAttribute (
                    IoMmu,
                    Mapping,
                    0
                    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = IoMmu->Unmap (
                    IoMmu,
                    Mapping
                    );
  return Status;
}

/**
  Allocates pages that are suitable for an EfiPciIoOperationBusMasterCommonBuffer
  or EfiPciOperationBusMasterCommonBuffer64 mapping.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Type                  This parameter is not used and must be ignored.
  @param  MemoryType            The type of memory to allocate, EfiBootServicesData or
                                EfiRuntimeServicesData.
  @param  Pages                 The number of pages to allocate.
  @param  HostAddress           A pointer to store the base system memory address of the
                                allocated range.
  @param  Attributes            The requested bit mask of attributes for the allocated range.

  @retval EFI_SUCCESS           The requested memory pages were allocated.
  @retval EFI_UNSUPPORTED       Attributes is unsupported. The only legal attribute bits are
                                MEMORY_WRITE_COMBINE, MEMORY_CACHED and DUAL_ADDRESS_CYCLE.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The memory pages could not be allocated.

**/
EFI_STATUS
PciBusAllocateBuffer (
  IN EFI_PCI_IO_PROTOCOL           *This,
  IN  EFI_ALLOCATE_TYPE            Type,
  IN  EFI_MEMORY_TYPE              MemoryType,
  IN  UINTN                        Pages,
  OUT VOID                         **HostAddress,
  IN  UINT64                       Attributes
  )
{
  EFI_STATUS       Status;
  EDKII_IOMMU_PPI  *IoMmu;

  Status = PeiServicesLocatePpi (
             &gEdkiiIoMmuPpiGuid,
             0,
             NULL,
             (VOID **) &IoMmu
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = IoMmu->AllocateBuffer (
                    IoMmu,
                    MemoryType,
                    Pages,
                    HostAddress,
                    Attributes
                    );
  return Status;
}

/**
  Frees memory that was allocated with AllocateBuffer().

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Pages                 The number of pages to free.
  @param  HostAddress           The base system memory address of the allocated range.

  @retval EFI_SUCCESS           The requested memory pages were freed.
  @retval EFI_INVALID_PARAMETER The memory range specified by HostAddress and Pages
                                was not allocated with AllocateBuffer().

**/
EFI_STATUS
PciBusFreeBuffer (
  IN EFI_PCI_IO_PROTOCOL           *This,
  IN  UINTN                        Pages,
  IN  VOID                         *HostAddress
  )
{
  EFI_STATUS       Status;
  EDKII_IOMMU_PPI  *IoMmu;

  Status = PeiServicesLocatePpi (
             &gEdkiiIoMmuPpiGuid,
             0,
             NULL,
             (VOID **) &IoMmu
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = IoMmu->FreeBuffer (
                    IoMmu,
                    Pages,
                    HostAddress
                    );
  return Status;
}

/**
  Flushes all PCI posted write transactions from a PCI host bridge to system memory.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.

  @retval EFI_SUCCESS           The PCI posted write transactions were flushed from the PCI host
                                bridge to system memory.
  @retval EFI_DEVICE_ERROR      The PCI posted write transactions were not flushed from the PCI
                                host bridge due to a hardware error.

**/
EFI_STATUS
PciBusFlush (
  IN EFI_PCI_IO_PROTOCOL  *This
  )
{
  return EFI_SUCCESS;
}

/**
  Retrieves this PCI controller's current PCI bus number, device number, and function number.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  SegmentNumber         The PCI controller's current PCI segment number.
  @param  BusNumber             The PCI controller's current PCI bus number.
  @param  DeviceNumber          The PCI controller's current PCI device number.
  @param  FunctionNumber        The PCI controller's current PCI function number.

  @retval EFI_SUCCESS           The PCI controller location was returned.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.

**/
EFI_STATUS
PciBusGetLocation (
  IN EFI_PCI_IO_PROTOCOL          *This,
  OUT UINTN                       *SegmentNumber,
  OUT UINTN                       *BusNumber,
  OUT UINTN                       *DeviceNumber,
  OUT UINTN                       *FunctionNumber
  )
{
  PCI_DEVICE_PRIVATE_DATA  *PrivateData;

  PrivateData = (PCI_DEVICE_PRIVATE_DATA*) This;

  *SegmentNumber = PrivateData->Sbdf.Seg;
  *BusNumber = PrivateData->Sbdf.Bus;
  *DeviceNumber = PrivateData->Sbdf.Dev;
  *FunctionNumber = PrivateData->Sbdf.Func;
  return EFI_SUCCESS;
}

/**
  Performs an operation on the attributes that this PCI controller supports. The operations include
  getting the set of supported attributes, retrieving the current attributes, setting the current
  attributes, enabling attributes, and disabling attributes.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Operation             The operation to perform on the attributes for this PCI controller.
  @param  Attributes            The mask of attributes that are used for Set, Enable, and Disable
                                operations.
  @param  Result                A pointer to the result mask of attributes that are returned for the Get
                                and Supported operations.

  @retval EFI_SUCCESS           The operation on the PCI controller's attributes was completed.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_UNSUPPORTED       one or more of the bits set in
                                Attributes are not supported by this PCI controller or one of
                                its parent bridges when Operation is Set, Enable or Disable.

**/
EFI_STATUS
PciBusAttributes (
  IN EFI_PCI_IO_PROTOCOL                       *This,
  IN  EFI_PCI_IO_PROTOCOL_ATTRIBUTE_OPERATION  Operation,
  IN  UINT64                                   Attributes,
  OUT UINT64                                   *Result OPTIONAL
  )
{
  PCI_DEVICE_PRIVATE_DATA  *PrivateData;
  UINT16  Command;
  UINT64  Supports;
  UINT64  UpStreamAttributes;
  UINT16  CurrentCommand;
  EFI_STATUS Status;

  PrivateData = (PCI_DEVICE_PRIVATE_DATA*) This;
  Command = 0;

  switch (Operation) {
    case EfiPciIoAttributeOperationGet:
      if (Result == NULL) {
        return EFI_INVALID_PARAMETER;
      }

      *Result = PrivateData->Attributes;
      return EFI_SUCCESS;

    case EfiPciIoAttributeOperationSupported:
      if (Result == NULL) {
        return EFI_INVALID_PARAMETER;
      }

      *Result = PrivateData->Supports;
      return EFI_SUCCESS;

    case EfiPciIoAttributeOperationSet:
      Status = PrivateData->PciDevice.PciIo.Attributes (
                                               &(PrivateData->PciDevice.PciIo),
                                               EfiPciIoAttributeOperationEnable,
                                               Attributes,
                                               NULL
                                               );
      if (EFI_ERROR (Status)) {
        return EFI_UNSUPPORTED;
      }

      Status = PrivateData->PciDevice.PciIo.Attributes (
                                               &(PrivateData->PciDevice.PciIo),
                                               EfiPciIoAttributeOperationDisable,
                                               (~Attributes) & (PrivateData->Supports),
                                               NULL
                                               );
      if (EFI_ERROR (Status)) {
        return EFI_UNSUPPORTED;
      }

      return EFI_SUCCESS;

    case EfiPciIoAttributeOperationEnable:
    case EfiPciIoAttributeOperationDisable:
      break;

    default:
      return EFI_INVALID_PARAMETER;
  }

  if ((Attributes & EFI_PCI_DEVICE_ENABLE) == EFI_PCI_DEVICE_ENABLE) {
    Attributes &= (PrivateData->Supports);
  }

  if ((Attributes & (EFI_PCI_IO_ATTRIBUTE_VGA_IO | EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO)) != 0) {
    if ((Attributes & (EFI_PCI_IO_ATTRIBUTE_VGA_IO_16 | EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO_16)) != 0) {
      return EFI_UNSUPPORTED;
    }
  }

  Supports = (PrivateData->Supports) & Attributes;
  if (Supports != Attributes) {
    return EFI_UNSUPPORTED;
  }

  if (PrivateData->Parent == NULL) {
    return EFI_SUCCESS;
  }

  if ((Attributes & EFI_PCI_IO_ATTRIBUTE_IO) != 0) {
    Command |= EFI_PCI_COMMAND_IO_SPACE;
  }

  if ((Attributes & EFI_PCI_IO_ATTRIBUTE_MEMORY) != 0) {
    Command |= EFI_PCI_COMMAND_MEMORY_SPACE;
  }

  if ((Attributes & EFI_PCI_IO_ATTRIBUTE_BUS_MASTER) != 0) {
    Command |= EFI_PCI_COMMAND_BUS_MASTER;
  }

  UpStreamAttributes = Attributes &
                       (~(EFI_PCI_IO_ATTRIBUTE_IO     |
                          EFI_PCI_IO_ATTRIBUTE_MEMORY |
                          EFI_PCI_IO_ATTRIBUTE_BUS_MASTER
                          )
                       );

  PrivateData->PciDevice.PciIo.Pci.Read (This, EfiPciIoWidthUint16, PCI_COMMAND_OFFSET, 1, &CurrentCommand);
  if (Operation == EfiPciIoAttributeOperationEnable) {
    CurrentCommand |= Command;
    Status = PrivateData->PciDevice.PciIo.Pci.Write (
                                                &(PrivateData->PciDevice.PciIo),
                                                EfiPciIoWidthUint16,
                                                PCI_COMMAND_OFFSET,
                                                1,
                                                &CurrentCommand
                                                );
    PrivateData->Attributes |= Attributes;
    Status = PrivateData->Parent->PciDevice.PciIo.Attributes (
                                                    &(PrivateData->Parent->PciDevice.PciIo),
                                                    EfiPciIoAttributeOperationEnable,
                                                    UpStreamAttributes,
                                                    NULL
                                                    );
  } else {
    CurrentCommand &= ~Command;
    Status = PrivateData->PciDevice.PciIo.Pci.Write (
                                                &(PrivateData->PciDevice.PciIo),
                                                EfiPciIoWidthUint16,
                                                PCI_COMMAND_OFFSET,
                                                1,
                                                &CurrentCommand
                                                );
    PrivateData->Attributes &= ~Attributes;
  }
  return Status;
}

/**
  Gets the attributes that this PCI controller supports setting on a BAR using
  SetBarAttributes(), and retrieves the list of resource descriptors for a BAR.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  BarIndex              The BAR index of the standard PCI Configuration header to use as the
                                base address for resource range. The legal range for this field is 0..5.
  @param  Supports              A pointer to the mask of attributes that this PCI controller supports
                                setting for this BAR with SetBarAttributes().
  @param  Resources             A pointer to the resource descriptors that describe the current
                                configuration of this BAR of the PCI controller.

  @retval EFI_SUCCESS           If Supports is not NULL, then the attributes that the PCI
                                controller supports are returned in Supports. If Resources
                                is not NULL, then the resource descriptors that the PCI
                                controller is currently using are returned in Resources.
  @retval EFI_INVALID_PARAMETER Both Supports and Attributes are NULL.
  @retval EFI_UNSUPPORTED       BarIndex not valid for this PCI controller.
  @retval EFI_OUT_OF_RESOURCES  There are not enough resources available to allocate
                                Resources.
**/
EFI_STATUS
PciBusGetBarAtributes (
  IN EFI_PCI_IO_PROTOCOL             *This,
  IN  UINT8                          BarIndex,
  OUT UINT64                         *Supports, OPTIONAL
  OUT VOID                           **Resources OPTIONAL
  )
{
  UINT32 BarValue;
  PCI_DEVICE_PRIVATE_DATA  *PrivateData;
  PCI_BAR_TYPE BarType;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  *Descriptor;
  EFI_ACPI_END_TAG_DESCRIPTOR        *End;

  PrivateData = (PCI_DEVICE_PRIVATE_DATA*) This;
  BarValue = PciSegmentRead32 (PrivateData->PciCfgBase + (R_BASE_ADDRESS_OFFSET_0 + (0x4 * BarIndex)));

  if (Resources != NULL) {
    Descriptor = AllocateZeroPool (sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) + sizeof (EFI_ACPI_END_TAG_DESCRIPTOR));
    if (Descriptor == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    *Resources = Descriptor;

    Descriptor->Desc         = ACPI_ADDRESS_SPACE_DESCRIPTOR;
    Descriptor->Len          = (UINT16)(sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) - 3);

    if (BarValue & 0x1) {
      // IO bar
      BarValue &= 0xFFFFFFFC;
      if ((BarValue & 0xFFFF0000) != 0) {
        //
        // It is a IO32 bar
        //
        BarType   = PciBarTypeIo32;
        Descriptor->AddrLen    = (~(BarValue) + 1);
        Descriptor->AddrRangeMax = Descriptor->AddrLen - 1;
      } else {
        //
        // It is a IO16 bar
        //
        BarType   = PciBarTypeIo16;
        Descriptor->AddrLen    = 0x0000FFFF & (~(BarValue) + 1);
        Descriptor->AddrRangeMax = Descriptor->AddrLen - 1;
      }
    } else {
      BarValue &= 0xFFFFFFF0;
      if ((BarValue & 0x6) == 0) {
        // 32bit BAR
        if (((BarValue & 0x8) != 0)) {
          BarType = PciBarTypePMem32;
        } else {
          BarType = PciBarTypeMem32;
        }
      } else {
        if (((BarValue & 0x8) != 0)) {
          BarType = PciBarTypePMem64;
        } else {
          BarType = PciBarTypeMem64;
        }
      }
      Descriptor->AddrLen    = (~(BarValue) + 1);
      Descriptor->AddrRangeMax = Descriptor->AddrLen - 1;
    }
    Descriptor->AddrRangeMin = BarValue;

    switch (BarType) {
      case PciBarTypeIo16:
      case PciBarTypeIo32:
        //
        // Io
        //
        Descriptor->ResType = ACPI_ADDRESS_SPACE_TYPE_IO;
        break;

      case PciBarTypePMem32:
        //
        // prefetchable
        //
        Descriptor->SpecificFlag = EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_CACHEABLE_PREFETCHABLE;
      //
      // Fall through
      //
      case PciBarTypeMem32:
        //
        // Mem
        //
        Descriptor->ResType = ACPI_ADDRESS_SPACE_TYPE_MEM;
        //
        // 32 bit
        //
        Descriptor->AddrSpaceGranularity = 32;
        break;

      case PciBarTypePMem64:
        //
        // prefetchable
        //
        Descriptor->SpecificFlag = EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_CACHEABLE_PREFETCHABLE;
      //
      // Fall through
      //
      case PciBarTypeMem64:
        //
        // Mem
        //
        Descriptor->ResType = ACPI_ADDRESS_SPACE_TYPE_MEM;
        //
        // 64 bit
        //
        Descriptor->AddrSpaceGranularity = 64;
        break;

      default:
        break;
    }

    //
    // put the checksum
    //
    End           = (EFI_ACPI_END_TAG_DESCRIPTOR *)(Descriptor + 1);
    End->Desc     = ACPI_END_TAG_DESCRIPTOR;
    End->Checksum = 0;
  }
  return EFI_SUCCESS;
}

/**
  Sets the attributes for a range of a BAR on a PCI controller.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Attributes            The mask of attributes to set for the resource range specified by
                                BarIndex, Offset, and Length.
  @param  BarIndex              The BAR index of the standard PCI Configuration header to use as the
                                base address for resource range. The legal range for this field is 0..5.
  @param  Offset                A pointer to the BAR relative base address of the resource range to be
                                modified by the attributes specified by Attributes.
  @param  Length                A pointer to the length of the resource range to be modified by the
                                attributes specified by Attributes.

  @retval EFI_SUCCESS           The set of attributes specified by Attributes for the resource
                                range specified by BarIndex, Offset, and Length were
                                set on the PCI controller, and the actual resource range is returned
                                in Offset and Length.
  @retval EFI_INVALID_PARAMETER Offset or Length is NULL.
  @retval EFI_UNSUPPORTED       BarIndex not valid for this PCI controller.
  @retval EFI_OUT_OF_RESOURCES  There are not enough resources to set the attributes on the
                                resource range specified by BarIndex, Offset, and
                                Length.

**/
EFI_STATUS
PciBusSetBarAttributes (
  IN EFI_PCI_IO_PROTOCOL              *This,
  IN     UINT64                       Attributes,
  IN     UINT8                        BarIndex,
  IN OUT UINT64                       *Offset,
  IN OUT UINT64                       *Length
  )
{
  return EFI_SUCCESS;
}

/**
  Prepare function pointers for the PCI_DEVICE_PPI.

  @param[in] PciDevPriv  Instance of PCI_DEVICE_PRIVATE_DATA.
**/
VOID
PciGetPciIoAccess (
  IN OUT PCI_DEVICE_PRIVATE_DATA  *PciDevPriv
  )
{
  PciDevPriv->PciDevice.PciIo.PollMem = PciBusPollMem;
  PciDevPriv->PciDevice.PciIo.PollIo = PciBusPollIo;
  PciDevPriv->PciDevice.PciIo.Mem.Read = PciBusMemRead;
  PciDevPriv->PciDevice.PciIo.Mem.Write = PciBusMemWrite;
  PciDevPriv->PciDevice.PciIo.Io.Read = PciBusIoRead;
  PciDevPriv->PciDevice.PciIo.Io.Write = PciBusIoWrite;
  PciDevPriv->PciDevice.PciIo.Pci.Read = PciBusConfigRead;
  PciDevPriv->PciDevice.PciIo.Pci.Write = PciBusConfigWrite;
  PciDevPriv->PciDevice.PciIo.CopyMem = PciBusCopyMem;
  PciDevPriv->PciDevice.PciIo.Map = PciBusMap;
  PciDevPriv->PciDevice.PciIo.Unmap = PciBusUnmap;
  PciDevPriv->PciDevice.PciIo.AllocateBuffer = PciBusAllocateBuffer;
  PciDevPriv->PciDevice.PciIo.FreeBuffer = PciBusFreeBuffer;
  PciDevPriv->PciDevice.PciIo.Flush = PciBusFlush;
  PciDevPriv->PciDevice.PciIo.GetLocation = PciBusGetLocation;
  PciDevPriv->PciDevice.PciIo.Attributes = PciBusAttributes;
  PciDevPriv->PciDevice.PciIo.GetBarAttributes = PciBusGetBarAtributes;
  PciDevPriv->PciDevice.PciIo.SetBarAttributes = PciBusSetBarAttributes;
}

EFI_PEI_PPI_DESCRIPTOR mPciDevicesReady = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gPciDevicesReadyPpiGuid,
  NULL
};

/**
  PcieBus entry point

  @param[in] FileHandle           The file handle of the file, Not used.
  @param[in] PeiServices          General purpose services available to every PEIM.

  @retval EFI_SUCCESS             The function completes successfully
**/
EFI_STATUS
EFIAPI
PcieBusEntryPoint (
  IN  EFI_PEI_FILE_HANDLE    FileHandle,
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  PCI_ROOT_BRIDGE  *RootBridgeList;
  UINTN            Count;
  UINTN            Index;
  EFI_STATUS       Status;

  DEBUG ((DEBUG_ERROR, "%a - start\n", __FUNCTION__));

  Count = 0;
  RootBridgeList = NULL;
  RootBridgeList = PciHostBridgeGetRootBridges (&Count);
  if (RootBridgeList == NULL || Count == 0) {
    DEBUG ((DEBUG_INFO, "No PCI root bridges on the system\n"));
    return EFI_UNSUPPORTED;
  }

  for (Index = 0; Index < Count; Index++) {
    PcieRootBridgeEnumerateEssentialDevices (&RootBridgeList[Index], Index);
  }

  Status = PeiServicesInstallPpi (&mPciDevicesReady);
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}