/*
Nintendo Switch Fightstick - Proof-of-Concept

Based on the LUFA library's Low-Level Joystick Demo
	(C) Dean Camera
Based on the HORI's Pokken Tournament Pro Pad design
	(C) HORI

This project implements a modified version of HORI's Pokken Tournament Pro Pad
USB descriptors to allow for the creation of custom controllers for the
Nintendo Switch. This also works to a limited degree on the PS3.

Since System Update v3.0.0, the Nintendo Switch recognizes the Pokken
Tournament Pro Pad as a Pro Controller. Physical design limitations prevent
the Pokken Controller from functioning at the same level as the Pro
Controller. However, by default most of the descriptors are there, with the
exception of Home and Capture. Descriptor modification allows us to unlock
these buttons for our use.
*/

/** \file
 *
 *  Main source file for the Joystick demo. This file contains the main tasks of the demo and
 *  is responsible for the initial application hardware configuration.
 */

#include "Joystick.h"

/*
The following ButtonMap variable defines all possible buttons within the
original 13 bits of space, along with attempting to investigate the remaining
3 bits that are 'unused'. This is what led to finding that the 'Capture'
button was operational on the stick.
*/
uint16_t ButtonMap[16] = {
	0x01, //Y
	0x02, //B
	0x04, //A
	0x08, //X
	0x10, //L
	0x20, //R
	0x40, //ZL
	0x80, //ZR
	0x100, //Minus
	0x200, //Plus
	0x400, //L-stick
	0x800, //R-stick
	0x1000, //Home
	0x2000, //Capture
	0x4000, //Unk
	0x8000, //Unk
};

const uint8_t image_data[0x12c1] PROGMEM;

/*** Debounce ****
The following is some -really bad- debounce code. I have a more robust library
that I've used in other personal projects that would be a much better use
here, especially considering that this is a stick indented for use with arcade
fighters.

This code exists solely to actually test on. This will eventually be replaced.
**** Debounce ***/
// Quick debounce hackery!
// We're going to capture each port separately and store the contents into a 32-bit value.
uint32_t pb_debounce = 0;
uint32_t pd_debounce = 0;

// We also need a port state capture. We'll use a 16-bit value for this.
uint16_t bd_state = 0;

// We'll also give us some useful macros here.
#define PINB_DEBOUNCED ((bd_state >> 0) & 0xFF)
#define PIND_DEBOUNCED ((bd_state >> 8) & 0xFF) 

// Main entry point.
int main(void) {
	// We'll start by performing hardware and peripheral setup.
	SetupHardware();
	// We'll then enable global interrupts for our use.
	GlobalInterruptEnable();
	// Once that's done, we'll enter an infinite loop.
	for (;;)
	{
		// We need to run our task to process and deliver data for our IN and OUT endpoints.
		HID_Task();
		// We also need to run the main USB management task.
		USB_USBTask();
	}
}

// Configures hardware and peripherals, such as the USB peripherals.
void SetupHardware(void) {
	// We need to disable watchdog if enabled by bootloader/fuses.
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	// We need to disable clock division before initializing the USB hardware.
	clock_prescale_set(clock_div_1);
	// We can then initialize our hardware and peripherals, including the USB stack.

	// Both PORTD and PORTB will be used for handling the buttons and stick.
	DDRD  &= ~0xFF;
	PORTD |=  0xFF;

	DDRB  &= ~0xFF;
	PORTB |=  0xFF;
	// The USB stack should be initialized last.
	USB_Init();
}

// Fired to indicate that the device is enumerating.
void EVENT_USB_Device_Connect(void) {
	// We can indicate that we're enumerating here (via status LEDs, sound, etc.).
}

// Fired to indicate that the device is no longer connected to a host.
void EVENT_USB_Device_Disconnect(void) {
	// We can indicate that our device is not ready (via status LEDs, sound, etc.).
}

