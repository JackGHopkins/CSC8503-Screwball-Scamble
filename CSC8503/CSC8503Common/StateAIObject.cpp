#include "StateAIObject.h"
#include "StateTransition.h"
#include "StateMachine.h"
#include "State.h"

using namespace NCL;
using namespace CSC8503;

StateAIObject::StateAIObject() {
	speed = 1.0f;
	stateMachine = new StateMachine();
	targetPos = GetTransform().GetPosition();

	State* huntPlayer = new State([&](float dt)-> void
		{
			this->MoveToBall(dt);
		}
	);
	State* avoidPlayer = new State([&](float dt)-> void
		{
			this->MoveFromBall(dt);
		}
	);
	State* huntCoins = new State([&](float dt)-> void
		{
			this->HuntCoin(dt);
		}
	);
	State* avoidBumpers = new State([&](float dt)-> void
		{
			this->AvoidBumper(dt);
		}
	);

	stateMachine->AddState(huntPlayer);
	stateMachine->AddState(avoidPlayer);
	stateMachine->AddState(huntCoins);
	stateMachine->AddState(avoidBumpers);

	stateMachine->AddTransition(new StateTransition(huntPlayer, huntCoins,
		[&]()-> bool
		{
			return false;
		}
	));
	stateMachine->AddTransition(new StateTransition(huntCoins, huntPlayer,
		[&]()-> bool
		{
			if (currentLength == (GetTransform().GetPosition() - targetPos).Length())
				return true;
			//coinPos = targetPos;
			return false;
		}
	));

	stateMachine->AddTransition(new StateTransition(huntCoins, avoidBumpers,
		[&]()-> bool
		{
			float currentLength;
			for (auto i : coins) {
				for (auto j : bumpers) {
					if ((i->GetTransform().GetPosition() - j->GetTransform().GetPosition()).Length() > (i->GetTransform().GetPosition() - j->GetTransform().GetPosition()).Length()) {
						coinPos = i->GetTransform().GetPosition() - GetTransform().GetPosition();
						return false;
					}
				}
				return true;
			}
		}
	));
}

StateAIObject ::~StateAIObject() {
	delete stateMachine;
}

 void StateAIObject::Update(float dt) {
	 stateMachine->Update(dt);
	 currentLength = (GetTransform().GetPosition() - targetPos).Length();
	 for (auto i : coins) {
		 if ((GetTransform().GetPosition() - i->GetTransform().GetPosition()).Length() < currentLength) {
			 currentLength = (GetTransform().GetPosition() - i->GetTransform().GetPosition()).Length();
			 coinPos = i->GetTransform().GetPosition();
		 }
	 }
}

 void StateAIObject::MoveFromBall(float dt) {
	 GetPhysicsObject()->AddForce((GetTransform().GetPosition() - targetPos) * speed);
 }

 void StateAIObject::MoveToBall(float dt) {
	 GetPhysicsObject()->AddForce((targetPos - GetTransform().GetPosition())* speed);
 }

 void StateAIObject::HuntCoin(float dt) {
	 GetPhysicsObject()->AddForce((coinPos - GetTransform().GetPosition())* speed);
 }

 void StateAIObject::AvoidBumper(float dt) {
	 GetPhysicsObject()->AddForce((coinPos - GetTransform().GetPosition())* speed);
 }