// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "FlowStateBase.h"
//#include "FlowControllerFactory.h"

class FFlowControllerEditor : public IModuleInterface
{
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void OnBlueprintCreated(UBlueprint* blueprint);
};