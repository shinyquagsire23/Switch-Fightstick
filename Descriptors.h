#ifndef _DESCRIPTORS_H_
#define _DESCRIPTORS_H_

// Includes
#include <LUFA/Drivers/USB/USB.h>

#include <avr/pgmspace.h>

// Type Defines
// Device Configuration Descriptor Structure
typedef struct
{
	USB_Descriptor_Configuration_Header_t Config;

	// Joystick HID Interface
	USB_Descriptor_Interface_t            HID_Interface;
	USB_HID_Descriptor_HID_t              HID_JoystickHID;
	USB_Descriptor_Endpoint_t             HID_ReportOUTEndpoint;
	USB_Descriptor_Endpoint_t             HID_ReportINEndpoint;
} USB_Descriptor_Configuration_t;

// Device Interface Descriptor IDs
enum InterfaceDescriptors_t
{
	INTERFACE_ID_Joystick = 0, /**< Joystick interface descriptor ID */
};

// Device String Descriptor IDs
enum StringDescriptors_t
{
	STRING_ID_Language     = 0, // Supported Languages string descriptor ID (must be zero)
	STRING_ID_Manufacturer = 1, // Manufacturer string ID
	STRING_ID_Product      = 2, // Product string ID
};

// Macros
// Endpoint Addresses
#define JOYSTICK_IN_EPADDR  (ENDPOINT_DIR_IN  | 1)
#define JOYSTICK_OUT_EPADDR (ENDPOINT_DIR_OUT | 2)
// HID Endpoint Size
// The Switch -needs- this to be 64.
// The Wii U is flexible, allowing us to use the default of 8 (which did not match the original Hori descriptors).
#define JOYSTICK_EPSIZE           64
// Descriptor Header Type - HID Class HID Descriptor
#define DTYPE_HID                 0x21
// Descriptor Header Type - HID Class HID Report Descriptor
#define DTYPE_Report              0x22
// Joystick endpoint polling interval (in ms). It looks like any value set here renders in a timing multiple of 8 ms.
#define POLLING_MS 8

// Function Prototypes
uint16_t CALLBACK_USB_GetDescriptor(
	const uint16_t wValue,
	const uint16_t wIndex,
	const void** const DescriptorAddress
) ATTR_WARN_UNUSED_RESULT ATTR_NON_NULL_PTR_ARG(3);

#endif
