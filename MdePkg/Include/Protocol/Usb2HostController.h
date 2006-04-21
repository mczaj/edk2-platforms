/** @file
  EFI_USB2_HC_PROTOCOL as defined in UEFI 2.0.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  Usb2HostController.h

**/

#ifndef _USB2_HOSTCONTROLLER_H_
#define _USB2_HOSTCONTROLLER_H_

#define EFI_USB2_HC_PROTOCOL_GUID \
  { \
    0x3e745226, 0x9818, 0x45b6, {0xa2, 0xac, 0xd7, 0xcd, 0xe, 0x8b, 0xa2, 0xbc } \
  }

//
// Forward reference for pure ANSI compatability
//
typedef struct _EFI_USB2_HC_PROTOCOL EFI_USB2_HC_PROTOCOL;

#define EFI_USB_SPEED_FULL      0x0000  // 12 Mb/s, USB 1.1 OHCI and UHCI HC.
#define EFI_USB_SPEED_LOW       0x0001  // 1 Mb/s, USB 1.1 OHCI and UHCI HC.
#define EFI_USB_SPEED_HIGH      0x0002  // 480 Mb/s, USB 2.0 EHCI HC.

typedef struct {
  UINT8      TranslatorHubAddress;
  UINT8      TranslatorPortNumber;
} EFI_USB2_HC_TRANSACTION_TRANSLATOR;

//
// Protocol definitions
//

