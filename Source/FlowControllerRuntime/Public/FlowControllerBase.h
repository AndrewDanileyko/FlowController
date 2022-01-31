#pragma once

#include "CoreMinimal.h"
#include "../Private/FlowItemBase.h"
#include <Runtime/AIModule/Classes/BehaviorTree/BlackboardComponent.h>
#include "FlowControllerBase.generated.h"

UCLASS(Blueprintable, BlueprintType)
class FLOWCONTROLLERRUNTIME_API UFlowControllerBase : public UFlowItemBase
{
	GENERATED_BODY()
public:

	BEGIN_PINS
		INPUT_PIN(Enter)
		OUTPUT_PIN(Done)
	END_PINS

	UPROPERTY()
	UObject* FlowExchangerObject { nullptr };

	virtual void StartController(UObject* ExchangerObject, FName InputName, FOnFlowItemFinishedDelegate const& StateFinishedCallback);
	virtual void EnterChild(UFlowItemBase *Child);
	virtual void RequestExit(FName ExitName) override;

	UPROPERTY()
	TArray<UFlowItemBase*> Children;

	UFUNCTION(BlueprintImplementableEvent)
	void OnFlowControllerEnter(FName EnterName);

	UFUNCTION()
	void OnChildFinishedNotify(UFlowItemBase* Child);
};

