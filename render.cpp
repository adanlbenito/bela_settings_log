#include "Bela.h"
#include "MiscUtilities.h"
#include <libraries/Gui/Gui.h>
#include <libraries/GuiController/GuiController.h>
#include <vector>
#include <string.h>

std::string tuningFilePath = "string_tunings.txt";
std::string tuningToken = "tuning_string_";

std::vector<float> gTunings{ 440.0, 440.0, 440.0, 440.0, 440.0, 440.0};
float gTuningRange[2] = {50.0, 500.0};
std::vector<int> gTuningSliderId;

int gLogCount = -1;
float gLogSpanSec = 3;

Gui gui;
GuiController controller;

AuxiliaryTask gLogSettingsTask;

void log_settings(void*);

/*** SETUP ***/
bool setup(BelaContext *context, void *userData)
{
	for(unsigned int i = 0; i < gTunings.size(); i++)
	{
		std::string tuningId = tuningToken + std::to_string(i+1);
		printf("%s\n", tuningId.c_str());
		std::string stringTuning = ConfigFileUtils::readValue(tuningFilePath, tuningId);
		if(stringTuning.empty())
		{
			printf("Tuning frequency not found for string %d\n", i+1);
		}
		else
		{
			gTunings[i] = std::stof(stringTuning);
			printf("Tunning frequency for string %d -> %f\n", i+1, gTunings[i]);
		}
	}

	// Set up the GUI
	gui.setup(context->projectName);
	// and attach to it
	controller.setup(&gui, "Tunings");

	//Retrieve tunings from file
	for(unsigned int i = 0; i < gTunings.size(); i++)
	{
		std::string sliderName = "Tuning String" + std::to_string(i+1);
		gTuningSliderId.push_back(controller.addSlider(sliderName, gTunings[i], gTuningRange[0], gTuningRange[1], 0.001));
	}

	// Initialise auxiliary tasks
	if((gLogSettingsTask = Bela_createAuxiliaryTask(&log_settings, 85, "bela-log-settings")) == 0)
		return false;

	return true;
}

/*** RENDER ***/
void render(BelaContext *context, void *userData)
{
	bool guiHasUpdated = false;
	for(unsigned int i = 0; i < gTunings.size(); i++)
	{
		guiHasUpdated |= controller.getSlider(gTuningSliderId[i]).hasChanged();
		gTunings[i] = controller.getSliderValue(gTuningSliderId[i]);
	}

	if(guiHasUpdated)
	{
		gLogCount = int(gLogSpanSec * context->audioSampleRate);
	}

	for(unsigned int n = 0; n < context->audioFrames; n++)
	{
		if(gLogCount > -1)
		{
			--gLogCount;
		}
		if(gLogCount == 0)
		{
			rt_printf("Schedule data logging\n");

			// Request that the lower-priority task run at next opportunity
			Bela_scheduleAuxiliaryTask(gLogSettingsTask);
		}
	}
}

void cleanup(BelaContext *context, void *userData)
{
	log_settings();
}

void log_settings(void*)
{
	for(unsigned int i = 0; i < gTunings.size(); i++)
	{
		std::string tuningId = tuningToken + std::to_string(i+1);
		if(i == 0)
		{
			// First write truncates file and ignores its contents
			ConfigFileUtils::writeValue(tuningFilePath, tuningId, std::to_string(gTunings[i]), IoUtils::TRUNCATE);
		}
		else
		{
			// Following writes append content
			ConfigFileUtils::writeValue(tuningFilePath, tuningId, std::to_string(gTunings[i]), IoUtils::APPEND);
		}
	}
}
