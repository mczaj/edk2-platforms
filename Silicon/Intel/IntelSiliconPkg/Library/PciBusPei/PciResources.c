/** @file
  Routines handling PEI PCIe resource allocation

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <Base.h>
#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/PcieHelperLib.h>
#include <Library/DebugLib.h>
#include <Library/PciSegmentLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PeiServicesLib.h>
#include <Protocol/DevicePath.h>
#include <Library/DevicePathLib.h>
#include <IndustryStandard/Pci.h>
#include "PcieBusPei.h"

/**
  This function discovers devices, builds device structures and resource list for the underlying devices.

  @param[in] This  Pointer to current bridge structure.
**/
VOID
EnumerateBridgeResources (
  IN P2P_BRIDGE  *This
  )
{
  SBDF               Sbdf;
  UINT32             BarOffset;
  UINT32             BarIndexLimit;
  UINT64             PciCfgBase;
  UINT32             BarValue;
  UINT32             BarSizeValue;
  UINT64             BarSize;
  UINT8              BarIndex;
  BOOLEAN            SkipNextBar;
  PEI_RESOURCE_TYPE  BarType;
  PEI_RESOURCE_NODE  *Resource;
  P2P_BRIDGE         *Child;
  UINT8              Dev;
  UINT8              Func;
  PCI_DEVICE_PRIVATE_DATA  *PrivateData;

  Sbdf.Seg = This->Device->Sbdf.Seg;
  Sbdf.Bus = This->SecBus;
  for (Dev = 0; Dev <= PCI_MAX_DEVICE; Dev++) {
    Sbdf.Dev = Dev;
    for (Func = 0; Func <= PCI_MAX_FUNC; Func++) {
      Sbdf.Func = Func;
      PciCfgBase = SbdfToBase (Sbdf);
      if (IsDevicePresent (PciCfgBase)) {
        //
        // get resources from the device if it is not decoding already and is essential/is a bridge
        //
        Sbdf.PcieCap = PcieBaseFindCapId (SbdfToBase (Sbdf), EFI_PCI_CAPABILITY_ID_PCIEXP);
        if ((IsBridgeDevice (Sbdf) || PciIsDeviceEssential (Sbdf)) && !IsDeviceDecodingResources (Sbdf)) {
          if (IsBridgeDevice (Sbdf)) {
            BarIndexLimit = 1;
          } else {
            BarIndexLimit = 5;
          }
          SkipNextBar = FALSE;

          PrivateData = AllocateZeroPool (sizeof (PCI_DEVICE_PRIVATE_DATA));
          if (PrivateData == NULL) {
            DEBUG ((DEBUG_ERROR, "%a - out of memory for PCI_DEVICE_PRIVATE_DATA\n", __FUNCTION__));
            ASSERT (FALSE);
          }
          PrivateData->Signature = PCI_DEVICE_PRIVATE_DATA_SIGNATURE;
          PrivateData->PciCfgBase = PciCfgBase;
          PrivateData->Sbdf = Sbdf;
          PrivateData->Supports = (EFI_PCI_IO_ATTRIBUTE_IO | EFI_PCI_IO_ATTRIBUTE_MEMORY | EFI_PCI_IO_ATTRIBUTE_BUS_MASTER);
          PrivateData->Parent = This->Device;
          PciGetPciIoAccess (PrivateData);

          for (BarIndex = 0; BarIndex <= BarIndexLimit; BarIndex++) {
            if (SkipNextBar) {
              SkipNextBar = FALSE;
              continue;
            }
            BarOffset = R_BASE_ADDRESS_OFFSET_0 + BarIndex * 0x4;
            BarValue = PciSegmentRead32 (PciCfgBase + BarOffset);
            PciSegmentWrite32 (PciCfgBase + BarOffset, MAX_UINT32);
            BarSizeValue = PciSegmentRead32 (PciCfgBase + BarOffset);

            if (BarValue == BarSizeValue) {
              continue;
            }
            if (BarValue & BIT0) {
              //
              // IO bar
              //
              BarSize = (UINT16)~(BarSizeValue & (~BIT0)) + 1;
              BarType = IoResource;
            } else {
              //
              // MEM bar
              //
              BarSize = ~(BarSizeValue & (~0xF)) + 1;
              BarType = MemResource;
              if (BarValue & (BIT2)) {
                //
                // for 64 bit BAR allow only smaller than 4GB
                //
                if (BarSize <= SIZE_2GB) {
                  SkipNextBar = TRUE;
                } else {
                  //
                  // Invalidate already assigned resource pools
                  //
                  RemoveResourceNodesBySbdf (This, Sbdf);
                  PrivateData->Supports = 0;
                  break;
                }
              }
            }

            //
            // Build resource node
            //
            Resource = (PEI_RESOURCE_NODE *) AllocateZeroPool (sizeof (PEI_RESOURCE_NODE));
            if (Resource == NULL) {
              DEBUG ((DEBUG_ERROR, "%a - out of memory for PEI_RESOURCE_NODE\n"));
              ASSERT (FALSE);
            }
            Resource->Signature = PEI_PCI_RESOURCE_SIGNATURE;
            Resource->Bar = BarIndex;
            Resource->Length = (UINT32) BarSize;
            Resource->Alignment = Resource->Length - 1;
            Resource->ResType = BarType;
            Resource->Device = PrivateData;
            InsertTailList (&This->ResourceListHead, &Resource->Link);
          }
          if (!IsBridgeDevice (Sbdf)) {
            InsertTailList (&This->EndpointListHead, &PrivateData->Link);
          } else {
            //
            // enumerate resources under child bridge
            //
            Child = (P2P_BRIDGE *) AllocateZeroPool (sizeof (P2P_BRIDGE));
            if (Child == NULL) {
              DEBUG ((DEBUG_ERROR, "%a - out of memory for P2P_BRIDGE\n"));
              ASSERT (FALSE);
            }
            Child->Signature = PEI_P2P_BRIDGE_SIGNATURE;
            Child->SecBus = PciSegmentRead8 (PciCfgBase + PCI_BRIDGE_SECONDARY_BUS_REGISTER_OFFSET);
            Child->Parent = This;
            Child->Device = PrivateData;

            InsertTailList (&This->ChildBridgeListHead, &Child->Link);
            InitializeListHead (&Child->ChildBridgeListHead);
            InitializeListHead (&Child->ResourceListHead);
            InitializeListHead (&Child->EndpointListHead);
            EnumerateBridgeResources (Child);
          }
        }
        if (!IsMultifunctionDevice (PciCfgBase) && Func == 0) {
          break;
        }
      } else {
        if (Func == 0) {
          break;
        }
      }
    }
  }
}

