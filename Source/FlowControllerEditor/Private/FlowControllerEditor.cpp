// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlowControllerEditor.h"
#include "Modules/ModuleManager.h"
#include "FlowControllerEnterNode.h"
#include "FlowControllerExitNode.h"
#include "FlowControllerBase.h"
#include <Editor/UnrealEd/Public/Kismet2/KismetEditorUtilities.h>

IMPLEMENT_MODULE(FFlowControllerEditor, FlowController);

void FFlowControllerEditor::StartupModule()
{
	FKismetEditorUtilities::FOnBlueprintCreated Delegate;
	Delegate.BindRaw(this, &FFlowControllerEditor::OnBlueprintCreated);

	FKismetEditorUtilities::RegisterOnBlueprintCreatedCallback(this, UFlowControllerBase::StaticClass(), Delegate);
}

void FFlowControllerEditor::ShutdownModule()
{
	// Put your module termination code here
}

void FFlowControllerEditor::OnBlueprintCreated(UBlueprint* blueprint) {
	UEdGraph* Graph = blueprint->UbergraphPages[0];

	UFlowControllerEnterNode* EnterNode = NewObject<UFlowControllerEnterNode>(Graph);
	EnterNode->CreateNewGuid();
	EnterNode->PostPlacedNewNode();
	EnterNode->AllocateDefaultPins();
	Graph->AddNode(EnterNode);

	UFlowControllerExitNode* ExitNode = NewObject<UFlowControllerExitNode>(Graph);
	ExitNode->NodePosX = 500;
	ExitNode->CreateNewGuid();
	ExitNode->PostPlacedNewNode();
	ExitNode->AllocateDefaultPins();
	Graph->AddNode(ExitNode);
}