// Fired when the host set the current configuration of the USB device after enumeration.
void EVENT_USB_Device_ConfigurationChanged(void) {
	bool ConfigSuccess = true;

	// We setup the HID report endpoints.
	ConfigSuccess &= Endpoint_ConfigureEndpoint(JOYSTICK_OUT_EPADDR, EP_TYPE_INTERRUPT, JOYSTICK_EPSIZE, 1);
	ConfigSuccess &= Endpoint_ConfigureEndpoint(JOYSTICK_IN_EPADDR, EP_TYPE_INTERRUPT, JOYSTICK_EPSIZE, 1);

	// We can read ConfigSuccess to indicate a success or failure at this point.
}

// Process control requests sent to the device from the USB host.
void EVENT_USB_Device_ControlRequest(void) {
	// We can handle two control requests: a GetReport and a SetReport.
	switch (USB_ControlRequest.bRequest)
	{
		// GetReport is a request for data from the device.
		case HID_REQ_GetReport:
			if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE))
			{
				// We'll create an empty report.
				USB_JoystickReport_Input_t JoystickInputData;
				// We'll then populate this report with what we want to send to the host.
				GetNextReport(&JoystickInputData);
				// Since this is a control endpoint, we need to clear up the SETUP packet on this endpoint.
				Endpoint_ClearSETUP();
				// Once populated, we can output this data to the host. We do this by first writing the data to the control stream.
				Endpoint_Write_Control_Stream_LE(&JoystickInputData, sizeof(JoystickInputData));
				// We then acknowledge an OUT packet on this endpoint.
				Endpoint_ClearOUT();
			}

			break;
		case HID_REQ_SetReport:
			if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE))
			{
				// We'll create a place to store our data received from the host.
				USB_JoystickReport_Output_t JoystickOutputData;
				// Since this is a control endpoint, we need to clear up the SETUP packet on this endpoint.
				Endpoint_ClearSETUP();
				// With our report available, we read data from the control stream.
				Endpoint_Read_Control_Stream_LE(&JoystickOutputData, sizeof(JoystickOutputData));
				// We then send an IN packet on this endpoint.
				Endpoint_ClearIN();
			}

			break;
	}
}

// Process and deliver data from IN and OUT endpoints.
void HID_Task(void) {
	// If the device isn't connected and properly configured, we can't do anything here.
	if (USB_DeviceState != DEVICE_STATE_Configured)
	  return;

	// We'll start with the OUT endpoint.
	Endpoint_SelectEndpoint(JOYSTICK_OUT_EPADDR);
	// We'll check to see if we received something on the OUT endpoint.
	if (Endpoint_IsOUTReceived())
	{
		// If we did, and the packet has data, we'll react to it.
		if (Endpoint_IsReadWriteAllowed())
		{
			// We'll create a place to store our data received from the host.
			USB_JoystickReport_Output_t JoystickOutputData;
			// We'll then take in that data, setting it up in our storage.
			Endpoint_Read_Stream_LE(&JoystickOutputData, sizeof(JoystickOutputData), NULL);
			// At this point, we can react to this data.
			// However, since we're not doing anything with this data, we abandon it.
		}
		// Regardless of whether we reacted to the data, we acknowledge an OUT packet on this endpoint.
		Endpoint_ClearOUT();
	}

	// We'll then move on to the IN endpoint.
	Endpoint_SelectEndpoint(JOYSTICK_IN_EPADDR);
	// We first check to see if the host is ready to accept data.
	if (Endpoint_IsINReady())
	{
		// We'll create an empty report.
		USB_JoystickReport_Input_t JoystickInputData;
		// We'll then populate this report with what we want to send to the host.
		GetNextReport(&JoystickInputData);
		// Once populated, we can output this data to the host. We do this by first writing the data to the control stream.
		Endpoint_Write_Stream_LE(&JoystickInputData, sizeof(JoystickInputData), NULL);
		// We then send an IN packet on this endpoint.
		Endpoint_ClearIN();

		/* Clear the report data afterwards */
		// memset(&JoystickInputData, 0, sizeof(JoystickInputData));
	}
}

