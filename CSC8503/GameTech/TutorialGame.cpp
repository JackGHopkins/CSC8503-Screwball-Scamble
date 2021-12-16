#include "TutorialGame.h"
#include "../CSC8503Common/GameWorld.h"
#include "../../Plugins/OpenGLRendering/OGLMesh.h"
#include "../../Plugins/OpenGLRendering/OGLShader.h"
#include "../../Plugins/OpenGLRendering/OGLTexture.h"
#include "../../Common/TextureLoader.h"
#include "..//CSC8503Common/PositionConstraint.h"

using namespace NCL;
using namespace CSC8503;

TutorialGame::TutorialGame()	{
	world		= new GameWorld();
	renderer	= new GameTechRenderer(*world);
	physics		= new PhysicsSystem(*world);

	forceMagnitude	= 10.0f;
	useGravity		= false;
	inSelectionMode = true;
	debugMenu		= false;
	debugObject		= false;
	finished		= false;
	timer			= 0.0f;
	score			= 0;

	Debug::SetRenderer(renderer);

	InitialiseAssets();
}

/*

Each of the little demo scenarios used in the game uses the same 2 meshes, 
and the same texture and shader. There's no need to ever load in anything else
for this module, even in the coursework, but you can add it if you like!

*/
void TutorialGame::InitialiseAssets() {
	auto loadFunc = [](const string& name, OGLMesh** into) {
		*into = new OGLMesh(name);
		(*into)->SetPrimitiveType(GeometryPrimitive::Triangles);
		(*into)->UploadToGPU();
	};

	loadFunc("cube.msh"		 , &cubeMesh);
	loadFunc("sphere.msh"	 , &sphereMesh);
	loadFunc("Male1.msh"	 , &charMeshA);
	loadFunc("courier.msh"	 , &charMeshB);
	loadFunc("security.msh"	 , &enemyMesh);
	loadFunc("coin.msh"		 , &bonusMesh);
	loadFunc("capsule.msh"	 , &capsuleMesh);

	basicTex	= (OGLTexture*)TextureLoader::LoadAPITexture("checkerboard.png");
	basicShader = new OGLShader("GameTechVert.glsl", "GameTechFrag.glsl");

	InitCamera();
	InitWorld();
}

TutorialGame::~TutorialGame()	{
	delete cubeMesh;
	delete sphereMesh;
	delete charMeshA;
	delete charMeshB;
	delete enemyMesh;
	delete bonusMesh;

	delete basicTex;
	delete basicShader;

	delete physics;
	delete renderer;
	delete world;
}

void TutorialGame::UpdateGame(float dt) {
	if (!finished)
		timer += dt;
	renderer->DrawString("Time:" + std::to_string(round(int(timer * 100)) / 100), Vector2(70, 5));
	renderer->DrawString("Score:" + std::to_string(round(score)), Vector2(70, 10));

	if (finished) {
		world->ClearForces();
		useGravity = false;
	}

	if (!inSelectionMode) {
		world->GetMainCamera()->UpdateCamera(dt);
	}

	UpdateKeys(dt);

	if (useGravity) {
		Debug::Print("(G)ravity on", Vector2(5, 95));
	}
	else {
		Debug::Print("(G)ravity off", Vector2(5, 95));
	}

	SelectObject(dt);
	MoveSelectedObject();
	physics->Update(dt);

	if (lockedObject != nullptr) {
		Vector3 objPos = lockedObject->GetTransform().GetPosition();
		Vector3 camPos = objPos + lockedOffset;

		Matrix4 temp = Matrix4::BuildViewMatrix(camPos, objPos, Vector3(0,1,0));

		Matrix4 modelMat = temp.Inverse();

		Quaternion q(modelMat);
		Vector3 angles = q.ToEuler(); //nearly there now!

		world->GetMainCamera()->SetPosition(camPos);
		world->GetMainCamera()->SetPitch(angles.x);
		world->GetMainCamera()->SetYaw(angles.y);

		//Debug::DrawAxisLines(lockedObject->GetTransform().GetMatrix(), 2.0f);
	}

	if (testStateObject)
		testStateObject->Update(dt);

	if (debugMenu)
		DebugMenu();

	if (debugObject)
		DebugObject();

	for (auto i : vSprings) {
		if (i)
			i->Update(dt);
	}

	UpdateObjectState(dt);
	
	world->UpdateWorld(dt);
	renderer->Update(dt);

	Debug::FlushRenderables(dt);
	renderer->Render();

}