/**
  Retrieves the Host Controller capabilities.

  @param  This           A pointer to the EFI_USB2_HC_PROTOCOL instance.
  @param  MaxSpeed       Host controller data transfer speed.
  @param  PortNumber     Number of the root hub ports.
  @param  Is64BitCapable TRUE if controller supports 64-bit memory addressing,
                         FALSE otherwise.

  @retval EFI_SUCCESS           The host controller capabilities were retrieved successfully.
  @retval EFI_INVALID_PARAMETER One of the input args was NULL.
  @retval EFI_DEVICE_ERROR      An error was encountered while attempting to
                                retrieve the capabilities.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_USB2_HC_PROTOCOL_GET_CAPABILITY) (
  IN  EFI_USB2_HC_PROTOCOL  *This,
  OUT UINT8                 *MaxSpeed,
  OUT UINT8                 *PortNumber,
  OUT UINT8                 *Is64BitCapable
  )
;

/**
  Provides software reset for the USB host controller.

  @param  This       A pointer to the EFI_USB2_HC_PROTOCOL instance.
  @param  Attributes A bit mask of the reset operation to perform.

  @retval EFI_SUCCESS           The reset operation succeeded.
  @retval EFI_INVALID_PARAMETER Attributes is not valid.
  @retval EFI_UNSUPPORTED       The type of reset specified by Attributes is not currently
                                supported by the host controller hardware.
  @retval EFI_ACCESS_DENIED     Reset operation is rejected due to the debug port being configured
                                and active; only EFI_USB_HC_RESET_GLOBAL_WITH_DEBUG or
                                EFI_USB_HC_RESET_HOST_WITH_DEBUG reset Attributes can be used to
                                perform reset operation for this host controller.
  @retval EFI_DEVICE_ERROR      An error was encountered while attempting to
                                retrieve the capabilities.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_USB2_HC_PROTOCOL_RESET) (
  IN EFI_USB2_HC_PROTOCOL   *This,
  IN UINT16                 Attributes
  )
;

/**
  Retrieves current state of the USB host controller.

  @param  This  A pointer to the EFI_USB2_HC_PROTOCOL instance.
  @param  State A pointer to the EFI_USB_HC_STATE data structure that
                indicates current state of the USB host controller.

  @retval EFI_SUCCESS           The state information of the host controller was returned in State.
  @retval EFI_INVALID_PARAMETER State is NULL.
  @retval EFI_DEVICE_ERROR      An error was encountered while attempting to retrieve the
                                host controller��s current state.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_USB2_HC_PROTOCOL_GET_STATE) (
  IN  EFI_USB2_HC_PROTOCOL    *This,
  OUT EFI_USB_HC_STATE        *State
  )
;  

/**
  Sets the USB host controller to a specific state.

  @param  This  A pointer to the EFI_USB2_HC_PROTOCOL instance.
  @param  State Indicates the state of the host controller that will be set.

  @retval EFI_SUCCESS           The USB host controller was successfully placed in the state
                                specified by State.
  @retval EFI_INVALID_PARAMETER State is not valid.
  @retval EFI_DEVICE_ERROR      Failed to set the state specified by State due to device error.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_USB2_HC_PROTOCOL_SET_STATE) (
  IN EFI_USB2_HC_PROTOCOL    *This,
  IN EFI_USB_HC_STATE        State
  )
;

/**
  Submits control transfer to a target USB device.

  @param  This                A pointer to the EFI_USB2_HC_PROTOCOL instance.
  @param  DeviceAddress       Represents the address of the target device on the USB.
  @param  DeviceSpeed         Indicates device speed.
  @param  MaximumPacketLength Indicates the maximum packet size that the default control transfer
                              endpoint is capable of sending or receiving.
  @param  Request             A pointer to the USB device request that will be sent to the USB device.
  @param  TransferDirection   Specifies the data direction for the transfer. There are three values
                              available, EfiUsbDataIn, EfiUsbDataOut and EfiUsbNoData.
  @param  Data                A pointer to the buffer of data that will be transmitted to USB device or
                              received from USB device.
  @param  DataLength          On input, indicates the size, in bytes, of the data buffer specified by Data.
                              On output, indicates the amount of data actually transferred.
  @param  Translator          A pointer to the transaction translator data.
  @param  TimeOut             Indicates the maximum time, in milliseconds, which the transfer is
                              allowed to complete.
  @param  TransferResult      A pointer to the detailed result information generated by this control
                              transfer.

  @retval EFI_SUCCESS           The control transfer was completed successfully.
  @retval EFI_INVALID_PARAMETER Some parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The control transfer could not be completed due to a lack of resources.
  @retval EFI_TIMEOUT           The control transfer failed due to timeout.
  @retval EFI_DEVICE_ERROR      The control transfer failed due to host controller or device error.
                                Caller should check TransferResult for detailed error information.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_USB2_HC_PROTOCOL_CONTROL_TRANSFER) (
  IN     EFI_USB2_HC_PROTOCOL               *This,
  IN     UINT8                              DeviceAddress,
  IN     UINT8                              DeviceSpeed,
  IN     UINTN                              MaximumPacketLength,
  IN     EFI_USB_DEVICE_REQUEST             *Request,
  IN     EFI_USB_DATA_DIRECTION             TransferDirection,
  IN OUT VOID                               *Data       OPTIONAL,
  IN OUT UINTN                              *DataLength OPTIONAL,
  IN     UINTN                              TimeOut,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR *Translator,
  OUT    UINT32                             *TransferResult
  )
;  

#define EFI_USB_MAX_BULK_BUFFER_NUM 10

/**
  Submits bulk transfer to a bulk endpoint of a USB device.

  @param  This                A pointer to the EFI_USB2_HC_PROTOCOL instance.
  @param  DeviceAddress       Represents the address of the target device on the USB.
  @param  EndPointAddress     The combination of an endpoint number and an endpoint direction of the
                              target USB device.
  @param  DeviceSpeed         Indicates device speed.
  @param  MaximumPacketLength Indicates the maximum packet size the target endpoint is capable of
                              sending or receiving.
  @param  DataBuffersNumber   Number of data buffers prepared for the transfer.
  @param  Data                Array of pointers to the buffers of data that will be transmitted to USB
                              device or received from USB device.
  @param  DataLength          When input, indicates the size, in bytes, of the data buffers specified by
                              Data. When output, indicates the actually transferred data size.
  @param  DataToggle          A pointer to the data toggle value.
  @param  Translator          A pointer to the transaction translator data.
  @param  TimeOut             Indicates the maximum time, in milliseconds, which the transfer is
                              allowed to complete.
  @param  TransferResult      A pointer to the detailed result information of the bulk transfer.

  @retval EFI_SUCCESS           The bulk transfer was completed successfully.
  @retval EFI_INVALID_PARAMETER Some parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The bulk transfer could not be submitted due to a lack of resources.
  @retval EFI_TIMEOUT           The bulk transfer failed due to timeout.
  @retval EFI_DEVICE_ERROR      The bulk transfer failed due to host controller or device error.
                                Caller should check TransferResult for detailed error information.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_USB2_HC_PROTOCOL_BULK_TRANSFER) (
  IN     EFI_USB2_HC_PROTOCOL               *This,
  IN     UINT8                              DeviceAddress,
  IN     UINT8                              EndPointAddress,
  IN     UINT8                              DeviceSpeed,
  IN     UINTN                              MaximumPacketLength,
  IN     UINT8                              DataBuffersNumber,
  IN OUT VOID                               *Data[EFI_USB_MAX_BULK_BUFFER_NUM],
  IN OUT UINTN                              *DataLength,
  IN OUT UINT8                              *DataToggle,
  IN     UINTN                              TimeOut,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR *Translator,
  OUT    UINT32                             *TransferResult
  )
;

/**
  Submits an asynchronous interrupt transfer to an interrupt endpoint of a USB device.

  @param  This                A pointer to the EFI_USB2_HC_PROTOCOL instance.
  @param  DeviceAddress       Represents the address of the target device on the USB.
  @param  EndPointAddress     The combination of an endpoint number and an endpoint direction of the
                              target USB device.
  @param  DeviceSpeed         Indicates device speed.
  @param  MaximumPacketLength Indicates the maximum packet size the target endpoint is capable of
                              sending or receiving.
  @param  IsNewTransfer       If TRUE, an asynchronous interrupt pipe is built between the host and the
                              target interrupt endpoint. If FALSE, the specified asynchronous interrupt
                              pipe is canceled. If TRUE, and an interrupt transfer exists for the target
                              end point, then EFI_INVALID_PARAMETER is returned.
  @param  DataToggle          A pointer to the data toggle value.
  @param  PollingInterval     Indicates the interval, in milliseconds, that the asynchronous interrupt
                              transfer is polled.
  @param  DataLength          Indicates the length of data to be received at the rate specified by
                              PollingInterval from the target asynchronous interrupt endpoint.
  @param  CallBackFunction    The Callback function. This function is called at the rate specified by
                              PollingInterval.
  @param  Context             The context that is passed to the CallBackFunction. This is an
                              optional parameter and may be NULL.

  @retval EFI_SUCCESS           The asynchronous interrupt transfer request has been successfully
                                submitted or canceled.
  @retval EFI_INVALID_PARAMETER Some parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_USB2_HC_PROTOCOL_ASYNC_INTERRUPT_TRANSFER) (
  IN     EFI_USB2_HC_PROTOCOL                                *This,
  IN     UINT8                                               DeviceAddress,
  IN     UINT8                                               EndPointAddress,
  IN     UINT8                                               DeviceSpeed,
  IN     UINTN                                               MaxiumPacketLength,
  IN     BOOLEAN                                             IsNewTransfer,
  IN OUT UINT8                                               *DataToggle,
  IN     UINTN                                               PollingInterval  OPTIONAL,
  IN     UINTN                                               DataLength       OPTIONAL,
  IN     EFI_ASYNC_USB_TRANSFER_CALLBACK                     CallBackFunction OPTIONAL,
  IN     VOID                                                *Context         OPTIONAL
  )
;

/**
  Submits synchronous interrupt transfer to an interrupt endpoint of a USB device.

  @param  This                  A pointer to the EFI_USB2_HC_PROTOCOL instance.
  @param  DeviceAddress         Represents the address of the target device on the USB.
  @param  EndPointAddress       The combination of an endpoint number and an endpoint direction of the
                                target USB device.
  @param  DeviceSpeed           Indicates device speed.
  @param  MaximumPacketLength   Indicates the maximum packet size the target endpoint is capable of
                                sending or receiving.
  @param  Data                  A pointer to the buffer of data that will be transmitted to USB device or
                                received from USB device.
  @param  DataLength            On input, the size, in bytes, of the data buffer specified by Data. On
                                output, the number of bytes transferred.
  @param  DataToggle            A pointer to the data toggle value.
  @param  TimeOut               Indicates the maximum time, in milliseconds, which the transfer is
                                allowed to complete.
  @param  TransferResult        A pointer to the detailed result information from the synchronous
                                interrupt transfer.

  @retval EFI_SUCCESS           The synchronous interrupt transfer was completed successfully.
  @retval EFI_INVALID_PARAMETER Some parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The synchronous interrupt transfer could not be submitted due to a lack of resources.
  @retval EFI_TIMEOUT           The synchronous interrupt transfer failed due to timeout.
  @retval EFI_DEVICE_ERROR      The synchronous interrupt transfer failed due to host controller or device error.
                                Caller should check TransferResult for detailed error information.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_USB2_HC_PROTOCOL_SYNC_INTERRUPT_TRANSFER) (
  IN     EFI_USB2_HC_PROTOCOL   *This,
  IN     UINT8                  DeviceAddress,
  IN     UINT8                  EndPointAddress,
  IN     UINT8                  DeviceSpeed,
  IN     UINTN                  MaximumPacketLength,
  IN OUT VOID                   *Data,
  IN OUT UINTN                  *DataLength,
  IN OUT UINT8                  *DataToggle,
  IN     UINTN                  TimeOut,
  OUT    UINT32                 *TransferResult
  )
;

#define EFI_USB_MAX_ISO_BUFFER_NUM  7
#define EFI_USB_MAX_ISO_BUFFER_NUM1 2

/**
  Submits isochronous transfer to an isochronous endpoint of a USB device.

  @param  This                  A pointer to the EFI_USB2_HC_PROTOCOL instance.
  @param  DeviceAddress         Represents the address of the target device on the USB.
  @param  EndPointAddress       The combination of an endpoint number and an endpoint direction of the
                                target USB device.
  @param  DeviceSpeed           Indicates device speed.
  @param  MaximumPacketLength   Indicates the maximum packet size the target endpoint is capable of
                                sending or receiving.
  @param  DataBuffersNumber     Number of data buffers prepared for the transfer.
  @param  Data                  Array of pointers to the buffers of data that will be transmitted to USB
                                device or received from USB device.
  @param  DataLength            Specifies the length, in bytes, of the data to be sent to or received from
                                the USB device.
  @param  Translator            A pointer to the transaction translator data.
  @param  TransferResult        A pointer to the detailed result information of the isochronous transfer.

  @retval EFI_SUCCESS           The isochronous transfer was completed successfully.
  @retval EFI_INVALID_PARAMETER Some parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The isochronous transfer could not be submitted due to a lack of resources.
  @retval EFI_TIMEOUT           The isochronous transfer cannot be completed within the one USB frame time.
  @retval EFI_DEVICE_ERROR      The isochronous transfer failed due to host controller or device error.
                                Caller should check TransferResult for detailed error information.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_USB2_HC_PROTOCOL_ISOCHRONOUS_TRANSFER) (
  IN     EFI_USB2_HC_PROTOCOL               *This,
  IN     UINT8                              DeviceAddress,
  IN     UINT8                              EndPointAddress,
  IN     UINT8                              DeviceSpeed,
  IN     UINTN                              MaximumPacketLength,
  IN     UINT8                              DataBuffersNumber,
  IN OUT VOID                               *Data[EFI_USB_MAX_ISO_BUFFER_NUM],
  IN     UINTN                              DataLength,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR *Translator,
  OUT    UINT32                             *TransferResult
  )
;

/**
  Submits nonblocking isochronous transfer to an isochronous endpoint of a USB device.

  @param  This                  A pointer to the EFI_USB2_HC_PROTOCOL instance.
  @param  DeviceAddress         Represents the address of the target device on the USB.
  @param  EndPointAddress       The combination of an endpoint number and an endpoint direction of the
                                target USB device.
  @param  DeviceSpeed           Indicates device speed.
  @param  MaximumPacketLength   Indicates the maximum packet size the target endpoint is capable of
                                sending or receiving.
  @param  DataBuffersNumber     Number of data buffers prepared for the transfer.
  @param  Data                  Array of pointers to the buffers of data that will be transmitted to USB
                                device or received from USB device.
  @param  DataLength            Specifies the length, in bytes, of the data to be sent to or received from
                                the USB device.
  @param  Translator            A pointer to the transaction translator data.
  @param  IsochronousCallback   The Callback function. This function is called if the requested
                                isochronous transfer is completed.
  @param  Context               Data passed to the IsochronousCallback function. This is an
                                optional parameter and may be NULL.

  @retval EFI_SUCCESS           The asynchronous isochronous transfer request has been successfully
                                submitted or canceled.
  @retval EFI_INVALID_PARAMETER Some parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The asynchronous isochronous transfer could not be submitted due to
                                a lack of resources.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_USB2_HC_PROTOCOL_ASYNC_ISOCHRONOUS_TRANSFER) (
  IN     EFI_USB2_HC_PROTOCOL               *This,
  IN     UINT8                              DeviceAddress,
  IN     UINT8                              EndPointAddress,
  IN     UINT8                              DeviceSpeed,
  IN     UINTN                              MaximumPacketLength,
  IN     UINT8                              DataBuffersNumber,
  IN OUT VOID                               *Data[EFI_USB_MAX_ISO_BUFFER_NUM],
  IN     UINTN                              DataLength,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR *Translator,
  IN     EFI_ASYNC_USB_TRANSFER_CALLBACK    IsochronousCallBack,
  IN     VOID                               *Context OPTIONAL
  )
;

/**
  Retrieves the current status of a USB root hub port.

  @param  This       A pointer to the EFI_USB2_HC_PROTOCOL instance.
  @param  PortNumber Specifies the root hub port from which the status is to be retrieved.
                     This value is zero based.
  @param  PortStatus A pointer to the current port status bits and port status change bits.

  @retval EFI_SUCCESS           The status of the USB root hub port specified by PortNumber
                                was returned in PortStatus.
  @retval EFI_INVALID_PARAMETER PortNumber is invalid.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_USB2_HC_PROTOCOL_GET_ROOTHUB_PORT_STATUS) (
  IN EFI_USB2_HC_PROTOCOL    *This,
  IN  UINT8                  PortNumber,
  OUT EFI_USB_PORT_STATUS    *PortStatus
  )
;  

/**
  Sets a feature for the specified root hub port.

  @param  This        A pointer to the EFI_USB2_HC_PROTOCOL instance.
  @param  PortNumber  Specifies the root hub port whose feature is requested to be set. This
                      value is zero based.
  @param  PortFeature Indicates the feature selector associated with the feature set request.

  @retval EFI_SUCCESS           The feature specified by PortFeature was set for the USB
                                root hub port specified by PortNumber.
  @retval EFI_INVALID_PARAMETER PortNumber is invalid or PortFeature is invalid for this function.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_USB2_HC_PROTOCOL_SET_ROOTHUB_PORT_FEATURE) (
  IN EFI_USB2_HC_PROTOCOL    *This,
  IN UINT8                   PortNumber,
  IN EFI_USB_PORT_FEATURE    PortFeature
  )
;  

/**
  Clears a feature for the specified root hub port.

  @param  This        A pointer to the EFI_USB2_HC_PROTOCOL instance.
  @param  PortNumber  Specifies the root hub port whose feature is requested to be cleared. This
                      value is zero based.
  @param  PortFeature Indicates the feature selector associated with the feature clear request.

  @retval EFI_SUCCESS           The feature specified by PortFeature was cleared for the USB
                                root hub port specified by PortNumber.
  @retval EFI_INVALID_PARAMETER PortNumber is invalid or PortFeature is invalid for this function.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_USB2_HC_PROTOCOL_CLEAR_ROOTHUB_PORT_FEATURE) (
  IN EFI_USB2_HC_PROTOCOL    *This,
  IN UINT8                   PortNumber,
  IN EFI_USB_PORT_FEATURE    PortFeature
  )
;  

struct _EFI_USB2_HC_PROTOCOL {
  EFI_USB2_HC_PROTOCOL_GET_CAPABILITY              GetCapability;
  EFI_USB2_HC_PROTOCOL_RESET                       Reset;
  EFI_USB2_HC_PROTOCOL_GET_STATE                   GetState;
  EFI_USB2_HC_PROTOCOL_SET_STATE                   SetState;
  EFI_USB2_HC_PROTOCOL_CONTROL_TRANSFER            ControlTransfer;
  EFI_USB2_HC_PROTOCOL_BULK_TRANSFER               BulkTransfer;
  EFI_USB2_HC_PROTOCOL_ASYNC_INTERRUPT_TRANSFER    AsyncInterruptTransfer;
  EFI_USB2_HC_PROTOCOL_SYNC_INTERRUPT_TRANSFER     SyncInterruptTransfer;
  EFI_USB2_HC_PROTOCOL_ISOCHRONOUS_TRANSFER        IsochronousTransfer;
  EFI_USB2_HC_PROTOCOL_ASYNC_ISOCHRONOUS_TRANSFER  AsyncIsochronousTransfer;
  EFI_USB2_HC_PROTOCOL_GET_ROOTHUB_PORT_STATUS     GetRootHubPortStatus;
  EFI_USB2_HC_PROTOCOL_SET_ROOTHUB_PORT_FEATURE    SetRootHubPortFeature;
  EFI_USB2_HC_PROTOCOL_CLEAR_ROOTHUB_PORT_FEATURE  ClearRootHubPortFeature;
  UINT16                                           MajorRevision;
  UINT16                                           MinorRevision;
};

extern EFI_GUID gEfiUsb2HcProtocolGuid;

#endif
