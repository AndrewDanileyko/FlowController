// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "FlowControllerEnterNode.h"
#include "Editor/BlueprintGraph/Public/BlueprintNodeSpawner.h"
#include "Editor/BlueprintGraph/Classes/EdGraphSchema_K2.h"
#include "Editor/KismetCompiler/Public/KismetCompiler.h"
#include "FlowControllerManager.h"
#include <Editor/BlueprintGraph/Classes/K2Node_Event.h>
#include <Editor/BlueprintGraph/Classes/K2Node_CallFunction.h>
#include <Editor/BlueprintGraph/Classes/K2Node_SwitchName.h>

#define LOCTEXT_NAMESPACE "FlowStateEnterNode"

#pragma optimize ("", off)

void UFlowControllerEnterNode::AllocateDefaultPins() {
	Super::AllocateDefaultPins();

	UEdGraph* Graph = GetGraph();
	UBlueprint* BP = Graph->GetTypedOuter<UBlueprint>();
	auto BPClass = BP->ParentClass;

	if (BPClass->IsChildOf(UFlowControllerBase::StaticClass())) {
		UClass* Result = FindObject<UClass>(ANY_PACKAGE, *BPClass->GetName());

		TSubclassOf<UFlowControllerBase> ControllerClass = Result;

		TArray<FName> PinNames;
		ControllerClass.GetDefaultObject()->GetPins(PinNames, EPinDirection::In);
		for (FName PinName : PinNames) {
			CreatePin(EEdGraphPinDirection::EGPD_Output, UEdGraphSchema_K2::PC_Exec, PinName);
		}
	}
}

FText UFlowControllerEnterNode::GetNodeTitle(ENodeTitleType::Type TitleType) const {
	return FText::FromString(TEXT("Enter"));
}

void UFlowControllerEnterNode::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) {
	Super::ExpandNode(CompilerContext, SourceGraph);

	UFunction* OnEnteredFunction = CompilerContext.NewClass->FindFunctionByName("OnFlowControllerEnter");
	UK2Node_Event* EventNode = CompilerContext.SpawnIntermediateNode<UK2Node_Event>(this, SourceGraph);
	EventNode->EventReference.SetFromField<UFunction>(OnEnteredFunction, false);
	EventNode->bOverrideFunction = true;
	EventNode->AllocateDefaultPins();
	CompilerContext.MessageLog.NotifyIntermediateObjectCreation(EventNode, this);
	UEdGraphPin* EventNamePin = EventNode->FindPin(FName(TEXT("EnterName")));

	// Spawn Switch node with Output pins

	TArray<UEdGraphPin *> ThisPins = GetAllPins();

	UK2Node_SwitchName* SwitchNode = CompilerContext.SpawnIntermediateNode<UK2Node_SwitchName>(this, SourceGraph);
	SwitchNode->bHasDefaultPin = false;
	for (UEdGraphPin* Pin : ThisPins) {
		SwitchNode->PinNames.Add(FName(Pin->GetName()));
	}
	SwitchNode->AllocateDefaultPins();
	CompilerContext.MessageLog.NotifyIntermediateObjectCreation(SwitchNode, this);

	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

	UEdGraphPin* EventThenPin = EventNode->FindPin(UEdGraphSchema_K2::PN_Then);
	Schema->TryCreateConnection(EventThenPin, SwitchNode->GetExecPin());
	Schema->TryCreateConnection(EventNamePin, SwitchNode->GetSelectionPin());

	for (UEdGraphPin* ThisThenPin : ThisPins) {
		UEdGraphPin* SwitchThenPin = SwitchNode->FindPin(FName(ThisThenPin->GetName()));

		if (ThisThenPin->LinkedTo.Num() > 0) {
			Schema->TryCreateConnection(SwitchThenPin, ThisThenPin->LinkedTo[0]);
		}
	}

	BreakAllNodeLinks();
}

#undef LOCTEXT_NAMESPACE