void TutorialGame::UpdateKeys(float dt) {
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F1)) {
		InitWorld(); //We can reset the simulation at any time with F1
		selectionObject = nullptr;
		lockedObject	= nullptr;
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F2)) {
		InitCamera(); //F2 will reset the camera to a specific default place
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::G)) {
		useGravity = !useGravity; //Toggle gravity!
		physics->UseGravity(useGravity);
	}
	//Running certain physics updates in a consistent order might cause some
	//bias in the calculations - the same objects might keep 'winning' the constraint
	//allowing the other one to stretch too much etc. Shuffling the order so that it
	//is random every frame can help reduce such bias.
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F9)) {
		world->ShuffleConstraints(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F10)) {
		world->ShuffleConstraints(false);
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F7)) {
		world->ShuffleObjects(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F8)) {
		world->ShuffleObjects(false);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F3)) {
		debugMenu = !debugMenu;
	}


	if (lockedObject) {
		LockedObjectMovement();
	}
	else {
		DebugObjectMovement();
	}

	// Reset 
	if (Window::GetKeyboard()->KeyPressed(NCL::KeyboardKeys::R)) {
		ball->GetPhysicsObject()->SetLinearVelocity(Vector3(0, 0, 0));
		ball->GetTransform().SetPosition(Vector3(16, 2, 16));
		world->GetMainCamera()->SetNearPlane(0.1f);
		world->GetMainCamera()->SetFarPlane(500.0f);
		world->GetMainCamera()->SetPitch(-90.0f);
		world->GetMainCamera()->SetYaw(0.0f);
		world->GetMainCamera()->SetPosition(Vector3(-0, 65, 0));
	}

	if (Window::GetKeyboard()->KeyPressed(NCL::KeyboardKeys::M)) {
		for (auto i : vSprings) {
			if (i) {
				i->springFired = true;
				i->Update(dt);
			}
		}
	}
}

void TutorialGame::LockedObjectMovement() {
	Matrix4 view		= world->GetMainCamera()->BuildViewMatrix();
	Matrix4 camWorld	= view.Inverse();

	Vector3 rightAxis = Vector3(camWorld.GetColumn(0)); //view is inverse of model!

	//forward is more tricky -  camera forward is 'into' the screen...
	//so we can take a guess, and use the cross of straight up, and
	//the right axis, to hopefully get a vector that's good enough!

	Vector3 fwdAxis = Vector3::Cross(Vector3(0, 1, 0), rightAxis);
	fwdAxis.y = 0.0f;
	fwdAxis.Normalise();

	Vector3 charForward  = lockedObject->GetTransform().GetOrientation() * Vector3(0, 0, 1);
	Vector3 charForward2 = lockedObject->GetTransform().GetOrientation() * Vector3(0, 0, 1);

	float force = 100.0f;

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {
		lockedObject->GetPhysicsObject()->AddForce(-rightAxis * force);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
		Vector3 worldPos = selectionObject->GetTransform().GetPosition();
		lockedObject->GetPhysicsObject()->AddForce(rightAxis * force);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
		lockedObject->GetPhysicsObject()->AddForce(fwdAxis * force);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
		lockedObject->GetPhysicsObject()->AddForce(-fwdAxis * force);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NEXT)) {
		lockedObject->GetPhysicsObject()->AddForce(Vector3(0,-10,0));
	}
}

void TutorialGame::DebugObjectMovement() {
//If we've selected an object, we can manipulate it with some key presses
	if (inSelectionMode && selectionObject) {
		//Twist the selected object!
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(-10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM7)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, 10, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM8)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, -10, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, -10));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, 10));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM5)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, -10, 0));
		}
	}

}

void TutorialGame::InitCamera() {
	world->GetMainCamera()->SetNearPlane(0.1f);
	world->GetMainCamera()->SetFarPlane(500.0f);
	world->GetMainCamera()->SetPitch(-90.0f);
	world->GetMainCamera()->SetYaw(0.0f);
	world->GetMainCamera()->SetPosition(Vector3(-0, 65, 0));
	lockedObject = nullptr;
}

void TutorialGame::InitWorld() {
	world->ClearAndErase();
	physics->Clear();

	//InitMixedGridWorld(5, 5, 3.5f, 3.5f);
	//InitGameExamples();
	//InitDefaultFloor();
	//BridgeConstraintTest();
	//testStateObject = AddStateObjectToWorld(Vector3(0, 10, 0));
	InitGamemode1();
	//InitCollisionTest();
}