/**
  This function performs necessary operations on the resource tree in order to get the optimal resource allocation.

  @param[in] Bridge  Pointer to current bridge structure.
**/
EFI_STATUS
AlignResourceTree (
  IN P2P_BRIDGE  *Bridge
  )
{
  LIST_ENTRY         *Link;
  P2P_BRIDGE         *ChildBridge;
  PEI_RESOURCE_NODE  *Resource1;
  PEI_RESOURCE_NODE  *Resource2;
  PEI_RESOURCE_NODE  *FirstResource;
  PEI_RESOURCE_NODE  *LastResource;
  PEI_RESOURCE_NODE  *Aperture;

  Link = GetFirstNode (&Bridge->ChildBridgeListHead);

  while (!IsNull (&Bridge->ChildBridgeListHead, Link)) {
    ChildBridge = PEI_P2P_BRIDGE_FROM_LINK (Link);
    AlignResourceTree (ChildBridge);
    Link = GetNextNode (&Bridge->ChildBridgeListHead, Link);
  }

  BridgeSortResourceList (Bridge);

  //
  // assign offsets to MEM resources
  //
  Resource1 = BridgeGetFirstResourceNode (Bridge, (MemResource | MemAperture));
  Resource2 = BridgeGetNextResourceNode (Bridge, Resource1, (MemResource | MemAperture));

  while (Resource1 != NULL && Resource2 != NULL) {
    Resource2->Offset = Resource1->Offset + Resource1->Length;
    if (Resource2->Offset & (Resource2->Length - 1)) {
      Resource2->Offset &= (~(Resource2->Length - 1));
      Resource2->Offset += Resource2->Length;
    }
    Resource1 = Resource2;
    Resource2 = BridgeGetNextResourceNode (Bridge, Resource1, (MemResource | MemAperture));
  }

  //
  // assign offsets to IO resources
  //
  Resource1 = BridgeGetFirstResourceNode (Bridge, (IoResource | IoAperture));
  Resource2 = BridgeGetNextResourceNode (Bridge, Resource1, (IoResource | IoAperture));

  while (Resource1 != NULL && Resource2 != NULL) {
    Resource2->Offset = Resource1->Offset + Resource1->Length;
    if (Resource2->Offset & (Resource2->Length - 1)) {
      Resource2->Offset &= (~(Resource2->Length - 1));
      Resource2->Offset += Resource2->Length;
    }
    Resource1 = Resource2;
    Resource2 = BridgeGetNextResourceNode (Bridge, Resource1, (IoResource | IoAperture));
  }

  //
  // publish apertures to parent bridge
  //
  if (Bridge->Parent != NULL) {
    LastResource = BridgeGetLastResourceNode (Bridge, (MemResource | MemAperture));
    if (LastResource != NULL) {
      FirstResource = BridgeGetFirstResourceNode (Bridge, (MemResource | MemAperture));
      Aperture = (PEI_RESOURCE_NODE *) AllocateZeroPool (sizeof (PEI_RESOURCE_NODE));
      if (Aperture == NULL) {
        DEBUG ((DEBUG_ERROR, "%a - out of memory for aperture descriptor\n", __FUNCTION__));
        return EFI_OUT_OF_RESOURCES;
      }
      Aperture->Signature = PEI_PCI_RESOURCE_SIGNATURE;
      Aperture->Length = LastResource->Offset + LastResource->Length;
      if (Aperture->Length & 0x000FFFFF) {
        Aperture->Length = (Aperture->Length & 0xFFF00000) + 0x100000;
      }
      Aperture->Alignment = MAX (FirstResource->Alignment, Aperture->Length - 1);
      Aperture->ResType = MemAperture;
      Aperture->Device = Bridge->Device;
      InsertTailList (&Bridge->Parent->ResourceListHead, &Aperture->Link);
    }

    LastResource = BridgeGetLastResourceNode (Bridge, (IoResource | IoAperture));
    if (LastResource != NULL) {
      FirstResource = BridgeGetFirstResourceNode (Bridge, (IoResource | IoAperture));
      Aperture = (PEI_RESOURCE_NODE *) AllocateZeroPool (sizeof (PEI_RESOURCE_NODE));
      if (Aperture == NULL) {
        DEBUG ((DEBUG_ERROR, "%a - out of memory for aperture descriptor\n", __FUNCTION__));
        return EFI_OUT_OF_RESOURCES;
      }
      Aperture->Signature = PEI_PCI_RESOURCE_SIGNATURE;
      Aperture->Length = LastResource->Offset + LastResource->Length;
      if (Aperture->Length & 0x0FFF) {
        Aperture->Length = (Aperture->Length & 0xF000) + 0x1000;
      }
      Aperture->Alignment = MAX (FirstResource->Alignment, Aperture->Length - 1);
      Aperture->ResType = IoAperture;
      Aperture->Device = Bridge->Device;
      InsertTailList (&Bridge->Parent->ResourceListHead, &Aperture->Link);
    }
  }
  return EFI_SUCCESS;
}

