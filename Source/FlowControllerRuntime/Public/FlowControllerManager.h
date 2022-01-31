#pragma once

#include "CoreMinimal.h"
#include "../Private/FlowItemBase.h"
#include "FlowStateBase.h"
#include "FlowControllerBase.h"
#include "FlowControllerManager.generated.h"

USTRUCT()
struct FItemExitRequest {
	GENERATED_BODY()
	FName ExitName;
	UFlowItemBase* FlowItem;
};

UCLASS()
class FLOWCONTROLLERRUNTIME_API UFlowControllersManager : public UObject, public FTickableGameObject {
	GENERATED_BODY()
public:
	static UFlowControllersManager* SharedInstance();

	// Tick override group
	bool IsTickable() const override { return bShouldTick; }
	void Tick(float DeltaTime) override;
	TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(FConversationEditorTickHelper, STATGROUP_Tickables); }
	
	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	static void StateEnter(FName EnterName, TSubclassOf<UFlowStateBase> StateClass, FString ItemID, UFlowControllerBase* Parent, FOnFlowItemFinishedDelegate const& StateFinishedCallback);
	
	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	static void StartController(FName EnterName, UObject* FlowExchangerObject, TSubclassOf<UFlowControllerBase> ControllerClass, FString ItemID, UFlowControllerBase* Parent, FOnFlowItemFinishedDelegate const& ControllerFinishedCallback);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	static void BreakController(FString ControllerID);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	static void RequestExit(FName ExitName, UFlowItemBase* FlowItem);

private:

	bool bShouldTick{ false };

	UPROPERTY()
	TMap<FString, UFlowControllerBase*> Controllers;

	UPROPERTY()
	TArray<FItemExitRequest> ExitRequests;
};

