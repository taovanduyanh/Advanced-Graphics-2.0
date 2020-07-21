#pragma comment(lib, "nclgl.lib")

#include "../../nclgl/window.h"
#include "Renderer.h"

int main() {
	Window w("Ray Tracing - Tao Van Duy Anh", 800, 600, false);
	if (!w.HasInitialised()) {
		return -1;
	}

	Renderer renderer(w);
	if (!renderer.HasInitialised()) {
		return -1;
	}

	w.LockMouseToWindow(true);
	w.ShowOSPointer(false);

	while (w.UpdateWindow() && !Window::GetKeyboard()->KeyDown(KEYBOARD_ESCAPE)) {
		renderer.UpdateScene(w.GetTimer()->GetTimedMS());
		renderer.RenderScene();

		if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_R)) {
			renderer.ResetCamera();
		}
	}

	return 0;
}