/**
  This function programmes prepared MEM resources to the devices.

  @param[in] Bridge    Pointer to the current bridge structure.
  @param[in] MemBase   Current memory base address.
  @param[in] MemLimit  Current memory limit.

  @retval EFI_OUT_OF_RESOURCES if memory limit has been reached, EFI_SUCCESS otherwise.
**/
EFI_STATUS
ApplyMemResources (
  IN P2P_BRIDGE  *Bridge,
  IN UINT32      MemBase,
  IN UINT32      MemLimit
  )
{
  EFI_STATUS         Status;
  LIST_ENTRY         *BridgeLink;
  P2P_BRIDGE         *ChildBridge;
  PEI_RESOURCE_NODE  *Resource;
  UINT32             Register;

  Status = EFI_SUCCESS;

  Resource = BridgeGetFirstResourceNode(Bridge, (MemResource | MemAperture));

  while (Resource != NULL) {
    if (Resource->ResType & MemResource) {
      Register = MemBase + Resource->Offset;
      Resource->Device->PciDevice.PciIo.Pci.Write (
                                              &Resource->Device->PciDevice.PciIo,
                                              EfiPciIoWidthUint32,
                                              R_BASE_ADDRESS_OFFSET_0 + (Resource->Bar * 0x4),
                                              1,
                                              &Register
                                              );

      if ((MemBase + Resource->Offset + Resource->Length - 1) > MemLimit) {
        DEBUG ((DEBUG_ERROR, "%a - memory limit reached\n", __FUNCTION__));
        return EFI_OUT_OF_RESOURCES;
      }
    } else if (Resource->ResType & MemAperture) {
      BridgeLink = GetFirstNode (&Bridge->ChildBridgeListHead);
      Register = ((MemBase + Resource->Offset) >> 16) | ((MemBase + Resource->Offset + Resource->Length - 1) & 0xFFFF0000);
      Resource->Device->PciDevice.PciIo.Pci.Write (
                                              &Resource->Device->PciDevice.PciIo,
                                              EfiPciIoWidthUint32,
                                              R_PCI_BRIDGE_MBL,
                                              1,
                                              &Register
                                              );

      while (!IsNull (&Bridge->ChildBridgeListHead, BridgeLink)) {
        ChildBridge = PEI_P2P_BRIDGE_FROM_LINK (BridgeLink);
        if (ChildBridge->Device == Resource->Device) {
          break;
        }
        BridgeLink = GetNextNode (&Bridge->ChildBridgeListHead, BridgeLink);
      }
      Status = ApplyMemResources (ChildBridge, (MemBase + Resource->Offset), (MemBase + Resource->Offset + Resource->Length - 1));
      if (EFI_ERROR (Status)) {
        break;
      }
    }
    Resource = BridgeGetNextResourceNode (Bridge, Resource, (MemResource | MemAperture));
  }
  return Status;
}

