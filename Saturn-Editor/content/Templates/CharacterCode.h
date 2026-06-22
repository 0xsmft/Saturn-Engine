#pragma once

#include "Saturn/Core/Timestep.h"
#include "Saturn/GameFramework/Character.h"

#include "__FILE_NAME__.Gen.h"

using namespace Saturn;

SCLASS( Spawnable, VisibleInEditor )
class __FILE_NAME__ : public Character
{
	GENERATED_BODY()
public:
	__FILE_NAME__();
	virtual ~__FILE_NAME__();

	virtual void BeginPlay() override;
	virtual void OnUpdate( Saturn::Timestep ts ) override;
	virtual void OnPhysicsUpdate( Saturn::Timestep ts ) override;

	virtual void SetupInputBindings() override;

private:

};
