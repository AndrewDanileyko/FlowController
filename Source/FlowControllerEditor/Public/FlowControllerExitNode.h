// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Editor/BlueprintGraph/Classes/K2Node.h"
#include "Editor/BlueprintGraph/Public/BlueprintActionDatabaseRegistrar.h"
#include "FlowControllerExitNode.generated.h"

UCLASS()
class UFlowControllerExitNode : public UK2Node
{
	GENERATED_BODY()
public:

	void AllocateDefaultPins() override;

	bool IsNodeSafeToIgnore() const override { return true; }

	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;

	void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;

	bool CanUserDeleteNode() const override { return false; };

};