/*******************************************************************************
 Copyright(c) 2017 Radek Kaczorek  <rkaczorek AT gmail DOT com>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License version 2 as published by the Free Software Foundation.
 .
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.
 .
 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <memory>
#include <typeinfo>
#include "config.h"

#include "amh_focuser.h"

#define FOCUSNAMEF "Motor HAT Focuser"

#define MAX_STEPS 20000
#define SPR 400 // steps per revolution

// We declare a pointer to indiAMHFocuser.
std::unique_ptr<IndiAMHFocuser> indiAMHFocuser(new IndiAMHFocuser);

void ISPoll(void *p);
void ISGetProperties(const char *dev)
{
        indiAMHFocuser->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num)
{
		indiAMHFocuser->ISNewSwitch(dev, name, states, names, num);
}

void ISNewText(	const char *dev, const char *name, char *texts[], char *names[], int num)
{
		indiAMHFocuser->ISNewText(dev, name, texts, names, num);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num)
{
		indiAMHFocuser->ISNewNumber(dev, name, values, names, num);
}

void ISNewBLOB (const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[], char *names[], int n)
{
  INDI_UNUSED(dev);
  INDI_UNUSED(name);
  INDI_UNUSED(sizes);
  INDI_UNUSED(blobsizes);
  INDI_UNUSED(blobs);
  INDI_UNUSED(formats);
  INDI_UNUSED(names);
  INDI_UNUSED(n);
}

void ISSnoopDevice (XMLEle *root)
{
	indiAMHFocuser->ISSnoopDevice(root);
}

IndiAMHFocuser::IndiAMHFocuser()
{
	setVersion(VERSION_MAJOR,VERSION_MINOR);
	FI::SetCapability(FOCUSER_CAN_ABS_MOVE | FOCUSER_CAN_REL_MOVE);
}

IndiAMHFocuser::~IndiAMHFocuser()
{

}

const char * IndiAMHFocuser::getDefaultName()
{
	return FOCUSNAMEF;
}

bool IndiAMHFocuser::Connect()
{
	// set device speed
	steppers[activeStepper].setSpeed(30); //  # 30 RPM

	IDMessage(getDeviceName(), "Adafruit Motor HAT Focuser connected successfully.");
	return true;
}

bool IndiAMHFocuser::Disconnect()
{
	// park focuser
	if ( FocusParkingS[0].s == ISS_ON )
	{
	    IDMessage(getDeviceName(), "Adafruit Motor HAT Focuser is parking...");
	    MoveAbsFocuser(FocusAbsPosN[0].min);
	}

	// close device
	hat.resetAll();

	IDMessage(getDeviceName(), "Adafruit Motor HAT Focuser disconnected successfully.");
	return true;
}

bool IndiAMHFocuser::initProperties()
{
    INDI::Focuser::initProperties();

	addDebugControl();

	// options tab
	IUFillNumber(&MotorSpeedN[0],"MOTOR_SPEED","RPM","%0.0f",10,250,10,30);
	IUFillNumberVector(&MotorSpeedNP,MotorSpeedN,1,getDeviceName(),"MOTOR_CONFIG","Focuser Speed",OPTIONS_TAB,IP_RW,0,IPS_OK);

	IUFillSwitch( &StepperStyleS[0], "SINGLE", "Single", ISS_ON);
	IUFillSwitch( &StepperStyleS[1], "DOUBLE", "Double", ISS_OFF);
	IUFillSwitch( &StepperStyleS[2], "INTERLEAVE", "Interleave", ISS_OFF);
	IUFillSwitch( &StepperStyleS[3], "MICROSTEP", "Microstep", ISS_OFF);
	IUFillSwitchVector(&StepperStyleSP,StepperStyleS,4,getDeviceName(),"STEPPER_STYLE","Stepper Style",OPTIONS_TAB,IP_RW,ISR_1OFMANY,60,IPS_OK);

	IUFillSwitch( &StepperChannelS[0], "STEPPER_A", "Stepper A", ISS_ON);
	IUFillSwitch( &StepperChannelS[1], "STEPPER_B", "Stepper B", ISS_OFF);
	IUFillSwitchVector(&StepperChannelSP,StepperChannelS,2,getDeviceName(),"STEPPER_CHANNEL","Stepper Channel",OPTIONS_TAB,IP_RW,ISR_1OFMANY,60,IPS_OK);


	IUFillSwitch(&MotorDirS[0],"FORWARD","Normal",ISS_ON);
	IUFillSwitch(&MotorDirS[1],"REVERSE","Reverse",ISS_OFF);
	IUFillSwitchVector(&MotorDirSP,MotorDirS,2,getDeviceName(),"MOTOR_DIR","Motor Dir",OPTIONS_TAB,IP_RW,ISR_1OFMANY,60,IPS_OK);

	IUFillNumber(&FocusBacklashN[0], "FOCUS_BACKLASH_VALUE", "Steps", "%0.0f", 0, 500, 1, 0);
	IUFillNumberVector(&FocusBacklashNP, FocusBacklashN, 1, getDeviceName(), "FOCUS_BACKLASH", "Backlash", OPTIONS_TAB, IP_RW, 0, IPS_IDLE);

	IUFillSwitch(&FocusParkingS[0],"FOCUS_PARKON","Enable",ISS_ON);
	IUFillSwitch(&FocusParkingS[1],"FOCUS_PARKOFF","Disable",ISS_OFF);
	IUFillSwitchVector(&FocusParkingSP,FocusParkingS,2,getDeviceName(),"FOCUS_PARK","Parking",OPTIONS_TAB,IP_RW,ISR_1OFMANY,60,IPS_OK);

	IUFillSwitch(&FocusResetS[0],"FOCUS_RESET","Reset",ISS_OFF);
	IUFillSwitchVector(&FocusResetSP,FocusResetS,1,getDeviceName(),"FOCUS_RESET","Position Reset",OPTIONS_TAB,IP_RW,ISR_1OFMANY,60,IPS_OK);

	// main tab
	IUFillSwitch(&FocusMotionS[0],"FOCUS_INWARD","Focus In",ISS_OFF);
	IUFillSwitch(&FocusMotionS[1],"FOCUS_OUTWARD","Focus Out",ISS_ON);
	IUFillSwitchVector(&FocusMotionSP,FocusMotionS,2,getDeviceName(),"FOCUS_MOTION","Direction",MAIN_CONTROL_TAB,IP_RW,ISR_ATMOST1,60,IPS_OK);

	IUFillNumber(&FocusRelPosN[0],"FOCUS_RELATIVE_POSITION","Steps","%0.0f",0,(int)MAX_STEPS/10,(int)MAX_STEPS/100,(int)MAX_STEPS/100);
	IUFillNumberVector(&FocusRelPosNP,FocusRelPosN,1,getDeviceName(),"REL_FOCUS_POSITION","Relative",MAIN_CONTROL_TAB,IP_RW,60,IPS_OK);

	IUFillNumber(&FocusAbsPosN[0],"FOCUS_ABSOLUTE_POSITION","Steps","%0.0f",0,MAX_STEPS,(int)MAX_STEPS/100,0);
	IUFillNumberVector(&FocusAbsPosNP,FocusAbsPosN,1,getDeviceName(),"ABS_FOCUS_POSITION","Absolute",MAIN_CONTROL_TAB,IP_RW,0,IPS_OK);

	IUFillNumber(&PresetN[0], "Preset 1", "", "%0.0f", 0, MAX_STEPS, (int)(MAX_STEPS/100), 0);
	IUFillNumber(&PresetN[1], "Preset 2", "", "%0.0f", 0, MAX_STEPS, (int)(MAX_STEPS/100), 0);
	IUFillNumber(&PresetN[2], "Preset 3", "", "%0.0f", 0, MAX_STEPS, (int)(MAX_STEPS/100), 0);
	IUFillNumberVector(&PresetNP, PresetN, 3, getDeviceName(), "Presets", "Presets", "Presets", IP_RW, 0, IPS_IDLE);

	IUFillSwitch(&PresetGotoS[0], "Preset 1", "Preset 1", ISS_OFF);
	IUFillSwitch(&PresetGotoS[1], "Preset 2", "Preset 2", ISS_OFF);
	IUFillSwitch(&PresetGotoS[2], "Preset 3", "Preset 3", ISS_OFF);
	IUFillSwitchVector(&PresetGotoSP, PresetGotoS, 3, getDeviceName(), "Presets Goto", "Goto", MAIN_CONTROL_TAB,IP_RW,ISR_1OFMANY,60,IPS_OK);

	// set default values
	dir = FOCUS_OUTWARD;
	mode = SINGLE;

	return true;
}

void IndiAMHFocuser::ISGetProperties (const char *dev)
{
	if(dev && strcmp(dev,getDeviceName()))
		return;

	INDI::Focuser::ISGetProperties(dev);

	addDebugControl();
    return;
}

bool IndiAMHFocuser::updateProperties()
{

    INDI::Focuser::updateProperties();

    if (isConnected())
    {
	defineNumber(&FocusAbsPosNP);
	defineNumber(&FocusRelPosNP);
	defineSwitch(&FocusMotionSP);
	defineSwitch(&FocusParkingSP);
	defineSwitch(&FocusResetSP);
	defineSwitch(&MotorDirSP);
	defineSwitch(&StepperStyleSP);
	defineSwitch(&StepperChannelSP);
	defineNumber(&MotorSpeedNP);
	defineNumber(&FocusBacklashNP);
    }
    else
    {
	deleteProperty(FocusAbsPosNP.name);
	deleteProperty(FocusRelPosNP.name);
	deleteProperty(FocusMotionSP.name);
	deleteProperty(FocusParkingSP.name);
	deleteProperty(FocusResetSP.name);
	deleteProperty(MotorDirSP.name);
	deleteProperty(StepperStyleSP.name);
	deleteProperty(StepperChannelSP.name);
	deleteProperty(MotorSpeedNP.name);
	deleteProperty(FocusBacklashNP.name);
    }

    return true;
}

bool IndiAMHFocuser::ISNewNumber (const char *dev, const char *name, double values[], char *names[], int n)
{
	// first we check if it's for our device
	if(strcmp(dev,getDeviceName())==0)
	{

        // handle focus absolute position
        if (!strcmp(name, FocusAbsPosNP.name))
        {
		int newPos = (int) values[0];
            if ( MoveAbsFocuser(newPos) == IPS_OK )
            {
               IUUpdateNumber(&FocusAbsPosNP,values,names,n);
               FocusAbsPosNP.s=IPS_OK;
               IDSetNumber(&FocusAbsPosNP, NULL);
            }
            return true;
        }

        // handle focus relative position
        if (!strcmp(name, FocusRelPosNP.name))
        {
	    IUUpdateNumber(&FocusRelPosNP,values,names,n);
	    FocusRelPosNP.s=IPS_OK;
	    IDSetNumber(&FocusRelPosNP, NULL);

	    //FOCUS_INWARD
            if ( FocusMotionS[0].s == ISS_ON )
		MoveRelFocuser(FOCUS_INWARD, FocusRelPosN[0].value);

	    //FOCUS_OUTWARD
            if ( FocusMotionS[1].s == ISS_ON )
		MoveRelFocuser(FOCUS_OUTWARD, FocusRelPosN[0].value);

	   return true;
        }

        // handle step delay
        if (!strcmp(name, MotorSpeedNP.name))
        {
            IUUpdateNumber(&MotorSpeedNP,values,names,n);
            MotorSpeedNP.s=IPS_BUSY;
            IDSetNumber(&MotorSpeedNP, NULL);
            steppers[activeStepper].setSpeed(MotorSpeedN[0].value); //  # 30 RPM
            MotorSpeedNP.s=IPS_OK;
            IDSetNumber(&MotorSpeedNP, "Adafruit Motor HAT Focuser speed set to %d RPM", (int) MotorSpeedN[0].value);
            return true;
        }

        // handle focus backlash
        if (!strcmp(name, FocusBacklashNP.name))
        {
            IUUpdateNumber(&FocusBacklashNP,values,names,n);
            FocusBacklashNP.s=IPS_OK;
            IDSetNumber(&FocusBacklashNP, "Adafruit Motor HAT Focuser backlash set to %d steps", (int) FocusBacklashN[0].value);
            return true;
        }

    }
    return INDI::Focuser::ISNewNumber(dev,name,values,names,n);
}

bool IndiAMHFocuser::ISNewSwitch (const char *dev, const char *name, ISState *states, char *names[], int n)
{
	// first we check if it's for our device
    if (!strcmp(dev, getDeviceName()))
    {
        // handle focus presets
        if (!strcmp(name, PresetGotoSP.name))
        {
            IUUpdateSwitch(&PresetGotoSP, states, names, n);

			//Preset 1
            if ( PresetGotoS[0].s == ISS_ON )
			MoveAbsFocuser(PresetN[0].value);

			//Preset 2
            if ( PresetGotoS[1].s == ISS_ON )
			MoveAbsFocuser(PresetN[1].value);

			//Preset 2
            if ( PresetGotoS[2].s == ISS_ON )
			MoveAbsFocuser(PresetN[2].value);

	    PresetGotoS[0].s = ISS_OFF;
	    PresetGotoS[1].s = ISS_OFF;
	    PresetGotoS[2].s = ISS_OFF;
	    PresetGotoSP.s = IPS_OK;
	    IDSetSwitch(&PresetGotoSP, NULL);
            return true;
        }

        // handle focus reset
        if(!strcmp(name, FocusResetSP.name))
        {
	    IUUpdateSwitch(&FocusResetSP, states, names, n);

            if ( FocusResetS[0].s == ISS_ON && FocusAbsPosN[0].value == FocusAbsPosN[0].min  )
            {
		FocusAbsPosN[0].value = (int)MAX_STEPS/100;
		IDSetNumber(&FocusAbsPosNP, NULL);
		MoveAbsFocuser(0);
	    }

            FocusResetS[0].s = ISS_OFF;
            IDSetSwitch(&FocusResetSP, NULL);
	    return true;
	}

        // handle parking mode
        if(!strcmp(name, FocusParkingSP.name))
        {
	    IUUpdateSwitch(&FocusParkingSP, states, names, n);
	    FocusParkingSP.s = IPS_BUSY;
	    IDSetSwitch(&FocusParkingSP, NULL);
	    FocusParkingSP.s = IPS_OK;
	    IDSetSwitch(&FocusParkingSP, NULL);
	    return true;
	}

        // handle motor direction
        if(!strcmp(name, MotorDirSP.name))
        {
	    IUUpdateSwitch(&MotorDirSP, states, names, n);
	    MotorDirSP.s = IPS_BUSY;
	    IDSetSwitch(&MotorDirSP, NULL);
	    MotorDirSP.s = IPS_OK;
	    IDSetSwitch(&MotorDirSP, NULL);
	    return true;
	}
	
        // handle motor channel
    if(!strcmp(name, StepperChannelSP.name))
    {
	    IUUpdateSwitch(&StepperChannelSP, states, names, n);
		if( StepperChannelS[1].s == ISS_ON)
			{
				activeStepper = 1;
				IDMessage(getDeviceName(), "Active Stepper set to CHANNEL_B.");
			}
			else
			{
				activeStepper = 0;
				IDMessage(getDeviceName(), "Active Stepper set to CHANNEL_A.");
			}
	    
	    StepperChannelSP.s = IPS_BUSY;
	    IDSetSwitch(&StepperChannelSP, NULL);
	    StepperChannelSP.s = IPS_OK;
	    IDSetSwitch(&StepperChannelSP, NULL);
	    return true;
	}
	
	// Handle Mode (Style)
	if( !strcmp(name, StepperStyleSP.name))
	{
	    IUUpdateSwitch(&StepperStyleSP, states, names, n);
	    
	    if( StepperStyleS[0].s == ISS_ON)
	    {
			mode = SINGLE;
			IDMessage(getDeviceName(), "Mode set to SINGLE.");
	    }
	    if( StepperStyleS[1].s == ISS_ON)
	    {
			mode = DOUBLE;
			IDMessage(getDeviceName(), "Mode set to DOUBLE.");
		}
	    if( StepperStyleS[2].s == ISS_ON)
		{
			mode = INTERLEAVE;
			IDMessage(getDeviceName(), "Mode set to INTERLEAVE.");
	    }
	    if( StepperStyleS[3].s == ISS_ON)
	    {
			mode = MICROSTEP;
			IDMessage(getDeviceName(), "Mode set to MICROSTEP.");
		}
	    
	    StepperStyleSP.s = IPS_BUSY;
	    IDSetSwitch(&StepperStyleSP, NULL);
	    StepperStyleSP.s = IPS_OK;
	    IDSetSwitch(&StepperStyleSP, NULL);
	    return true;
		
	}

        // handle focus abort - TODO
/*
        if (!strcmp(name, AbortSP.name))
        {
            IUUpdateSwitch(&AbortSP, states, names, n);
            if ( AbortFocuser() )
            {
				//FocusAbsPosNP.s = IPS_IDLE;
				//IDSetNumber(&FocusAbsPosNP, NULL);
				AbortS[0].s = ISS_OFF;
				AbortSP.s = IPS_OK;
			}
			else
			{
				AbortSP.s = IPS_ALERT;
			}

			IDSetSwitch(&AbortSP, NULL);
            return true;
        }
*/
    }
    return INDI::Focuser::ISNewSwitch(dev,name,states,names,n);
}
bool IndiAMHFocuser::saveConfigItems(FILE *fp)
{
	IUSaveConfigNumber(fp, &PresetNP);
	IUSaveConfigNumber(fp, &MotorSpeedNP);
	IUSaveConfigNumber(fp, &FocusBacklashNP);
	IUSaveConfigSwitch(fp, &FocusParkingSP);
	IUSaveConfigSwitch(fp, &MotorDirSP);
	IUSaveConfigSwitch(fp, &StepperStyleSP);
	IUSaveConfigSwitch(fp, &StepperChannelSP);

	if ( FocusParkingS[0].s == ISS_ON )
		IUSaveConfigNumber(fp, &FocusAbsPosNP);

	return true;
}

