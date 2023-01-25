#include "server.h"

int main(int argc, char** argv) {
	AllocConsole();
	freopen("conin$", "r", stdin);
	freopen("conout$", "w", stdout);
	freopen("conout$", "w", stderr);
	AppointmentServer app;
	return app.run(argc, argv);
}