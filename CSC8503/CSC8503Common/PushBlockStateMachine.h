#pragma once
#include "GameObject.h"
namespace NCL {
	namespace CSC8503 {
		class StateMachine;
		class PushBlockStateMachine {
		public:
			PushBlockStateMachine(Vector3 position, GameObject* block, Vector3 force, Vector3 direction, float eLimit);
			~PushBlockStateMachine() {}

			//virtual void Update(float dt);

		protected:
			StateMachine* stateMachine;
			GameObject* block;
			Vector3 initPosition;
			Vector3 fireDir;
			Vector3 fireForce;

			float fireDist;
			bool fired;
		};

	}
}