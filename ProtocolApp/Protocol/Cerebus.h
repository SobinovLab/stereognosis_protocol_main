/*********************************************************************
*
* Description:
*    This class manages Blackrock Cerebus recording
* 10 Digital Lines
* NI 6001 card
* Port 0 -> 0..7
* Port 1 -> 0..3
* BlackRock Cerebus Digital Input
* NI Digital ports P0 and P1
* 8 bits for data sync
* 4 bits for recording commands (Start/Stop/Pause/Resume) 
* MAPPING: 
* ProtocolBegin		  -> Line 0	Cerebus 37bits Cable - Pin -> 02  
* ProtocolEnd		  -> Line 1	Cerebus 37bits Cable -  Pin -> 03
* TrialBegin		  -> Line 2	Cerebus 37bits Cable -  Pin -> 04
* TrialEnd			  -> Line 3	Cerebus 37bits Cable -  Pin -> 05
* StimBegin			  -> Line 4	Cerebus 37bits Cable -  Pin -> 06
* StimEnd			  -> Line 5	Cerebus 37bits Cable -  Pin -> 07
* PostTrialBegin	  -> Line 6	Cerebus 37bits Cable -  Pin -> 08
* PostTrialEnd		  -> Line 7	Cerebus 37bits Cable -  Pin -> 09
* Start Recording     -> Line 8  Cerebus 37bits Cable -  Pin -> 10
* Stop Recording      -> Line 9  Cerebus 37bits Cable -  Pin -> 11
* Pause Recording     -> Line 10 Cerebus 37bits Cable -  Pin -> 12
* Resume Recording    -> Line 11 Cerebus 37bits Cable -  Pin -> 13
* Digital Ground is 32
*********************************************************************/
#pragma once

#include <stdio.h>
#include <NIDAQmx.h>

class Cerebus
{
	public:
		Cerebus();
		~Cerebus();

		void beginProtocol();
		void endProtocol();
		void beginTrial();
		void endTrial();
		void beginStim();
		void endStim();
		void beginPostTrial();
		void endPostTrial();
		void beginRecording();
		void endRecording();

	private:

		enum class Command : int { PROTOCOL_BEGIN = 0, PROTOCOL_END, TRIAL_BEGIN, TRIAL_END, STIM_BEGIN, STIM_END, POST_TRIAL_BEGIN, POST_TRIAL_END, START_RECORDING, STOP_RECORDING };
		void send(Command cmd);
		/*********************************************/
		// ****** Task handler for reading/writing ***
		/*********************************************/
		TaskHandle m_taskHandle;

};