IPState IndiAMHFocuser::MoveFocuser(FocusDirection direction, int duration)
{
	int ticks = (int) ( SPR * duration / MotorSpeedN[0].value );
	return 	MoveRelFocuser(direction, ticks);
}


IPState IndiAMHFocuser::MoveRelFocuser(FocusDirection direction, int ticks)
{
	int targetTicks = FocusAbsPosN[0].value + (ticks * (direction == FOCUS_INWARD ? -1 : 1));
	return MoveAbsFocuser(targetTicks);
}

IPState IndiAMHFocuser::MoveAbsFocuser(int targetTicks)
{
    if (targetTicks < FocusAbsPosN[0].min || targetTicks > FocusAbsPosN[0].max)
    {
        IDMessage(getDeviceName(), "Requested position is out of range.");
        return IPS_ALERT;
    }

    if (targetTicks == FocusAbsPosN[0].value)
    {
        IDMessage(getDeviceName(), "Adafruit Motor HAT Focuser already in the requested position.");
        return IPS_OK;
    }

	// set focuser busy
	FocusAbsPosNP.s = IPS_BUSY;
	IDSetNumber(&FocusAbsPosNP, NULL);

	// check last motion direction for backlash triggering
	FocusDirection lastdir = dir;

	// set direction
	if (targetTicks > FocusAbsPosN[0].value)
	{
	    dir = FOCUS_OUTWARD;
	    IDMessage(getDeviceName() , "Adafruit Motor HAT Focuser is moving %s outward by %f", StepperChannelS[activeStepper].name, abs(targetTicks - FocusAbsPosN[0].value));
	}
	else
	{
	    dir = FOCUS_INWARD;
	    IDMessage(getDeviceName() , "Adafruit Motor HAT Focuser is moving %s inward by %f", StepperChannelS[activeStepper].name, abs(targetTicks - FocusAbsPosN[0].value));
	}

	// if direction changed do backlash adjustment - TO DO
	if ( FocusBacklashN[0].value != 0 && lastdir != dir && FocusAbsPosN[0].value != 0 )
	{
		IDMessage(getDeviceName() , "Adafruit Motor HAT Focuser backlash compensation by %0.0f steps...", FocusBacklashN[0].value);
		StepperMotor(FocusBacklashN[0].value, dir);
	}

	// process targetTicks
	int ticks = abs(targetTicks - FocusAbsPosN[0].value);

	// GO
	StepperMotor(ticks, dir);

	// update abspos value and status
	FocusAbsPosN[0].value = targetTicks;
	IDSetNumber(&FocusAbsPosNP, "Adafruit Motor HAT Focuser moved to position %0.0f", FocusAbsPosN[0].value );
	FocusAbsPosNP.s = IPS_OK;
	IDSetNumber(&FocusAbsPosNP, NULL);

	return IPS_OK;
}

int IndiAMHFocuser::StepperMotor(int steps, FocusDirection direction)
{

	IDMessage( getDeviceName(),  "activeStepper: %d mode: %d\n", activeStepper, mode);
 
	if(direction == FOCUS_OUTWARD)
	{
		if ( MotorDirS[1].s == ISS_ON )
		{
			//clockwise out
			steppers[activeStepper].step(steps, FORWARD,  mode);
		}
		else
		{
			//clockwise in
			steppers[activeStepper].step(steps, BACKWARD,  mode);
		}
	}
	else
	{
		if ( MotorDirS[1].s == ISS_ON )
		{
			//clockwise in
			steppers[activeStepper].step(steps, BACKWARD,  mode);
		}
		else
		{
			//clockwise out
			steppers[activeStepper].step(steps, FORWARD,  mode);
		}
	}

	// zero all PWM ports to release motor
	hat.resetAll();

	return 0;
}
bool IndiAMHFocuser::AbortFocuser()
{
	// TODO
	IDMessage(getDeviceName() , "Adafruit Motor HAT Focuser aborted");
	return true;
}