void TutorialGame::InitCollisionTest() {
	float sphereRadius = 1.0f;
	Vector3 cubeDims = Vector3(1, 1, 1);
	AddCubeToWorld(Vector3(-10,5,-5), cubeDims, Quaternion(0, 0, 0, 1), 1.0f);
	AddCubeToWorld(Vector3(-5,5,-5), cubeDims, Quaternion(0, 0, 0.5, 1), 0.0f);
	fallingLog = AddCapsuleToWorld(Vector3(0, 12, 0), 2.0f, 0.4f, Quaternion::EulerAnglesToQuaternion(0.0f, 0.0f, 90.0f), 0.001f);
	AddCubeToWorld(Vector3(0, 6, 2), Vector3(4, 1, 6.5), Quaternion(0.315, 0, 0, 1), 0, GameObjectType::_RAMP);
	//AddCubeToWorld(Vector3(13.5, 3.5, 0), Vector3(4.5, 1, 6.5), Quaternion(0.12, 0, 0, 1), 0, GameObjectType::_RAMP);

	AddSphereToWorld(Vector3(-20,5,-20), sphereRadius, 1.0f);
	//AddSphereToWorld(Vector3(-3,20,-5), sphereRadius, 1.0f);
	vSprings.emplace_back(AddSpringBlockToWorld(Vector3(19, 2, 16), Vector3(1, 1, 2), Quaternion(0, 0, 0, 1), 1.0f, Vector3(-250, 0, 0), 3.0f));
}