int meme = 1;
int input_brakes = 0;
int input_count = 0;
int xpos = 0;
int ypos = -1;
bool print_right = true;
bool other = true;

bool reset_request = false;
bool reset = true, sync_setup = true;
int reset_count = 0;

bool done_printing = false;

// Prepare the next report for the host.
void GetNextReport(USB_JoystickReport_Input_t* const ReportData) {
	// All of this code here is handled -really poorly-, and should be replaced with something a bit more production-worthy.
	uint16_t buf_button   = 0x00;
	uint8_t  buf_joystick = 0x00;

	/* Clear the report contents */
	memset(ReportData, 0, sizeof(USB_JoystickReport_Input_t));

	buf_button   = 0x0;
	buf_joystick = 0x0;

	for (int i = 0; i < 16; i++) {
		if (buf_button & (1 << i))
			ReportData->Button |= ButtonMap[i];
	}

	if (buf_joystick & 0x10)
		ReportData->LX = 0;
	else if (buf_joystick & 0x20)
		ReportData->LX = 255;
	else
		ReportData->LX = 128;

	if (buf_joystick & 0x80)
		ReportData->LY = 0;
	else if (buf_joystick & 0x40)
		ReportData->LY = 255;
	else
		ReportData->LY = 128;

	switch(buf_joystick & 0xF0) {
		case 0x80: // Top
			ReportData->HAT = 0x00;
			break;
		case 0xA0: // Top-Right
			ReportData->HAT = 0x01;
			break;
		case 0x20: // Right
			ReportData->HAT = 0x02;
			break;
		case 0x60: // Bottom-Right
			ReportData->HAT = 0x03;
			break;
		case 0x40: // Bottom
			ReportData->HAT = 0x04;
			break;
		case 0x50: // Bottom-Left
			ReportData->HAT = 0x05;
			break;
		case 0x10: // Left
			ReportData->HAT = 0x06;
			break;
		case 0x90: // Top-Left
			ReportData->HAT = 0x07;
			break;
		default:
			ReportData->HAT = 0x08;
	}
	
	//ReportData->LX = xpos;
	//ReportData->LY = ypos;
	
	if (done_printing) return;
	
	if (reset)
	{
	   reset_count++;
	   
	   ReportData->HAT = 0x2;
	   
   	   if (
		reset_count % 100 == 0 &&
		reset_count < 400 &&
		sync_setup == true
	   )
	   	ReportData->Button |= 0x30;

	   if (reset_count == 500 && sync_setup == true)
	   	ReportData->Button |= 0x04;

	   if (reset_count >= 800)
	   {
	      reset_count = 0;
	      reset = false;
	      
	      xpos = 320;
	      ypos++;
	   }
	   return;
	}
	
	sync_setup = false;

	input_brakes++;
	
	if (input_brakes >= 5)
	{
	   if (reset_request)
	   {
	      reset = true;
	      input_brakes = 0;
	      reset_request = false;;
	      return;
	   }
	   
	   meme = !meme;
	   
	   if (!meme)
	      other = !other;
	      
	   input_brakes = 0;
	}
	
	if (meme)
	{
	   input_count = 0;
	   
	   if (xpos >= 0 && xpos < 320)
	   {
	      ReportData->HAT = 0x6; //go left
	   }
	   else
	   {
	      ReportData->HAT = 0x4;
	      reset_request = true;
	   }
	}
	else
	{
	   if (!input_count)
	   {
	      if (xpos >= 0)
	         xpos--;
	   }
	   
	   if ((pgm_read_byte(&(image_data[(xpos / 8)+(ypos*40)])) & 1 << (xpos % 8)) && (xpos >= 0))
	      ReportData->Button |= 0x4;
	      
	   if (xpos <= 0 && ypos >= 120-1) done_printing = true;

	   input_count++;
   }
}
