#pragma once
#include "GameTechRenderer.h"
#include "TutorialGame.h"
namespace NCL {
	namespace CSC8503 {
		class MainMenu {
		public:
			MainMenu();
			~MainMenu();

			void Update(float dt);

		protected:
			void DrawMenu();
			GameTechRenderer* renderer;
			GameWorld* world;
		};

		class PauseMenu {
		public:
			PauseMenu();
			~PauseMenu();
		};
	}
}