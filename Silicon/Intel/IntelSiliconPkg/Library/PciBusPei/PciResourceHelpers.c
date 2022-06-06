/** @file
  Helper routines for handling PEI PCIe resource allocation

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>
#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/PciSegmentLib.h>
#include <Library/PcieHelperLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/DevicePath.h>
#include <Library/DevicePathLib.h>
#include <IndustryStandard/Pci.h>

#include "PcieBusPei.h"

/**
  This function returns whether given device is a P2P bridge device.

  @param[in] Sbdf  Device's segment:bus:device:function coordinates
**/
BOOLEAN
IsBridgeDevice (
  IN SBDF  Sbdf
  )
{
  if (GetDeviceType (Sbdf) == DevTypePcieUpstream || GetDeviceType (Sbdf) == DevTypePcieDownstream) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  This function returns whether given device should have assigned resources.

  @param[in] Sbdf  Device's segment:bus:device:function coordinates
**/
BOOLEAN
PciIsDeviceEssential (
  IN SBDF  Sbdf
  )
{
  UINT8  Baseclass;
  UINT8  Subclass;

  Baseclass = PciSegmentRead8 (SbdfToBase (Sbdf) + R_PCI_BCC_OFFSET);
  Subclass = PciSegmentRead8 (SbdfToBase (Sbdf) + R_PCI_SCC_OFFSET);

  if ((Baseclass == PCI_CLASS_MASS_STORAGE ||
     (Baseclass == PCI_CLASS_SERIAL && Subclass == PCI_CLASS_SERIAL_USB) ||
     (Baseclass == PCI_CLASS_SYSTEM_PERIPHERAL && Subclass == PCI_SUBCLASS_SD_HOST_CONTROLLER))) {
    return TRUE;
  }
  return FALSE;
}

/**
  This function returns whether given device is decoding its resources.

  @param[in] Sbdf  Device's segment:bus:device:function coordinates
**/
BOOLEAN
IsDeviceDecodingResources (
  IN SBDF  Sbdf
  )
{
  if (PciSegmentRead16 (SbdfToBase (Sbdf) + PCI_COMMAND_OFFSET) & (EFI_PCI_COMMAND_MEMORY_SPACE | EFI_PCI_COMMAND_IO_SPACE)) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  This function removes resource nodes from the tree based on device's address.

  @param[in] Bridge Pointer to bridge structure.
  @param[in] Sbdf   Device's segment:bus:device:function coordinates
**/
VOID
RemoveResourceNodesBySbdf (
  IN P2P_BRIDGE  *Bridge,
  IN SBDF        Sbdf
  )
{
  LIST_ENTRY         *Link;
  PEI_RESOURCE_NODE  *Resource;

  Link = GetFirstNode (&Bridge->ResourceListHead);
  while (!IsNull (&Bridge->ResourceListHead, Link)) {
    Resource = PEI_RESOURCE_NODE_FROM_LINK (Link);
    if (!CompareMem (&Sbdf, &Resource->Device->Sbdf, sizeof (SBDF))) {
      Link = RemoveEntryList (Link);
      FreePool (Resource);
      continue;
    }
    Link = GetNextNode (&Bridge->ResourceListHead, Link);
  }
}

/**
  This function sorts resource list under particular bridge in a descending order.

  @param[in] Bridge  Pointer to current bridge structure.
**/
VOID
BridgeSortResourceList (
  IN P2P_BRIDGE  *Bridge
  )
{
  BOOLEAN            Swapped;
  LIST_ENTRY         *Link;
  LIST_ENTRY         *NextLink;
  PEI_RESOURCE_NODE  *Resource1;
  PEI_RESOURCE_NODE  *Resource2;

  do {
    Swapped = FALSE;
    Link = GetFirstNode (&Bridge->ResourceListHead);
    NextLink = GetNextNode (&Bridge->ResourceListHead, Link);
    while (!IsNull (&Bridge->ResourceListHead, NextLink) && !IsNull (&Bridge->ResourceListHead, Link)) {
      Resource1 = PEI_RESOURCE_NODE_FROM_LINK (Link);
      Resource2 = PEI_RESOURCE_NODE_FROM_LINK (NextLink);
      if (Resource2->Length > Resource1->Length) {
        SwapListEntries (Link, NextLink);
        Swapped = TRUE;
      }
      Link = NextLink;
      NextLink = GetNextNode (&Bridge->ResourceListHead, Link);
    }
  } while (Swapped);
}

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
  )
{
  LIST_ENTRY         *Link;
  PEI_RESOURCE_NODE  *Resource;

  if (Bridge == NULL) {
    return NULL;
  }

  Link = GetFirstNode (&Bridge->ResourceListHead);

  while (!IsNull (&Bridge->ResourceListHead, Link)) {
    Resource = PEI_RESOURCE_NODE_FROM_LINK (Link);
    if (Resource->ResType & ResourceType) {
      return Resource;
    }
    Link = GetNextNode (&Bridge->ResourceListHead, Link);
  }

  return NULL;
}

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
  )
{
  LIST_ENTRY         *Link;
  PEI_RESOURCE_NODE  *Resource;

  if (Bridge == NULL || Node == NULL) {
    return NULL;
  }

  Link = GetNextNode (&Bridge->ResourceListHead, &Node->Link);
  while (!IsNull (&Bridge->ResourceListHead, Link)) {
    Resource = PEI_RESOURCE_NODE_FROM_LINK (Link);
    if (Resource->ResType & ResourceType) {
      return Resource;
    }
    Link = GetNextNode (&Bridge->ResourceListHead, Link);
  }
  return NULL;
}

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
  )
{
  PEI_RESOURCE_NODE  *Resource;
  PEI_RESOURCE_NODE  *NextResource;

  if (Bridge == NULL) {
    return NULL;
  }

  Resource = BridgeGetFirstResourceNode (Bridge, ResourceType);
  NextResource = BridgeGetNextResourceNode (Bridge, Resource, ResourceType);

  while (Resource != NULL && NextResource != NULL) {
    Resource = NextResource;
    NextResource = BridgeGetNextResourceNode (Bridge, Resource, ResourceType);
  }
  return Resource;
}
