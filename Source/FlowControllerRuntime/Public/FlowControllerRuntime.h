// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "FlowStateBase.h"

class FFlowControllerRuntime : public IModuleInterface
{
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	virtual bool IsGameModule() const override { return true; };
	void OnGameInit();
};