// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "../Private/FlowItemBase.h"
#include "FlowStateBase.generated.h"

UCLASS()
class FLOWCONTROLLERRUNTIME_API UFlowStateBase : public UFlowItemBase {
	GENERATED_BODY()
public:

	BEGIN_PINS
		INPUT_PIN(Exec)
	END_PINS

private:
};
