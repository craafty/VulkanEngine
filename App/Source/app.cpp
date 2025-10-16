#include "vulkan_app.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define APP_NAME "Vulkan Window"

int main(int argc, char* argv[])
{
	VulkanApp App(WINDOW_WIDTH, WINDOW_HEIGHT);

	App.Init(APP_NAME);

	App.Execute();

	return 0;
}