/**
  This function programmes prepared IO resources to the devices.

  @param[in] Bridge    Pointer to the current bridge structure.
  @param[in] MemBase   Current IO base address.
  @param[in] MemLimit  Current IO limit.

  @retval EFI_OUT_OF_RESOURCES if IO limit has been reached, EFI_SUCCESS otherwise.
**/
EFI_STATUS
ApplyIoResources (
  IN P2P_BRIDGE  *Bridge,
  IN UINT32      IoBase,
  IN UINT32      IoLimit
  )
{
  EFI_STATUS         Status;
  LIST_ENTRY         *BridgeLink;
  P2P_BRIDGE         *ChildBridge;
  PEI_RESOURCE_NODE  *Resource;
  UINT32             Register;

  Status = EFI_SUCCESS;

  Resource = BridgeGetFirstResourceNode (Bridge, (IoResource | IoAperture));

  while (Resource != NULL) {
    if (Resource->ResType & IoResource) {
      Register = IoBase + Resource->Offset;
      Resource->Device->PciDevice.PciIo.Pci.Write (
                                              &Resource->Device->PciDevice.PciIo,
                                              EfiPciIoWidthUint32,
                                              R_BASE_ADDRESS_OFFSET_0 + (Resource->Bar * 0x4),
                                              1,
                                              &Register
                                              );

      if ((IoBase + Resource->Offset + Resource->Length - 1) > IoLimit) {
        DEBUG ((DEBUG_ERROR, "%a - IO limit reached\n", __FUNCTION__));
        return EFI_OUT_OF_RESOURCES;
      }
    } else if (Resource->ResType & IoAperture) {
      BridgeLink = GetFirstNode (&Bridge->ChildBridgeListHead);
      Register = ((IoBase + Resource->Offset) >> 8) | ((IoBase + Resource->Offset + Resource->Length - 1) & 0xFF00);
      Resource->Device->PciDevice.PciIo.Pci.Write (
                                              &Resource->Device->PciDevice.PciIo,
                                              EfiPciIoWidthUint32,
                                              R_PCI_BRIDGE_IOBL,
                                              1,
                                              &Register
                                              );

      while (!IsNull (&Bridge->ChildBridgeListHead, BridgeLink)) {
        ChildBridge = PEI_P2P_BRIDGE_FROM_LINK (BridgeLink);
        if (ChildBridge->Device == Resource->Device) {
          break;
        }
        BridgeLink = GetNextNode (&Bridge->ChildBridgeListHead, BridgeLink);
      }
      Status = ApplyIoResources (ChildBridge, IoBase + Resource->Offset, IoBase + Resource->Length - 1);
      if (EFI_ERROR (Status)) {
        break;
      }
    }
    Resource = BridgeGetNextResourceNode (Bridge, Resource, (IoResource | IoAperture));
  }
  return Status;
}

