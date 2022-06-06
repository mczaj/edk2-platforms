/** @file
  Header file for PciBusPei module

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef _PCIE_BUS_PEI_H_
#define _PCIE_BUS_PEI_H_

#include <Base.h>
#include <Library/PciHostBridgeLib.h>
#include <Ppi/PciDevicePpi.h>
#include <Protocol/DevicePath.h>

typedef enum {
  PciBarTypeUnknown = 0,
  PciBarTypeIo16,
  PciBarTypeIo32,
  PciBarTypeMem32,
  PciBarTypePMem32,
  PciBarTypeMem64,
  PciBarTypePMem64,
  PciBarTypeOpRom,
  PciBarTypeIo,
  PciBarTypeMem,
  PciBarTypeMaxType
} PCI_BAR_TYPE;

typedef struct _PCI_DEVICE_PRIVATE_DATA PCI_DEVICE_PRIVATE_DATA;

#define PCI_DEVICE_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('P', 'D', 'P', 'D')

struct _PCI_DEVICE_PRIVATE_DATA {
  PCI_DEVICE_PPI           PciDevice;
  UINT64                   PciCfgBase;
  UINT32                   Signature;
  LIST_ENTRY               Link;
  SBDF                     Sbdf;
  UINT64                   Attributes;
  UINT64                   Supports;
  PCI_DEVICE_PRIVATE_DATA  *Parent;
};

#define PCI_DEVICE_PRIVATE_DATA_FROM_LINK(a) \
  CR (a, PCI_DEVICE_PRIVATE_DATA, Link, PCI_DEVICE_PRIVATE_DATA_SIGNATURE)

typedef enum {
  IoResource  = BIT0,
  MemResource = BIT1,
  IoAperture  = BIT2,
  MemAperture = BIT3
} PEI_RESOURCE_TYPE;

#define PEI_P2P_BRIDGE_SIGNATURE  SIGNATURE_32 ('P', 'P', 'B', 'R')

#define PEI_PCI_RESOURCE_SIGNATURE  SIGNATURE_32 ('P', 'R', 'E', 'S')
typedef struct {
  UINT32                   Signature;
  LIST_ENTRY               Link;
  UINT8                    Bar;
  UINT32                   Length;
  UINT32                   Offset;
  UINT32                   Alignment;
  PCI_DEVICE_PRIVATE_DATA  *Device;
  PEI_RESOURCE_TYPE        ResType;
} PEI_RESOURCE_NODE;

#define PEI_RESOURCE_NODE_FROM_LINK(a) \
  CR (a, PEI_RESOURCE_NODE, Link, PEI_PCI_RESOURCE_SIGNATURE)

typedef struct _P2P_BRIDGE P2P_BRIDGE;

struct _P2P_BRIDGE {
  UINT32                   Signature;
  LIST_ENTRY               Link;
  UINT8                    SecBus;
  P2P_BRIDGE               *Parent;
  PCI_DEVICE_PRIVATE_DATA  *Device;
  LIST_ENTRY               ChildBridgeListHead;
  LIST_ENTRY               ResourceListHead;
  LIST_ENTRY               EndpointListHead;
};

#define PEI_P2P_BRIDGE_FROM_LINK(a) \
  CR (a, P2P_BRIDGE, Link, PEI_P2P_BRIDGE_SIGNATURE)

/**
  Prepare function pointers for the PCI_DEVICE_PPI.

  @param[in] PciDevPriv  Instance of PCI_DEVICE_PRIVATE_DATA.
**/
VOID
PciGetPciIoAccess (
  IN PCI_DEVICE_PRIVATE_DATA  *PciDevPriv
  );

/**
  This function starts device recognition and resource allocation process under particular root bridge.

  @param[in] PciRootBridge  Root bridge instance.
  @param[in] Index          Root bridge index.
**/
VOID
PcieRootBridgeEnumerateEssentialDevices (
  IN PCI_ROOT_BRIDGE  *PciRootBridge,
  IN UINTN            Index
  );

/**
  This function returns whether given device is a P2P bridge device.

  @param[in] Sbdf  Device's segment:bus:device:function coordinates
**/
BOOLEAN
IsBridgeDevice (
  IN SBDF  Sbdf
  );

/**
  This function returns whether given device should have assigned resources.

  @param[in] Sbdf  Device's segment:bus:device:function coordinates
**/
BOOLEAN
PciIsDeviceEssential (
  IN SBDF  Sbdf
  );

/**
  This function returns whether given device is decoding its resources.

  @param[in] Sbdf  Device's segment:bus:device:function coordinates
**/
BOOLEAN
IsDeviceDecodingResources (
  IN SBDF  Sbdf
  );

/**
  This function removes resource nodes from the tree based on device's address.

  @param[in] Bridge Pointer to bridge structure.
  @param[in] Sbdf   Device's segment:bus:device:function coordinates
**/
VOID
RemoveResourceNodesBySbdf (
  IN P2P_BRIDGE  *Bridge,
  IN SBDF        Sbdf
  );

/**
  This function sorts resource list under particular bridge in a descending order.

  @param[in] Bridge  Pointer to current bridge structure.
**/
VOID
BridgeSortResourceList (
  IN P2P_BRIDGE  *Bridge
  );

/**
  This function returns first resource node under particular bridge.

  @param[in] Bridge        Pointer to current bridge structure.
  @param[in] ResourceType  Type of resource node to be returned.

  @retval Pointer to resource node structure.
**/
PEI_RESOURCE_NODE *
BridgeGetFirstResourceNode (
  IN P2P_BRIDGE         *Bridge,
  IN PEI_RESOURCE_TYPE  ResourceType
  );

/**
  This function returns next resource node.

  @param[in] Bridge        Pointer to current bridge structure.
  @param[in] Node          Pointer to the resource node.
  @param[in] ResourceType  Type of resource node to be returned.

  @retval Pointer to next resource node structure.
**/
PEI_RESOURCE_NODE *
BridgeGetNextResourceNode (
  IN P2P_BRIDGE         *Bridge,
  IN PEI_RESOURCE_NODE  *Node,
  IN PEI_RESOURCE_TYPE  ResourceType
  );

/**
  This function returns last resource node.

  @param[in] Bridge        Pointer to current bridge structure.
  @param[in] ResourceType  Type of resource node to be returned.

  @retval Pointer to last resource node structure.
**/
PEI_RESOURCE_NODE *
BridgeGetLastResourceNode (
  IN P2P_BRIDGE         *Bridge,
  IN PEI_RESOURCE_TYPE  ResourceType
  );

#endif
