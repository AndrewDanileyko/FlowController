// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "FlowItemBase.generated.h"

// Input pin macro wrapper

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnFlowItemFinishedDelegate, FName, ExitPointName);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnFlowItemFinishedNotifyParent, class UFlowItemBase*, Child);

const FName ParentExit = FName(TEXT("System.Parent.Exit"));
const FName ItemBroken = FName(TEXT("Broken"));
const FName ItemTerminate = FName(TEXT("System.Terminate"));

enum EPinDirection {
	In,
	Out
};

#define BEGIN_PINS void GetPins(TArray<FName>& Names, EPinDirection Direction) { Super::GetPins(Names, Direction);

#define INPUT_PIN(x) if (Direction == EPinDirection::In) {	Names.Add(FName(TEXT(#x))); }

#define OUTPUT_PIN(x) if (Direction == EPinDirection::Out) { Names.Add(FName(TEXT(#x))); }

#define END_PINS };

UCLASS()
class FLOWCONTROLLERRUNTIME_API UFlowItemBase : public UObject, public FTickableGameObject {
	GENERATED_BODY()
public:

	FOnFlowItemFinishedDelegate FinishedCallback;
	FOnFlowItemFinishedNotifyParent FinishedCallbackParentNotify;

	~UFlowItemBase();

	UPROPERTY()
	FString ItemID;	

	UPROPERTY()
	UObject* ContextFlowExchanger { nullptr };	
	
	// Tick override group
	virtual bool IsTickable() const override { return bShouldTick; }
	void Tick(float DeltaTime) override {};
	TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(FConversationEditorTickHelper, STATGROUP_Tickables); }

	virtual void GetPins(TArray<FName>& Names, EPinDirection Direction) {};

	void Enter(FName InputName, FOnFlowItemFinishedDelegate const& StateFinishedCallback);
	void Exit(FName ExitName);
	virtual void DidEnter(FName InputName) {};
	
	UFUNCTION(BlueprintCallable)
	virtual void RequestExit(FName ExitName);
	
	virtual void PrepareToExit(FName ExitName);
	virtual void UnimplementedExitName(FName ExitName) {UE_LOG(LogTemp, Warning, TEXT("Unimplemented exit name for class %s"), *this->GetClass()->GetName())};

private:

	bool bShouldTick{ false };
};
