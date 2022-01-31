// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Editor/BlueprintGraph/Classes/K2Node.h"
#include "Editor/BlueprintGraph/Public/BlueprintActionDatabaseRegistrar.h"
#include "FlowControllerManager.h"
#include "FlowControllerNode.generated.h"

UCLASS()
class UFlowControllerNode : public UK2Node
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere)
	bool bBreakPin{ false };

	void AllocateDefaultPins() override;

	bool IsNodeSafeToIgnore() const override { return true; }

	void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;

	FText GetMenuCategory() const override { return FText::FromString(TEXT("FlowController")); }

	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;

	UObject* GetJumpTargetForDoubleClick() const override;

	void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;

	void PinDefaultValueChanged(UEdGraphPin* Pin) override;

	bool ShouldShowNodeProperties() const override { return true; }

	FString GetFindReferenceSearchString() const override;

private:

	UPROPERTY()
	FString ControllerClassName;

	bool CheckForErrors(FKismetCompilerContext& CompilerContext);

	void GetPins(TSubclassOf<UFlowControllerBase> ControllerClass, TArray<FName>& Names, EPinDirection Direction);

	void PostEditChangeProperty(struct FPropertyChangedEvent& e) override;
};