void TutorialGame::InitGamemode1(){
	float magNum = 2.0f;

	// Ball
	ball = AddSphereToWorld(Vector3(16, 2, 16), 0.5f, 1.0f);

	// Log
	fallingLog = AddCapsuleToWorld(Vector3(0, 20, 0), 2.0f, 0.4f, Quaternion::EulerAnglesToQuaternion(0.0f, 0.0f, 90.0f), 0.001f, GameObjectType::_LOG);

	// Reset Collider
	AddCubeToWorld(Vector3(0, -5, 0), Vector3(20, 0.5, 20), Quaternion(0, 0, 0, 1), 0, GameObjectType::_RESET);
	
	// Base Floor
	AddCubeToWorld(Vector3(16, 0, 16), Vector3(5, 1, 2), Quaternion(0, 0, 0, 1), 0, GameObjectType::_FLOOR);

	// 1st Floor
	AddCubeToWorld(Vector3(-15, 2, 13), Vector3(5, 1, 5), Quaternion(0, 0, 0, 1), 0, GameObjectType::_FLOOR);
	AddCubeToWorld(Vector3(0, 2, 9), Vector3(11, 1, 1), Quaternion(0, 0, 0, 1), 0, GameObjectType::_FLOOR);
	AddCubeToWorld(Vector3(14, 2, 9), Vector3(4, 1, 3), Quaternion(0, 0, 0, 1), 0, GameObjectType::_FLOOR);

	// GOAL
	AddCubeToWorld(Vector3(13.5, 3, -16), Vector3(4.5, 1, 2), Quaternion(0, 0, 0, 1), 0, GameObjectType::_GOAL);


	// Walls
	AddCubeToWorld(Vector3(0, 2, 19), Vector3(20, 4, 1), Quaternion(0, 0, 0, 1), 0, GameObjectType::_WALL);
	AddCubeToWorld(Vector3(3, 2, 13), Vector3(17, 4, 1), Quaternion(0, 0, 0, 1), 0, GameObjectType::_WALL);
	AddCubeToWorld(Vector3(-18, 4, 17.36), Vector3(2, 2, 1), Quaternion(0, -0.414, 0, 1), 0, GameObjectType::_WALL);// Tilted Corner

	AddCubeToWorld(Vector3(19, 2, -3), Vector3(1, 8, 15), Quaternion(0, 0, 0, 1), 0, GameObjectType::_WALL);
	AddCubeToWorld(Vector3(6.5, 2, -5), Vector3(2.5, 8, 13), Quaternion(0, 0, 0, 1), 0, GameObjectType::_WALL);
	AddCubeToWorld(Vector3(12, 2, -19), Vector3(8,8, 1), Quaternion(0, 0, 0, 1), 0, GameObjectType::_WALL);

	AddCubeToWorld(Vector3(-11.3334, 4, 11.9), Vector3(2, 2, 1), Quaternion(0, 0.268, 0, 1), 0, GameObjectType::_WALL); // Tilted Corner
	AddCubeToWorld(Vector3(-7, 2, 11), Vector3(3, 4, 1), Quaternion(0, 0, 0, 1), 0, GameObjectType::_WALL);
	AddCubeToWorld(Vector3(8, 2, 11), Vector3(4, 4, 1), Quaternion(0, 0, 0, 1), 0, GameObjectType::_WALL);

	AddCubeToWorld(Vector3(-19, 2, 15), Vector3(1, 4, 3), Quaternion(0, 0, 0, 1), 0, GameObjectType::_WALL);
	AddCubeToWorld(Vector3(10.5, 2, 6), Vector3(1.5, 4, 2), Quaternion(0, 0, 0, 1), 0, GameObjectType::_WALL);
	AddCubeToWorld(Vector3(-12, 2, 7), Vector3(8, 6, 1), Quaternion(0, 0, 0, 1), 0, GameObjectType::_WALL);

	// Springs
	vSprings.emplace_back(AddSpringBlockToWorld(Vector3(19, 2, 16), Vector3(1,1,2), Quaternion(0, 0, 0, 1), 1.0f, Vector3(-250, 0, 0) ,3.0f));
	vSprings.emplace_back(AddSpringBlockToWorld(Vector3(-19, 4, 10), Vector3(1,1,2), Quaternion(0, 0, 0, 1), 1.0f, Vector3(300, 0, 0), 2.5f));
	vSprings.emplace_back(AddSpringBlockToWorld(Vector3(15, 4, 11), Vector3(3,1,1), Quaternion(0, 0, 0, 1), 1.0f, Vector3(0, 0, -300), 2.0f));

	// Ramps
	AddCubeToWorld(Vector3(0, 1, 16), Vector3(11, 1, 2), Quaternion::EulerAnglesToQuaternion(0.0f, 0.0f, -5.8f), 0, GameObjectType::_RAMP);
	AddCubeToWorld(Vector3(0, 7, 2), Vector3(4, 1, 6.5), Quaternion(0.315, 0, 0, 1), 0, GameObjectType::_RAMP);
	AddCubeToWorld(Vector3(13.5, 3.5, 0), Vector3(4.5, 1, 6.5), Quaternion(0.12, 0, 0, 1), 0, GameObjectType::_RAMP);

	// Slime
	AddCubeToWorld(Vector3(9.54, 2, 2), Vector3(1, 3.5, 3), Quaternion::EulerAnglesToQuaternion(0.0f, 45.0f, 0.0f), 0, GameObjectType::_SLIME);
	AddCubeToWorld(Vector3(16.66, 2, 0), Vector3(1, 3.5, 3), Quaternion::EulerAnglesToQuaternion(0.0f, 45.0f, 0.0f), 0, GameObjectType::_SLIME);
	AddCubeToWorld(Vector3(16.66, 2, -2.8), Vector3(1, 3.5, 3), Quaternion::EulerAnglesToQuaternion(0.0f, -45.0f, 0.0f), 0, GameObjectType::_SLIME);

	// Button
	AddCubeToWorld(Vector3(-16, 2, -16), Vector3(2, 1, 2), Quaternion(0,0,0,1), 0, GameObjectType::_BUTTON_SPRING);

	// Coins
	coins.emplace_back(AddSphereToWorld(Vector3(-12, 4, 16), 1.0f, 1.0f, GameObjectType::_COIN));
	coins.emplace_back(AddSphereToWorld(Vector3(10, 4, 9), 1.0f, 1.0f, GameObjectType::_COIN));
	coins.emplace_back(AddSphereToWorld(Vector3(-14, 4, 9), 1.0f, 1.0f, GameObjectType::_COIN));
}

//void TutorialGame::BridgeConstraintTest() {
//	Vector3 cubeSize = Vector3(8, 8, 8);
//	
//	float invCubeMass = 5; //how heavy the middle pieces are
//	int numLinks = 10;
//	float maxDistance = 30; // constraint distance
//	float cubeDistance = 20; // distance between links
//	
//	Vector3 startPos = Vector3(500, 500, 500);
//	
//	 GameObject * start = AddCubeToWorld(startPos + Vector3(0, 0, 0), cubeSize, Quaternion(0, 0, 0, 0), 0);
//	 GameObject * end = AddCubeToWorld(startPos + Vector3((numLinks + 2)* cubeDistance, 0, 0), cubeSize, Quaternion(0, 0, 0, 0), 0);
//	
//	 GameObject * previous = start;
//	
//	 for (int i = 0; i < numLinks; ++i) {
//	 GameObject * block = AddCubeToWorld(startPos + Vector3((i + 1) *cubeDistance, 0, 0), cubeSize, Quaternion(0, 0, 0, 0), invCubeMass);
//	 PositionConstraint * constraint = new PositionConstraint(previous,block, maxDistance);
//	 world->AddConstraint(constraint);
//	 previous = block;
//	}
//	PositionConstraint * constraint = new PositionConstraint(previous,end, maxDistance);
//	world->AddConstraint(constraint);
//}

