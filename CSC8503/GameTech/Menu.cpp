#include "Menu.h"

MainMenu::MainMenu() {
	world = new GameWorld();
	renderer = new GameTechRenderer(*world);

	Debug::SetRenderer(renderer);
	DrawMenu();
}

MainMenu::~MainMenu() {
	delete renderer;
	delete world;
}

void MainMenu::Update(float dt) {
	renderer->Update(dt);

	Debug::FlushRenderables(dt);
	renderer->Render();
	DrawMenu();
}

void MainMenu::DrawMenu() {
	renderer->DrawString("J.Hopkins' CSC38503 Coursework!", Vector2(5, 5), Vector4(1,0,0,1), 25.0f);
	renderer->DrawString("Press 1 for 'Screwball Srcamball'", Vector2(5, 35));
	renderer->DrawString("Press 2 for 'Maze Game'", Vector2(5, 40));
	renderer->DrawString("Press ESC to Exit", Vector2(60, 90));
}

// Pause

PauseMenu::PauseMenu() {
}

PauseMenu::~PauseMenu() {
}