/**
  This function performs initial PCI resource programming.

  @param[in] Bridge    Pointer to current bridge structure.
  @param[in] MemLimit  Memory limit
  @param[in] IoLimit   IO limit
**/
VOID
InitResources (
  IN P2P_BRIDGE  *Bridge,
  IN UINT32      MemLimit,
  IN UINT32      IoLimit
  )
{
  LIST_ENTRY  *Link;
  P2P_BRIDGE  *ChildBridge;
  UINT32      Register;

  Link = GetFirstNode (&Bridge->ChildBridgeListHead);

  while (!IsNull (&Bridge->ChildBridgeListHead, Link)) {
    ChildBridge = PEI_P2P_BRIDGE_FROM_LINK (Link);
    InitResources (ChildBridge, MemLimit, IoLimit);
    Link = GetNextNode (&Bridge->ChildBridgeListHead, Link);
  }

  if (Bridge->Parent != NULL) {
    Register = (MemLimit >> 16) | (MemLimit & 0xFFFF0000);
    Bridge->Device->PciDevice.PciIo.Pci.Write (&Bridge->Device->PciDevice.PciIo, EfiPciIoWidthUint32, R_PCI_BRIDGE_MBL, 1, &Register);
    Register = (IoLimit >> 8) | (IoLimit & 0xFF00);
    Bridge->Device->PciDevice.PciIo.Pci.Write (&Bridge->Device->PciDevice.PciIo, EfiPciIoWidthUint32, R_PCI_BRIDGE_IOBL, 1, &Register);
  }
}

EFI_STATUS
PreparePciIoAccess (
  IN P2P_BRIDGE                *This,
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  LIST_ENTRY                *Link;
  PCI_DEVICE_PATH           PciDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *UpdatedDevicePath;
  EFI_PEI_PPI_DESCRIPTOR    *PpiDescriptor;
  PCI_DEVICE_PRIVATE_DATA   *Device;
  P2P_BRIDGE                *Bridge;

  Link = GetFirstNode (&This->EndpointListHead);

  while (!IsNull (&This->EndpointListHead, Link)) {
    Device = PCI_DEVICE_PRIVATE_DATA_FROM_LINK (Link);

    if (PciIsDeviceEssential (Device->Sbdf)) {
      PciDevicePath.Header.Type = HARDWARE_DEVICE_PATH;
      PciDevicePath.Header.SubType = HW_PCI_DP;
      SetDevicePathNodeLength (&PciDevicePath.Header, sizeof (PCI_DEVICE_PATH));
      PciDevicePath.Device = (UINT8) Device->Sbdf.Dev;
      PciDevicePath.Function = (UINT8) Device->Sbdf.Func;
      Device->PciDevice.DevicePath = AppendDevicePathNode (DevicePath, (EFI_DEVICE_PATH_PROTOCOL*) &PciDevicePath);

      DEBUG ((DEBUG_INFO, "PcieBusPei: installing PciIoAccess for device %s\n", ConvertDevicePathToText (AppendDevicePathNode (DevicePath, (EFI_DEVICE_PATH_PROTOCOL*) &PciDevicePath), FALSE, FALSE)));

      PpiDescriptor = AllocateZeroPool (sizeof (EFI_PEI_PPI_DESCRIPTOR));
      if (PpiDescriptor == NULL) {
        DEBUG ((DEBUG_ERROR, "%a - out of memory for PPI descriptor\n", __FUNCTION__));
        return EFI_OUT_OF_RESOURCES;
      }
      PpiDescriptor->Flags = EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST;
      PpiDescriptor->Guid  = &gEdkiiPeiPciDevicePpiGuid;
      PpiDescriptor->Ppi   = Device;
      PeiServicesInstallPpi (PpiDescriptor);
    }
    Link = GetNextNode (&This->EndpointListHead, Link);
  }

  Link = GetFirstNode (&This->ChildBridgeListHead);

  while (!IsNull (&This->ChildBridgeListHead, Link)) {
    Bridge = PEI_P2P_BRIDGE_FROM_LINK (Link);
    PciDevicePath.Header.Type = HARDWARE_DEVICE_PATH;
    PciDevicePath.Header.SubType = HW_PCI_DP;
    SetDevicePathNodeLength (&PciDevicePath.Header, sizeof (PCI_DEVICE_PATH));
    PciDevicePath.Device = (UINT8) Bridge->Device->Sbdf.Dev;
    PciDevicePath.Function = (UINT8) Bridge->Device->Sbdf.Func;
    UpdatedDevicePath = AppendDevicePathNode (DevicePath, (EFI_DEVICE_PATH_PROTOCOL*) &PciDevicePath);
    PreparePciIoAccess (Bridge, UpdatedDevicePath);
    Link = GetNextNode (&This->ChildBridgeListHead, Link);
  }
  return EFI_SUCCESS;
}

