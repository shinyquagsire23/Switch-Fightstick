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
 *  Main source file for the posts printer demo. This file contains the main tasks of
 *  the demo and is responsible for the initial application hardware configuration.
 */

#include "Joystick.h"

extern const uint8_t image_data[0x12c1] PROGMEM;

// Main entry point.
int main(void)
{
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
void SetupHardware(void)
{
	// We need to disable watchdog if enabled by bootloader/fuses.
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	// We need to disable clock division before initializing the USB hardware.
	clock_prescale_set(clock_div_1);

	// We can then initialize our hardware and peripherals, including the USB stack.

#ifdef ALERT_WHEN_DONE
	// Both PORTD and PORTB will be used for the optional LED flashing and buzzer.
#warning LED and Buzzer functionality enabled. All pins on both PORTB and PORTD will toggle when printing is done.
	DDRD = 0xFF; //Teensy uses PORTD
	PORTD = 0x0;
	//We'll just flash all pins on both ports since the UNO R3
	DDRB = 0xFF; //uses PORTB. Micro can use either or, but both give us 2 LEDs
	PORTB = 0x0; //The ATmega328P on the UNO will be resetting, so unplug it?
#endif

	// The USB stack should be initialized last.
	USB_Init();
}

// Fired to indicate that the device is enumerating.
void EVENT_USB_Device_Connect(void)
{
	// We can indicate that we're enumerating here (via status LEDs, sound, etc.).
}

// Fired to indicate that the device is no longer connected to a host.
void EVENT_USB_Device_Disconnect(void)
{
	// We can indicate that our device is not ready (via status LEDs, sound, etc.).
}

// Fired when the host set the current configuration of the USB device after enumeration.
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	// We setup the HID report endpoints.
	ConfigSuccess &= Endpoint_ConfigureEndpoint(JOYSTICK_OUT_EPADDR, EP_TYPE_INTERRUPT, JOYSTICK_EPSIZE, 1);
	ConfigSuccess &= Endpoint_ConfigureEndpoint(JOYSTICK_IN_EPADDR, EP_TYPE_INTERRUPT, JOYSTICK_EPSIZE, 1);

	// We can read ConfigSuccess to indicate a success or failure at this point.
}

// Process control requests sent to the device from the USB host.
void EVENT_USB_Device_ControlRequest(void)
{
	// We can handle two control requests: a GetReport and a SetReport.

	// Not used here, it looks like we don't receive control request from the Switch.
}

// Process and deliver data from IN and OUT endpoints.
void HID_Task(void)
{
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
	}
}

typedef enum {
	SYNC_CONTROLLER,
	SYNC_POSITION,
	ZIG_ZAG,
	MOVE,
	STOP,
	DONE
} State_t;
State_t state = SYNC_CONTROLLER;

// Repeat ECHOES times the last sent report.
//
// This value is affected by several factors:
// - The descriptors *.PollingIntervalMS value.
// - The Switch readiness to accept reports (driven by the Endpoint_IsINReady() function,
//   it looks to be 8 ms).
// - The Switch screen refresh rate (it looks that anything that would update the screen
//   at more than 30 fps triggers pixel skipping).
#if defined(ZIG_ZAG_PRINTING)
	#if defined(SYNC_TO_30_FPS)
		// In this case we will send 641 moves and 1 stop every 2 lines, using 4 reports for
		// each send (done in 32 ms). We will inject an additional report every 6 commands, to
		// align them to 6 video frames (lasting 200 ms).
		#define ECHOES 3
	#else
		// In this case we will send 641 moves and 1 stop every 2 lines, using 5 reports for
		// each send, in around 25 s (thus 8 ms per report), updating the screen every 40 ms.
		#define ECHOES 4
	#endif
#else
	// In this case we will send 320 moves and 320 stops per line, using 3 reports for each
	// send, in around 15 s (thus 8 ms per report), updating the screen every 48 ms.
	#define ECHOES 2
#endif
int echoes = 0;
USB_JoystickReport_Input_t last_report;

int command_count = 0;
int report_count = 0;
int xpos = 0;
int ypos = 0;
int portsval = 0;

#define max(a, b) (a > b ? a : b)
#define ms_2_count(ms) (ms / ECHOES / (max(POLLING_MS, 8) / 8 * 8))
#define is_black(x, y) (pgm_read_byte(&(image_data[((x) / 8) + ((y) * 40)])) & 1 << ((x) % 8))