/*

A single function to add a large immoveable cube to the bottom of our world

*/
GameObject* TutorialGame::AddFloorToWorld(const Vector3& position) {
	GameObject* floor = new GameObject();
	std::string name = "floor";
	floor->SetName(name);

	Vector3 floorSize	= Vector3(100, 2, 100);
	AABBVolume* volume	= new AABBVolume(floorSize);
	floor->SetBoundingVolume((CollisionVolume*)volume);
	floor->GetTransform()
		.SetScale(floorSize * 2)
		.SetPosition(position);

	floor->SetRenderObject(new RenderObject(&floor->GetTransform(), cubeMesh, basicTex, basicShader));
	floor->SetPhysicsObject(new PhysicsObject(&floor->GetTransform(), floor->GetBoundingVolume()));

	floor->GetPhysicsObject()->SetInverseMass(0);
	floor->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(floor);

	return floor;
}

/*

Builds a game object that uses a sphere mesh for its graphics, and a bounding sphere for its
rigid body representation. This and the cube function will let you build a lot of 'simple' 
physics worlds. You'll probably need another function for the creation of OBB cubes too.

*/
GameObject* TutorialGame::AddSphereToWorld(const Vector3& position, float radius, float inverseMass, GameObjectType type) {
	GameObject* sphere = new GameObject();
	std::string name = "sphere";
	sphere->SetName(name);

	Vector3 sphereSize = Vector3(radius, radius, radius);
	SphereVolume* volume = new SphereVolume(radius);
	sphere->SetBoundingVolume((CollisionVolume*)volume);

	sphere->GetTransform()
		.SetScale(sphereSize)
		.SetPosition(position);

	sphere->gOType = type;

	sphere->SetRenderObject(new RenderObject(&sphere->GetTransform(), sphereMesh, basicTex, basicShader));
	sphere->SetPhysicsObject(new PhysicsObject(&sphere->GetTransform(), sphere->GetBoundingVolume()));

	sphere->GetPhysicsObject()->SetInverseMass(inverseMass);
	sphere->GetPhysicsObject()->InitSphereInertia();

	sphere->InitObjType();

	world->AddGameObject(sphere);

	return sphere;
}

GameObject* TutorialGame::AddCapsuleToWorld(const Vector3& position, float halfHeight, float radius, float inverseMass) {
	GameObject* capsule = new GameObject();
	std::string name = "capsule";
	capsule->SetName(name);

	CapsuleVolume* volume = new CapsuleVolume(halfHeight, radius);
	capsule->SetBoundingVolume((CollisionVolume*)volume);

	capsule->GetTransform()
		.SetScale(Vector3(radius* 2, halfHeight, radius * 2))
		.SetPosition(position);

	capsule->SetRenderObject(new RenderObject(&capsule->GetTransform(), capsuleMesh, basicTex, basicShader));
	capsule->SetPhysicsObject(new PhysicsObject(&capsule->GetTransform(), capsule->GetBoundingVolume()));

	capsule->GetPhysicsObject()->SetInverseMass(inverseMass);
	capsule->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(capsule);

	return capsule;
}

GameObject* TutorialGame::AddCapsuleToWorld(const Vector3& position, float  halfheight, float radius, Quaternion& orientation, float inverseMass, GameObjectType type) {
	GameObject* capsule = new GameObject();
	std::string name = "capsule";
	capsule->SetName(name);

	CapsuleVolume* volume = new CapsuleVolume(halfheight, radius);
	capsule->SetBoundingVolume((CollisionVolume*)volume);

	capsule->GetTransform()
		.SetScale(Vector3(radius * 2, halfheight, radius * 2))
		.SetPosition(position)
		.SetOrientation(orientation);

	capsule->SetRenderObject(new RenderObject(&capsule->GetTransform(), capsuleMesh, basicTex, basicShader));
	capsule->SetPhysicsObject(new PhysicsObject(&capsule->GetTransform(), capsule->GetBoundingVolume()));
	capsule->GetPhysicsObject()->SetInverseMass(inverseMass);
	capsule->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(capsule);

	return capsule;
}

