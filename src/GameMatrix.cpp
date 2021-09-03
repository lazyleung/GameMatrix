#include <iostream>
#include <signal.h>

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
}

int main(int argc, char *argv[]) {
	
	std::cout << "Hello World!\n";

	// It is always good to set up a signal handler to cleanly exit when we
	// receive a CTRL-C for instance. The DrawOnCanvas() routine is looking
	// for that.
	signal(SIGTERM, InterruptHandler);
	signal(SIGINT, InterruptHandler);

	// DrawOnCanvas(canvas);    // Using the canvas.

	// Animation finished. Shut down the RGB matrix.
	// canvas->Clear();
	// delete canvas;

	return 0;
}