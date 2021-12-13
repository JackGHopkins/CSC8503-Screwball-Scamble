#include "StateGameObject.h"
#include "StateTransition.h"
#include "StateMachine.h"
#include "State.h"
#include "PushBlockStateMachine.h"

using namespace NCL;
using namespace CSC8503;

PushBlockStateMachine::PushBlockStateMachine(Vector3 position, GameObject* gameObj, Vector3 force, Vector3 direction, float distance) {
	stateMachine = new StateMachine();
	this->initPosition = position;
	this->block = gameObj;
	this->fireDir = direction;
	this->fireForce = force;
	this->fireDist = distance;
	fired = false;

	State* cocked = new State([&](float dt)-> void
		{
			return;
		}
	);
	State* released = new State([&](float dt)-> void
		{
			if ((block->GetTransform().GetPosition() - initPosition).Length() > Vector3(0.1, 0.1, 0.1).Length() 
				|| (block->GetTransform().GetPosition() - initPosition).Length() < Vector3(-0.1, -0.1, -0.1).Length()) {
				block->GetPhysicsObject()->AddForceAtPosition(fireForce, fireDir);
			}
		}
	);

	stateMachine->AddState(cocked);
	stateMachine->AddState(released);

	static StateTransition* stateCookedReleased = new StateTransition(cocked, released, [this](void)->bool
		{
			return block->springFired;
		}
	);
	static StateTransition* stateReleasedCocked = new StateTransition(released, cocked, [this](void)->bool
		{
			if (block->GetTransform().GetPosition().Length() >= initPosition.Length() + fireDist)
			{
				block->springFired = false;
				block->GetPhysicsObject()->SetLinearVelocity(Vector3(0, 0, 0));
				block->GetTransform().SetPosition(initPosition);
				return true;
			}
			return false;
		}
	);

	stateMachine->AddTransition(stateCookedReleased);
	stateMachine->AddTransition(stateReleasedCocked);
}