GameObject* TutorialGame::AddCubeToWorld(const Vector3& position, Vector3 dimensions, Quaternion& orientation, float inverseMass, GameObjectType type) {
	GameObject* cube = new GameObject();
	std::string name = "cube";
	cube->SetName(name);

	if (orientation == Quaternion(0, 0, 0, 1)) {
		AABBVolume* volume = new AABBVolume(dimensions);
		cube->SetBoundingVolume((CollisionVolume*)volume);
	}
	else {
		OBBVolume* volume = new OBBVolume(dimensions);
		cube->SetBoundingVolume((CollisionVolume*)volume);
	}

	cube->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2)
		.SetOrientation(orientation);

	cube->gOType = type;

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->InitCubeInertia();
	cube->InitObjType();

	world->AddGameObject(cube);

	return cube;
}

SMPushBlock* TutorialGame::AddSpringBlockToWorld(const Vector3& position, Vector3 dimensions, Quaternion& orientation, float inverseMass, Vector3 force, float distance) {
	SMPushBlock *spring = new SMPushBlock(position, force, distance);
	std::string name = "spring";
	spring->SetName(name);

	if (orientation == Quaternion(0, 0, 0, 1)) {
		AABBVolume* volume = new AABBVolume(dimensions);
		spring->SetBoundingVolume((CollisionVolume*)volume);
	}
	else {
		OBBVolume* volume = new OBBVolume(dimensions);
		spring->SetBoundingVolume((CollisionVolume*)volume);
	}

	spring->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2)
		.SetOrientation(orientation);

	spring->SetRenderObject(new RenderObject(&spring->GetTransform(), cubeMesh, basicTex, basicShader));
	spring->SetPhysicsObject(new PhysicsObject(&spring->GetTransform(), spring->GetBoundingVolume()));

	spring->GetPhysicsObject()->SetInverseMass(inverseMass);
	spring->GetPhysicsObject()->InitCubeInertia();

	spring->gOType = GameObjectType::_SPRING;
	spring->InitObjType();

	world->AddGameObject(spring);

	return spring;
}

void TutorialGame::InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius) {
	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			AddSphereToWorld(position, radius, 1.0f);
		}
	}
	AddFloorToWorld(Vector3(0, -2, 0));
}

void TutorialGame::InitMixedGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing) {
	float sphereRadius = 1.0f;
	Vector3 cubeDims = Vector3(1, 1, 1);

	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);

			if (rand() % 2) {
				AddCubeToWorld(position, cubeDims);
			}
			else {
				AddSphereToWorld(position, sphereRadius);
			}
		}
	}
}

void TutorialGame::InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims) {
	for (int x = 1; x < numCols+1; ++x) {
		for (int z = 1; z < numRows+1; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			AddCubeToWorld(position, cubeDims, Quaternion(0,0,0,0), 1.0f);
		}
	}
}


void TutorialGame::InitDefaultFloor() {
	AddFloorToWorld(Vector3(0, -2, 0));
}

void TutorialGame::InitGameExamples() {
	AddPlayerToWorld(Vector3(0, 5, 0));
	AddEnemyToWorld(Vector3(5, 5, 0));
	AddBonusToWorld(Vector3(10, 5, 0));
}

GameObject* TutorialGame::AddPlayerToWorld(const Vector3& position) {
	float meshSize = 3.0f;
	float inverseMass = 0.5f;

	GameObject* character = new GameObject();

	AABBVolume* volume = new AABBVolume(Vector3(0.3f, 0.85f, 0.3f) * meshSize);

	character->SetBoundingVolume((CollisionVolume*)volume);

	character->GetTransform()
		.SetScale(Vector3(meshSize, meshSize, meshSize))
		.SetPosition(position);

	if (rand() % 2) {
		character->SetRenderObject(new RenderObject(&character->GetTransform(), charMeshA, nullptr, basicShader));
	}
	else {
		character->SetRenderObject(new RenderObject(&character->GetTransform(), charMeshB, nullptr, basicShader));
	}
	character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

	character->GetPhysicsObject()->SetInverseMass(inverseMass);
	character->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(character);

	//lockedObject = character;

	return character;
}

