#pragma once

#include "Saturn/Core/Timestep.h"
#include "Saturn/GameFramework/Core/GameScript.h"
#include "Saturn/Scene/Entity.h"

#include "__FILE_NAME__.Gen.h"

using namespace Saturn;

SCLASS( Spawnable, VisibleInEditor )
class __FILE_NAME__ : public Entity
{
	GENERATED_BODY()
public:
	__FILE_NAME__();
	~__FILE_NAME__();

	virtual void BeginPlay() override;
	virtual void OnUpdate( Saturn::Timestep ts ) override;
	virtual void OnPhysicsUpdate( Saturn::Timestep ts ) override;

private:

};
