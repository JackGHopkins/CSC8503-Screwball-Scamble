#pragma once
#include "GameObject.h"
namespace NCL {
	namespace CSC8503 {
		class StateMachine;
		class SMPushBlock : public GameObject {
		public:
			SMPushBlock(Vector3 position, Vector3 force, float eLimit);
			~SMPushBlock();

			virtual void Update(float dt);

		protected:
			StateMachine* stateMachine;
			Vector3 initPosition;
			Vector3 fireForce;

			float fireDist;
			bool fired;
		};

	}
}