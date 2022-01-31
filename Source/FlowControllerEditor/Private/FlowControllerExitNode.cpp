// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "FlowControllerExitNode.h"
#include "Editor/BlueprintGraph/Public/BlueprintNodeSpawner.h"
#include "Editor/BlueprintGraph/Classes/EdGraphSchema_K2.h"
#include "Editor/KismetCompiler/Public/KismetCompiler.h"
#include "FlowControllerManager.h"
#include <Editor/BlueprintGraph/Classes/K2Node_CustomEvent.h>
#include <Editor/BlueprintGraph/Classes/K2Node_CallFunction.h>
#include <Editor/BlueprintGraph/Classes/K2Node_SwitchName.h>

#define LOCTEXT_NAMESPACE "FlowStateEnterNode"

#pragma optimize ("", off)

void UFlowControllerExitNode::AllocateDefaultPins() {
	Super::AllocateDefaultPins();

	UEdGraph* Graph = GetGraph();
	UBlueprint* BP = Graph->GetTypedOuter<UBlueprint>();
	auto BPClass = BP->ParentClass;

	if (BPClass->IsChildOf(UFlowControllerBase::StaticClass())) {
		UClass* Result = FindObject<UClass>(ANY_PACKAGE, *BPClass->GetName());

		TSubclassOf<UFlowControllerBase> ControllerClass = Result;

		TArray<FName> PinNames;
		ControllerClass.GetDefaultObject()->GetPins(PinNames, EPinDirection::Out);
		for (FName PinName : PinNames) {
			CreatePin(EEdGraphPinDirection::EGPD_Input, UEdGraphSchema_K2::PC_Exec, PinName);
		}
	}
}

FText UFlowControllerExitNode::GetNodeTitle(ENodeTitleType::Type TitleType) const {
	return FText::FromString(TEXT("Exit"));
}

void UFlowControllerExitNode::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) {
	Super::ExpandNode(CompilerContext, SourceGraph);

	TArray<UEdGraphPin*> ThisPins = GetAllPins();

	for (UEdGraphPin* Pin : ThisPins) {
		if (Pin->HasAnyConnections()) {

			UEdGraph* Graph = GetGraph();
			UBlueprint* BP = Graph->GetTypedOuter<UBlueprint>();
			auto BPClass = BP->ParentClass;
			UClass* Result = FindObject<UClass>(ANY_PACKAGE, *BPClass->GetName());
			TSubclassOf<UFlowControllerBase> ControllerClass = Result;

			// Spawn Call Function node for each Exec pin. Target is UFlowControllersManager shared instance.
			UK2Node_CallFunction* CallFunction = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
			UFunction* StateEnterFunction = ControllerClass->FindFunctionByName(TEXT("RequestExit"));
			CallFunction->SetFromFunction(StateEnterFunction);
			CallFunction->AllocateDefaultPins();
			UEdGraphPin* CallFunctionEnterPin = CallFunction->GetExecPin();
			UEdGraphPin* EnterNamePin = CallFunction->FindPin(TEXT("ExitName"));
			EnterNamePin->DefaultValue = Pin->GetName();

			CompilerContext.MovePinLinksToIntermediate(*Pin, *CallFunctionEnterPin);
			CompilerContext.MessageLog.NotifyIntermediateObjectCreation(CallFunction, this);
		}
	}

	BreakAllNodeLinks();
}

#undef LOCTEXT_NAMESPACE