/**
  THis function enables bridge devices.

  @param[in] This  Pointer to current bridge structure.
**/
VOID
EnableBridgeDevices (
  IN P2P_BRIDGE  *This
  )
{
  LIST_ENTRY  *Link;
  P2P_BRIDGE  *Bridge;
  UINT64      Attributes;

  Link = GetFirstNode (&This->ChildBridgeListHead);

  while (!IsNull (&This->ChildBridgeListHead, Link)) {
    Bridge = PEI_P2P_BRIDGE_FROM_LINK (Link);
    EnableBridgeDevices (Bridge);
    Link = GetNextNode (&This->ChildBridgeListHead, Link);
  }

  if (This->Parent != NULL) {
    Attributes = EFI_PCI_DEVICE_ENABLE;
    This->Device->PciDevice.PciIo.Attributes (
                                    &This->Device->PciDevice.PciIo,
                                    EfiPciIoAttributeOperationEnable,
                                    Attributes,
                                    NULL
                                    );
  }
}

/**
  This function releases memory previously allocated for resource tree.

  @param[in] This  Pointer to bridge structure.
**/
VOID
FreeResourceTree (
  IN P2P_BRIDGE  *This
  )
{
  LIST_ENTRY         *Link;
  P2P_BRIDGE         *Bridge;
  PEI_RESOURCE_NODE  *Resource;

  Link = GetFirstNode (&This->ChildBridgeListHead);

  while (!IsNull (&This->ChildBridgeListHead, Link)) {
    Bridge = PEI_P2P_BRIDGE_FROM_LINK (Link);
    FreeResourceTree (Bridge);
    Link = GetNextNode (&This->ChildBridgeListHead, Link);
  }

  Link = GetFirstNode (&This->ResourceListHead);

  while (!IsNull (&This->ResourceListHead, Link)) {
    Resource = PEI_RESOURCE_NODE_FROM_LINK (Link);
    Link = GetNextNode (&This->ResourceListHead, Link);
    FreePool (Resource);
  }
  if (This->Parent != NULL) {
    FreePool (This);
  }
}

