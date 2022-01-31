// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "FlowStateBase.h"
#include "Editor/BlueprintGraph/Classes/K2Node.h"
#include "Editor/BlueprintGraph/Public/BlueprintActionDatabaseRegistrar.h"
#include "FlowStateNode.generated.h"

class UFlowStateBase;

UCLASS()
class UFlowStateNode : public UK2Node
{
	GENERATED_BODY()
public:

	void AllocateDefaultPins() override;

	bool IsNodeSafeToIgnore() const override { return true; }

	void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;

	FText GetMenuCategory() const override { return FText::FromString(TEXT("FlowController")); }

	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;

	void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;

	void PinDefaultValueChanged(UEdGraphPin* Pin) override;

	bool CanPasteHere(const UEdGraph* TargetGraph) const override;

	bool IsCompatibleWithGraph(UEdGraph const* Graph) const override;

	bool ShouldShowNodeProperties() const override { return true; }

	FString GetFindReferenceSearchString() const override;

	UPROPERTY(EditAnywhere)
	bool bDefaultNextPin{ true };

	UPROPERTY(EditAnywhere)
	bool bDefaultBackPin{ false };

private:

	UPROPERTY()
	FString StateClassName;

	bool CheckForErrors(FKismetCompilerContext& CompilerContext);

	void GetPins(TSubclassOf<UFlowStateBase> FlowClass, TArray<FName>& Names, EPinDirection Direction);

	void PostEditChangeProperty(struct FPropertyChangedEvent& e) override;

};