GameObject* TutorialGame::AddEnemyToWorld(const Vector3& position) {
	float meshSize		= 3.0f;
	float inverseMass	= 0.5f;

	GameObject* character = new GameObject();

	AABBVolume* volume = new AABBVolume(Vector3(0.3f, 0.9f, 0.3f) * meshSize);
	character->SetBoundingVolume((CollisionVolume*)volume);

	character->GetTransform()
		.SetScale(Vector3(meshSize, meshSize, meshSize))
		.SetPosition(position);

	character->SetRenderObject(new RenderObject(&character->GetTransform(), enemyMesh, nullptr, basicShader));
	character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

	character->GetPhysicsObject()->SetInverseMass(inverseMass);
	character->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(character);

	return character;
}

GameObject* TutorialGame::AddBonusToWorld(const Vector3& position) {
	GameObject* apple = new GameObject();

	SphereVolume* volume = new SphereVolume(0.25f);
	apple->SetBoundingVolume((CollisionVolume*)volume);
	apple->GetTransform()
		.SetScale(Vector3(0.25, 0.25, 0.25))
		.SetPosition(position);

	apple->SetRenderObject(new RenderObject(&apple->GetTransform(), bonusMesh, nullptr, basicShader));
	apple->SetPhysicsObject(new PhysicsObject(&apple->GetTransform(), apple->GetBoundingVolume()));

	apple->GetPhysicsObject()->SetInverseMass(1.0f);
	apple->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(apple);

	return apple;
}

StateGameObject* TutorialGame::AddStateObjectToWorld(const Vector3& position) {
	StateGameObject* apple = new StateGameObject();

	SphereVolume* volume = new SphereVolume(0.25f);
	apple->SetBoundingVolume((CollisionVolume*)volume);
	apple->GetTransform()
		.SetScale(Vector3(0.25, 0.25, 0.25))
		.SetPosition(position);

	apple->SetRenderObject(new RenderObject(&apple->GetTransform(), bonusMesh, nullptr, basicShader));
	apple->SetPhysicsObject(new PhysicsObject(&apple->GetTransform(), apple->GetBoundingVolume()));

	apple->GetPhysicsObject()->SetInverseMass(1.0f);
	apple->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(apple);

	return apple;
}

/*

Every frame, this code will let you perform a raycast, to see if there's an object
underneath the cursor, and if so 'select it' into a pointer, so that it can be 
manipulated later. Pressing Q will let you toggle between this behaviour and instead
letting you move the camera around. 

*/
bool TutorialGame::SelectObject(float dt) {
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::Q)) {
		inSelectionMode = !inSelectionMode;
		if (inSelectionMode) {
			Window::GetWindow()->ShowOSPointer(true);
			Window::GetWindow()->LockMouseToWindow(false);
		}
		else {
			Window::GetWindow()->ShowOSPointer(false);
			Window::GetWindow()->LockMouseToWindow(true);
		}
	}
	if (inSelectionMode) {
		renderer->DrawString("Press Q to change to camera mode!", Vector2(5, 85));

		if (Window::GetMouse()->ButtonPressed(NCL::MouseButtons::LEFT)) {
			if (selectionObject) {	//set colour to deselected;
				selectionObject->GetRenderObject()->SetColour(Vector4(1, 1, 1, 1));

				selectionObject = nullptr;
				lockedObject = nullptr;
			}

			Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());

			RayCollision closestCollision;
			if (world->Raycast(ray, closestCollision, true)) {
				selectionObject = (GameObject*)closestCollision.node;

				if (selectionObject->gOType == GameObjectType::_BUTTON_SPRING) {
					for (auto i : vSprings) {
						if (i) {
							i->springFired = true;
							i->Update(dt);
						}
					}
					return true;
				}

				if (selectionObject) {
					debugObject = true;
				}
				selectionObject->GetRenderObject()->SetColour(Vector4(0, 1, 0, 1));
				return true;
			}
			else {
				return false;
			}
		}
	}
	else {
		renderer->DrawString("Press Q to change to select mode!", Vector2(5, 85));
	}

	if (lockedObject) {
		renderer->DrawString("Press L to unlock object!", Vector2(5, 80));
	}

	else if(selectionObject){
		renderer->DrawString("Press L to lock selected object object!", Vector2(5, 80));
	}

	if (Window::GetKeyboard()->KeyPressed(NCL::KeyboardKeys::L)) {
		if (selectionObject) {
			if (lockedObject == selectionObject) {
				lockedObject = nullptr;
			}
			else {
				lockedObject = selectionObject;
			}
		}

	}

	return false;
}

