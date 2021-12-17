#pragma once
#include "GameObject.h"
namespace NCL {
	namespace CSC8503 {
		class StateMachine;
		class StateAIObject : public GameObject {
		public:
			StateAIObject();
			~StateAIObject();

			virtual void Update(float dt);

			Vector3 targetPos;
			Vector3 coinPos;
			Vector3 bumperPos;
			std::vector<GameObject*> coins;
			std::vector<GameObject*> bumpers;
		protected:
			void MoveToBall(float dt);
			void MoveFromBall(float dt);
			void HuntCoin(float dt);
			void AvoidBumper(float dt);

			StateMachine* stateMachine;
			float speed;
			float currentLength;
		};
	}
}