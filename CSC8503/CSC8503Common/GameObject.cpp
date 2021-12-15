#include "GameObject.h"
#include "CollisionDetection.h"

using namespace NCL::CSC8503;

GameObject::GameObject(string objectName)	{
	name			= objectName;
	worldID			= -1;
	isActive		= true;
	boundingVolume	= nullptr;
	physicsObject	= nullptr;
	renderObject	= nullptr;
	gOType			= GameObjectType::_NULL;
}

GameObject::~GameObject()	{
	delete boundingVolume;
	delete physicsObject;
	delete renderObject;
}

bool GameObject::GetBroadphaseAABB(Vector3&outSize) const {
	if (!boundingVolume) {
		return false;
	}
	outSize = broadphaseAABB;
	return true;
}

void GameObject::UpdateBroadphaseAABB() {
	if (!boundingVolume) {
		return;
	}
	if (boundingVolume->type == VolumeType::AABB) {
		broadphaseAABB = ((AABBVolume&)*boundingVolume).GetHalfDimensions();
	}
	else if (boundingVolume->type == VolumeType::Sphere) {
		float r = ((SphereVolume&)*boundingVolume).GetRadius();
		broadphaseAABB = Vector3(r, r, r);
	}
	else if (boundingVolume->type == VolumeType::OBB) {
		Matrix3 mat = Matrix3(transform.GetOrientation());
		mat = mat.Absolute();
		Vector3 halfSizes = ((OBBVolume&)*boundingVolume).GetHalfDimensions();
		broadphaseAABB = mat * halfSizes;
	}
}

void GameObject::SetColour() {
	switch (gOType)
	{
	case GameObjectType::_FLOOR:
		this->GetRenderObject()->SetColour(Vector4(1, 0, 0, 1));
		break;
	case GameObjectType::_RAMP:
		this->GetRenderObject()->SetColour(Vector4(0.5, 0, 0, 1));
		break;
	case GameObjectType::_RESET:
		this->GetRenderObject()->SetColour(Vector4(0, 0, 0, 1));
		break;
	case GameObjectType::_SLIME:
		this->GetRenderObject()->SetColour(Vector4(0, 1, 0, 1));
		break;
	case GameObjectType::_SPRING:
		this->GetRenderObject()->SetColour(Vector4(1, 0, 1, 1));
		break;
	case GameObjectType::_WALL:
		this->GetRenderObject()->SetColour(Vector4(0, 0, 1, 1));
		break;
	default:
		break;
	}
}