/**
  This function starts device recognition and resource allocation process under particular root bridge.

  @param[in] PciRootBridge  Root bridge instance.
  @param[in] Index          Root bridge index.
**/
VOID
PcieRootBridgeEnumerateEssentialDevices (
  IN PCI_ROOT_BRIDGE  *PciRootBridge,
  IN UINTN            Index
  )
{
  EFI_STATUS    Status;
  UINT32        MemoryBase;
  UINT16        IoBase;
  SBDF          Sbdf;
  SBDF_TABLE    BridgeCleanupList;
  UINT8         BusBase;
  UINT8         Dev;
  UINT8         Func;
  P2P_BRIDGE    Root;

  DEBUG ((DEBUG_INFO, "Enumerating PCI BUS %d\n", Index));
  DEBUG ((DEBUG_INFO, "Segment %X\n", PciRootBridge->Segment));
  DEBUG ((DEBUG_INFO, "Bus %X - ", PciRootBridge->Bus.Base));
  DEBUG ((DEBUG_INFO, "%X\n", PciRootBridge->Bus.Limit));
  DEBUG ((DEBUG_INFO, "MEM32 %X - ", PciRootBridge->Mem.Base));
  DEBUG ((DEBUG_INFO, "%X\n", PciRootBridge->Mem.Limit));
  DEBUG ((DEBUG_INFO, "MEM64 %X - ", PciRootBridge->MemAbove4G.Base));
  DEBUG ((DEBUG_INFO, "%X\n", PciRootBridge->MemAbove4G.Limit));
  DEBUG ((DEBUG_INFO, "IO %X - ", PciRootBridge->Io.Base));
  DEBUG ((DEBUG_INFO, "%X\n", PciRootBridge->Io.Limit));

  BridgeCleanupList.Count = 0;
  MemoryBase = (UINT32) PciRootBridge->Mem.Base;
  IoBase = (UINT16) PciRootBridge->Io.Base;
  Sbdf.Seg = PciRootBridge->Segment;
  Sbdf.Bus = (UINT32) PciRootBridge->Bus.Base;
  BusBase = (UINT8) (PciRootBridge->Bus.Base + 1);

  //
  // Step 1. Recursively assign bus numbers to bridges under this root bridge
  //
  for (Dev = 0; Dev <= PCI_MAX_DEVICE; Dev++) {
    Sbdf.Dev = Dev;
    for (Func = 0; Func <= PCI_MAX_FUNC; Func++) {
      Sbdf.Func = Func;
      Sbdf.PcieCap = PcieBaseFindCapId (SbdfToBase (Sbdf), EFI_PCI_CAPABILITY_ID_PCIEXP);
      if (PciSegmentRead16 (SbdfToBase (Sbdf)) == 0xFFFF) {
        if (Func == 0) {
          break;
        } else {
          continue;
        }
      } else {
        if (IsBridgeDevice (Sbdf)) {
          BusBase = RecursiveBusAssignment (Sbdf, BusBase, (UINT8) PciRootBridge->Bus.Limit, &BridgeCleanupList);
          BusBase++;
        }
      }
    }
  }

  //
  // Step 2. Build resource tree under this root bridge
  //
  ZeroMem (&Root, sizeof(P2P_BRIDGE));
  Root.Signature = PEI_P2P_BRIDGE_SIGNATURE;
  Root.SecBus = (UINT8) PciRootBridge->Bus.Base;
  Root.Device = AllocateZeroPool (sizeof (PCI_DEVICE_PRIVATE_DATA));
  if (Root.Device == NULL) {
    DEBUG ((DEBUG_ERROR, "%a - out of memory for PCI_DEVICE_PRIVATE_DATA\n", __FUNCTION__));
    ASSERT (FALSE);
  }
  Root.Device->Sbdf.Seg = PciRootBridge->Segment;
  Root.Device->Sbdf.Bus = (UINT32) PciRootBridge->Bus.Base;
  Root.Device->Sbdf.Dev = 0;
  Root.Device->Sbdf.Func = 0;
  Root.Device->PciCfgBase = SbdfToBase (Root.Device->Sbdf);
  Root.Device->Signature = PCI_DEVICE_PRIVATE_DATA_SIGNATURE;
  Root.Device->Supports = PciRootBridge->Supports;
  PciGetPciIoAccess (Root.Device);
  InitializeListHead (&Root.ChildBridgeListHead);
  InitializeListHead (&Root.ResourceListHead);
  InitializeListHead (&Root.EndpointListHead);
  EnumerateBridgeResources (&Root);
  Status = AlignResourceTree (&Root);
  ASSERT_EFI_ERROR (Status);

  //
  // Step 3. Apply proposed resources for essential devices
  //
  InitResources (&Root, (UINT32) PciRootBridge->Mem.Limit, (UINT16) PciRootBridge->Io.Limit);
  Status = ApplyMemResources (&Root, MemoryBase, (UINT32) PciRootBridge->Mem.Limit);
  ASSERT_EFI_ERROR (Status);

  Status = ApplyIoResources (&Root, IoBase, (UINT16) PciRootBridge->Io.Limit);
  ASSERT_EFI_ERROR (Status);

  EnableBridgeDevices (&Root);

  //
  // Step 4. Prepare PciIo PPI instances
  //
  Status = PreparePciIoAccess (&Root, PciRootBridge->DevicePath);
  ASSERT_EFI_ERROR (Status);

  //
  // Step 5. Free allocated memory
  //
  FreeResourceTree (&Root);
}