void complete_zig_zag_pattern(USB_JoystickReport_Input_t *const ReportData)
{
	// This function move the dot, switching between two consecutive lines, following
	// the move pattern below while moving to the right:
	//
	//    3  4 ... N-5  N-4  N-1
	// 1  2  5 ... N-6  N-3  N-2 <- (N, N+1)
	//                       N+2
	//
	// and its specular one while moving to the left:
	//
	//             N-1  N-4  N-5 ... 4  3
	// (N, N+1) -> N-2  N-3  N-6 ... 5  2  1
	//             N+2
	//
	// In each pattern, the N and N+2 moves are the same, thus we need a stop in N+1,
	// to avoid the acceleration triggered by two consecutive moves done in the same
	// direction. This pattern pass on the same pixel 3 times (N-2, N and N+1), but
	// is the easiest to check that I found.
	uint8_t move_direction;

	if (ypos % 4 < 2)
		move_direction = HAT_RIGHT;
	else
		move_direction = HAT_LEFT;

	if (command_count < 642)
	{
		if (command_count % 2 == 1)
			ReportData->HAT = move_direction;
		else if (command_count % 4 == 0)
			ReportData->HAT = HAT_BOTTOM;
		else
			ReportData->HAT = HAT_TOP;
		if (command_count == 640)
			ReportData->HAT = HAT_CENTER;
		else if (command_count == 639 || command_count == 641)
			ReportData->HAT = HAT_BOTTOM;
		command_count++;
		return;
	}

	command_count = 0;
	return;
}

// Prepare the next report for the host
void GetNextReport(USB_JoystickReport_Input_t *const ReportData)
{

	// Prepare an empty report
	memset(ReportData, 0, sizeof(USB_JoystickReport_Input_t));
	ReportData->LX = STICK_CENTER;
	ReportData->LY = STICK_CENTER;
	ReportData->RX = STICK_CENTER;
	ReportData->RY = STICK_CENTER;
	ReportData->HAT = HAT_CENTER;

#if defined(ZIG_ZAG_PRINTING) && defined(SYNC_TO_30_FPS)
	if (state == ZIG_ZAG)
	{
		// Inject an additional echo every 192 ms, aligning the command stream to 200 ms (equivalent to 6 video frames)
		report_count++;
		if (report_count == 24) // this seems be the best spot to inject the echo...
		{
			memcpy(ReportData, &last_report, sizeof(USB_JoystickReport_Input_t));
			return;
		}
		if (report_count == 25) // reset the report count every 25 reports (200 ms)
			report_count = 0;
	}
#endif

	// Repeat ECHOES times the last report
	if (echoes > 0)
	{
		memcpy(ReportData, &last_report, sizeof(USB_JoystickReport_Input_t));
		echoes--;
		return;
	}

	// States and moves management
	switch (state)
	{
	case SYNC_CONTROLLER:
		if (command_count > ms_2_count(2000))
		{
			command_count = 0;
			state = SYNC_POSITION;
		}
		else
		{
			if (command_count == ms_2_count(500) || command_count == ms_2_count(1000))
				ReportData->Button |= SWITCH_L | SWITCH_R;
			else if (command_count == ms_2_count(1500) || command_count == ms_2_count(2000))
				ReportData->Button |= SWITCH_A;
			command_count++;
		}
		break;
	case SYNC_POSITION:
		if (command_count > ms_2_count(4000))
		{
			command_count = 0;
			xpos = 0;
			ypos = 0;
#if defined(ZIG_ZAG_PRINTING)
			state = ZIG_ZAG;
#else
			state = STOP;
#endif
		}
		else
		{
			// Moving faster with LX/LY
			ReportData->LX = STICK_MIN;
			ReportData->LY = STICK_MIN;
			// Clear the screen
			if (command_count == ms_2_count(1500) || command_count == ms_2_count(3000))
				ReportData->Button |= SWITCH_MINUS;
			command_count++;
		}
		break;
	case ZIG_ZAG:
		complete_zig_zag_pattern(ReportData);
		if (ypos > 119)
			state = DONE;
		break;
	case MOVE:
		if ((xpos == 0 && ypos % 2 == 1) || (xpos == 319 && ypos % 2 == 0))
			ReportData->HAT = HAT_BOTTOM;
		else if (ypos % 2 == 0)
			ReportData->HAT = HAT_RIGHT;
		else
			ReportData->HAT = HAT_LEFT;
		state = STOP;
		break;
	case STOP:
		state = MOVE;
		if (ypos > 119)
			state = DONE;
		break;
	case DONE:
#ifdef ALERT_WHEN_DONE
		portsval = ~portsval;
		PORTD = portsval; // flash LED(s) and sound buzzer if attached
		PORTB = portsval;
		_delay_ms(250);
#endif
		return;
	}

	if (state != SYNC_CONTROLLER && state != SYNC_POSITION && state != DONE)
	{
		// Position update (diagonal moves doesn't work since they ink two dots... is not necessary to test them)
		if (ReportData->HAT == HAT_RIGHT)
			xpos++;
		else if (ReportData->HAT == HAT_LEFT)
			xpos--;
		else if (ReportData->HAT == HAT_TOP)
			ypos--;
		else if (ReportData->HAT == HAT_BOTTOM)
			ypos++;

		// Inking (the printing patterns above will not move outside the canvas... is not necessary to test them)
		if (is_black(xpos, ypos))
			ReportData->Button |= SWITCH_A;
	}

	// Prepare to echo this report
	memcpy(&last_report, ReportData, sizeof(USB_JoystickReport_Input_t));
	echoes = ECHOES;
}
