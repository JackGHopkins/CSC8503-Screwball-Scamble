#pragma once
#include "GameTechRenderer.h"
#include "../CSC8503Common/PhysicsSystem.h"
#include "../CSC8503Common/StateGameObject.h"
#include "../CSC8503Common/SMPushBlock.h"

namespace NCL {
	namespace CSC8503 {
		enum class Gamemode {
			_GM1 = 0,
			_GM2
		};

		enum class FinishState {
			_NULL = 0,
			_LOSE,
			_WIN
		};

		class TutorialGame		{
		public:
			TutorialGame(bool gm1);
			~TutorialGame();

			virtual void UpdateGame(float dt);

			float GetTimer() { return timer; }
			float GetScore() { return score; }
			GameTechRenderer* GetRenderer() { return renderer; }
			void ResetRenderer();
			void PrintPause();
			void PrintWin();
			void PrintLose();

			FinishState fState;
			Gamemode gMode;
		protected:
			void InitialiseAssets();

			void InitCamera();
			void UpdateKeys(float dt);
			void UpdateObjectState(float dt);
			void UpdateTimer(float dt);

			void InitWorld();

			void InitGameExamples();

			void InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius);
			void InitMixedGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing);
			void InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims);
			void InitCollisionTest();
			void InitDefaultFloor();
			void InitGamemode1();
			void InitGamemode2();
			void BridgeConstraintTest();
	
			bool SelectObject(float dt);
			void MoveSelectedObject();
			void DebugObjectMovement();
			void DebugMenu();
			void DebugObject();
			void LockedObjectMovement();

			GameObject* AddFloorToWorld(const Vector3& position);
			GameObject* AddSphereToWorld(const Vector3& position, float radius, float inverseMass = 10.0f, GameObjectType type = GameObjectType::_NULL);
			GameObject* AddCubeToWorld(const Vector3& position, Vector3 dimensions, Quaternion& orientation = Quaternion(0,0,0,0),
				float inverseMass = 10.0f, GameObjectType type = GameObjectType::_NULL);
			GameObject* AddCapsuleToWorld(const Vector3& position, float halfheight, float radius, Quaternion& orientation = Quaternion(0, 0, 0, 0),
				float inverseMass = 10.0f, GameObjectType type = GameObjectType::_NULL);
			SMPushBlock* AddSpringBlockToWorld(const Vector3& position, Vector3 dimensions, Quaternion& orientation = Quaternion(0, 0, 0, 0), 
				float inverseMass = 10.0f, Vector3 force = Vector3(0,100,0), float distance = 1.0f);
			SMPushBlock* testPushBlock;

			GameObject* AddCapsuleToWorld(const Vector3& position, float halfHeight, float radius, float inverseMass = 10.0f);

			GameObject* AddPlayerToWorld(const Vector3& position);
			GameObject* AddEnemyToWorld(const Vector3& position);
			GameObject* AddBonusToWorld(const Vector3& position);
			StateGameObject* AddStateObjectToWorld(const Vector3& position);

			GameObject* fallingLog;
			GameObject* ball;
			StateGameObject* testStateObject;

			GameTechRenderer*	renderer;
			PhysicsSystem*		physics;
			GameWorld*			world;

			bool		useGravity;
			bool		inSelectionMode;
			bool		debugMenu;
			bool		debugObject;
			bool		finished;
			bool		addScore;

			float		forceMagnitude;
			float		timer;
			int			score;

			GameObject* selectionObject = nullptr;
			std::vector<GameObject*> coins;
			std::vector<SMPushBlock*> vSprings;

			OGLMesh*	capsuleMesh = nullptr;
			OGLMesh*	cubeMesh	= nullptr;
			OGLMesh*	sphereMesh	= nullptr;
			OGLTexture* basicTex	= nullptr;
			OGLShader*	basicShader = nullptr;

			//Coursework Meshes
			OGLMesh*	charMeshA	= nullptr;
			OGLMesh*	charMeshB	= nullptr;
			OGLMesh*	enemyMesh	= nullptr;
			OGLMesh*	bonusMesh	= nullptr;

			//Coursework Additional functionality	
			GameObject* lockedObject	= nullptr;
			Vector3 lockedOffset		= Vector3(0, 14, 20);
			void LockCameraToObject(GameObject* o) {
				lockedObject = o;
			}
		};
	}
}

