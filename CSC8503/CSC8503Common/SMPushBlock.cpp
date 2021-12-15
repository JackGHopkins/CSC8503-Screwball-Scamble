#include "StateGameObject.h"
#include "StateTransition.h"
#include "StateMachine.h"
#include "State.h"
#include "SMPushBlock.h"

using namespace NCL;
using namespace CSC8503;

SMPushBlock::SMPushBlock(Vector3 position, Vector3 force, float distance) {
	stateMachine = new StateMachine();
	this->initPosition = position;
	this->fireForce = force;
	this->fireDist = distance;
	fired = false;

	this->gOType == GameObjectType::_SPRING;

	State* cocked = new State([&](float dt)-> void
		{
			//this->GetPhysicsObject()->SetLinearVelocity(Vector3(0, 0, 0));
			//this->GetTransform().SetPosition(initPosition);
			return;
		}
	);
	State* released = new State([&](float dt)-> void
		{
			if ((this->GetTransform().GetPosition() - initPosition).Length() > Vector3(0.1, 0.1, 0.1).Length() 
				|| (this->GetTransform().GetPosition() - initPosition).Length() < Vector3(-0.1, -0.1, -0.1).Length()) {
				this->GetPhysicsObject()->AddForce(fireForce);
			}
		}
	);

	stateMachine->AddState(cocked);
	stateMachine->AddState(released);

	StateTransition* stateCookedReleased = new StateTransition(cocked, released, [this](void)->bool
		{
			if (this->springFired) {
				this->springFired = false;
				this->initPosition = this->GetTransform().GetPosition();
				return true;
			}			
			return this->springFired;
		}
	);
	StateTransition* stateReleasedCocked = new StateTransition(released, cocked, [this](void)->bool
		{
			if ((this->GetTransform().GetPosition() - initPosition).Length() > fireDist)
			{
				this->GetPhysicsObject()->ClearForces();
				this->GetPhysicsObject()->SetLinearVelocity(Vector3(0, 0, 0));
				this->GetTransform().SetPosition(initPosition);
				return true;
			}
			return false;
		}
	);

	stateMachine->AddTransition(stateCookedReleased);
	stateMachine->AddTransition(stateReleasedCocked);
}

SMPushBlock ::~SMPushBlock() {
	delete stateMachine;
}

void SMPushBlock::Update(float dt) {
	stateMachine->Update(dt);
}