void TutorialGame::UpdateObjectState(float dt) {
	if (fallingLog) {
		if (fallingLog->gOType == GameObjectType::_RESET) {
			fallingLog->GetPhysicsObject()->SetLinearVelocity(Vector3(0,0,0));
			fallingLog->GetTransform().SetPosition(Vector3(0,20,0));
			fallingLog->gOType = GameObjectType::_LOG;
		}
	}
	if (ball) {
		if (ball->gOType == GameObjectType::_RESET) {
			ball->GetPhysicsObject()->SetLinearVelocity(Vector3(0, 0, 0));
			ball->GetTransform().SetPosition(Vector3(16, 2, 16));
			ball->gOType = GameObjectType::_NULL;
		}
		if (ball->gOType == GameObjectType::_GOAL) {
			finished = true;
		}
	}
	for (int i = 0; i < coins.size(); i++) {
		if (coins[i]->gOType == GameObjectType::_COIN_COLLECTED) {
			PlaySound(TEXT("../../Assets/Audio/coin.wav"), NULL, SND_ASYNC);
			score++;
			coins[i]->gOType = GameObjectType::_COIN;
			coins[i]->GetPhysicsObject()->SetLinearVelocity(Vector3(0, 0, 0));
			coins[i]->GetTransform().SetPosition(Vector3(-12, 2, -4 + (3*score)));
			coins.erase(coins.begin() + i);
		}
	}
}
/*
If an object has been clicked, it can be pushed with the right mouse button, by an amount
determined by the scroll wheel. In the first tutorial this won't do anything, as we haven't
added linear motion into our physics system. After the second tutorial, objects will move in a straight
line - after the third, they'll be able to twist under torque aswell.
*/
void TutorialGame::MoveSelectedObject() {
	renderer->DrawString("Click Force:" + std::to_string(forceMagnitude), Vector2(0, 40)); //Draw debug text at 10,20
	forceMagnitude += Window::GetMouse()->GetWheelMovement() * 100.0f;

	if (!selectionObject) {
		return;//we havent selected anything! 
	}
	//Push the selected object!
	if (Window::GetMouse()->ButtonPressed(NCL::MouseButtons::RIGHT)) {
		Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());
		RayCollision closestCollision;

		//if (world->Raycast(ray, closestCollision, true)) {
		//	if (closestCollision.node == selectionObject) {
		//		selectionObject->GetPhysicsObject()->AddForceAtPosition(ray.GetDirection() * forceMagnitude, closestCollision.collidedAt);
		//	}
		//}
	}

	if (Window::GetKeyboard()->KeyPressed(NCL::KeyboardKeys::SPACE)) {
		if (selectionObject->gOType == GameObjectType::_SPRING)
			selectionObject->springFired = true;
	}
}

void TutorialGame::DebugMenu() {
	renderer->DrawString("Camera - X: " + std::to_string(world->GetMainCamera()->GetPosition().x), Vector2(0, 10));
	renderer->DrawString("Camera - Y: " + std::to_string(world->GetMainCamera()->GetPosition().y), Vector2(0, 13));
	renderer->DrawString("Camera - Z: " + std::to_string(world->GetMainCamera()->GetPosition().z), Vector2(0, 16));
	renderer->DrawString("Camera - Pitch:" + std::to_string(world->GetMainCamera()->GetPitch()), Vector2(0, 20));
	renderer->DrawString("Camera - Yaw:" + std::to_string(world->GetMainCamera()->GetYaw()), Vector2(0, 30));
}

void TutorialGame::DebugObject() {
	debugMenu = false;
	renderer->DrawString("Name:" + selectionObject->GetName(), Vector2(0, 5));
	renderer->DrawString("X: " + std::to_string(selectionObject->GetTransform().GetPosition().x), Vector2(0, 10));
	renderer->DrawString("Y: " + std::to_string(selectionObject->GetTransform().GetPosition().y), Vector2(0, 13));
	renderer->DrawString("Z: " + std::to_string(selectionObject->GetTransform().GetPosition().z), Vector2(0, 16));
	renderer->DrawString("Orientation X:" + std::to_string(selectionObject->GetTransform().GetOrientation().ToEuler().x), Vector2(0, 20));
	renderer->DrawString("Orientation Y:" + std::to_string(selectionObject->GetTransform().GetOrientation().ToEuler().y), Vector2(0, 23));
	renderer->DrawString("Orientation Z:" + std::to_string(selectionObject->GetTransform().GetOrientation().ToEuler().z), Vector2(0, 26));
}


