// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlowStateNode.h"
#include "Editor/BlueprintGraph/Public/BlueprintNodeSpawner.h"
#include "Editor/BlueprintGraph/Classes/EdGraphSchema_K2.h"
#include "Editor/KismetCompiler/Public/KismetCompiler.h"
#include "FlowControllerManager.h"
#include <Editor/BlueprintGraph/Classes/K2Node_CustomEvent.h>
#include <Editor/BlueprintGraph/Classes/K2Node_CallFunction.h>
#include <Editor/BlueprintGraph/Classes/K2Node_SwitchName.h>
#include <Editor/BlueprintGraph/Classes/K2Node_Self.h>

#define LOCTEXT_NAMESPACE "FlowStateNode"

#pragma optimize ("", off)

void UFlowStateNode::AllocateDefaultPins() {
	Super::AllocateDefaultPins();

	if (StateClassName.IsEmpty() == false) {
		UClass* Result = FindObject<UClass>(ANY_PACKAGE, *StateClassName);

		TSubclassOf<UFlowStateBase> FlowClass = Result;

		TArray<FName> PinNames;
		GetPins(FlowClass, PinNames, EPinDirection::In);
		for (FName PinName : PinNames) {
			CreatePin(EEdGraphPinDirection::EGPD_Input, UEdGraphSchema_K2::PC_Exec, PinName);
		}

		PinNames.Empty();
		GetPins(FlowClass, PinNames, EPinDirection::Out);
		for (FName PinName : PinNames) {
			CreatePin(EEdGraphPinDirection::EGPD_Output, UEdGraphSchema_K2::PC_Exec, PinName);
		}
	}

	UEdGraphPin* ClassPin = CreatePin(EEdGraphPinDirection::EGPD_Input, UEdGraphSchema_K2::PC_Class, UFlowStateBase::StaticClass()->GetOwnerClass(), FName(TEXT("StateClass")));
	ClassPin->bNotConnectable = true;
}

void UFlowStateNode::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const {
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);

		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

void UFlowStateNode::PinDefaultValueChanged(UEdGraphPin* Pin) {
	if (Pin && Pin->PinName == "StateClass")
	{
		StateClassName = Pin->DefaultObject != nullptr ? Pin->DefaultObject->GetName() : TEXT("");
		ReconstructNode();
	}
}

FText UFlowStateNode::GetNodeTitle(ENodeTitleType::Type TitleType) const {
	if (TitleType == ENodeTitleType::FullTitle) {
		UEdGraphPin* ClassPin = FindPin(FName(TEXT("StateClass")), EEdGraphPinDirection::EGPD_Input);
		if (ClassPin->DefaultObject != nullptr) {
			FString Title = TEXT("Flow State: ");
			FString ClassName = ClassPin->DefaultObject->GetName();
			if (ClassName.EndsWith(TEXT("_C"))) {
				ClassName.RemoveFromEnd(TEXT("_C"));
			}
			Title.Append(ClassName);

			return FText::FromString(Title);
		}
		else {
			return FText::FromString(TEXT("Flow State"));
		}
	}

	if (TitleType == ENodeTitleType::MenuTitle) {
		return FText::FromString(TEXT("Flow State Node"));
	}
	
	return FText::GetEmpty();
}

void UFlowStateNode::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) {
	Super::ExpandNode(CompilerContext, SourceGraph);

	if (CheckForErrors(CompilerContext))
	{
		BreakAllNodeLinks();
		return;
	}

	UEdGraphPin* StateClassPin = FindPin(FName(TEXT("StateClass")), EEdGraphPinDirection::EGPD_Input);
	if (StateClassPin->DefaultObject != nullptr) {
		
		const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

		FString StClassName = StateClassPin->DefaultObject->GetName();
		TSubclassOf<UFlowStateBase> FlowClass = FindObject<UClass>(ANY_PACKAGE, *StClassName);
		TArray<FName> PinNames;
		GetPins(FlowClass, PinNames, EPinDirection::Out);

		// Spawn Switch node with Output pins
		UK2Node_SwitchName* SwitchNode = CompilerContext.SpawnIntermediateNode<UK2Node_SwitchName>(this, SourceGraph);
		SwitchNode->bHasDefaultPin = false;
		for (FName PinName : PinNames) {
			SwitchNode->PinNames.Add(PinName);
		}
		SwitchNode->AllocateDefaultPins();
		CompilerContext.MessageLog.NotifyIntermediateObjectCreation(SwitchNode, this);
		for (FName PinName : PinNames) {
			UEdGraphPin* ThisThenPin = FindPin(PinName);
			UEdGraphPin* ThenPin = SwitchNode->FindPin(PinName);

			if (ThisThenPin->LinkedTo.Num() > 0) {
				Schema->TryCreateConnection(ThenPin, ThisThenPin->LinkedTo[0]);
			}
		}

		// Spawn Custom Event node with ExitPointName output parameter
		UK2Node_CustomEvent* EventNode = CompilerContext.SpawnIntermediateNode<UK2Node_CustomEvent>(this, SourceGraph);
		FString title = StateClassPin->DefaultObject->GetName();
		title.Append("_");
		title.Append(this->NodeGuid.ToString());
		EventNode->CustomFunctionName = FName(*title);
		EventNode->AllocateDefaultPins();
		FEdGraphPinType NamePinType;
		NamePinType.PinCategory = UEdGraphSchema_K2::PC_Name;
		UEdGraphPin* CustomEventExitNamePin = EventNode->CreateUserDefinedPin(FName(TEXT("ExitPointName")), NamePinType, EEdGraphPinDirection::EGPD_Output);
		UEdGraphPin* OutputDelegatePin = EventNode->FindPin(TEXT("OutputDelegate"));
		CompilerContext.MessageLog.NotifyIntermediateObjectCreation(EventNode, this);

		// Spawn Self node with ExitPointName output parameter
		UEdGraphPin* SelfPin = nullptr;
		UEdGraph* Graph = GetGraph();
		UBlueprint* BP = Graph->GetTypedOuter<UBlueprint>();
		auto BPClass = BP->ParentClass;
		if (BPClass->IsChildOf(UFlowControllerBase::StaticClass())) {
			UK2Node_Self* SelfNode = CompilerContext.SpawnIntermediateNode<UK2Node_Self>(this, SourceGraph);
			SelfNode->AllocateDefaultPins();
			SelfPin = SelfNode->FindPin(UEdGraphSchema_K2::PN_Self, EEdGraphPinDirection::EGPD_Output);
			CompilerContext.MessageLog.NotifyIntermediateObjectCreation(SelfNode, this);
		}

		PinNames.Empty();
		GetPins(FlowClass, PinNames, EPinDirection::In);
		for (FName PinName : PinNames) {
			UEdGraphPin* EnterPin = FindPin(PinName);
			// Spawn Call Function node for each Exec pin. Target is UFlowControllersManager shared instance.
			UK2Node_CallFunction* CallFunction = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
			UFunction* StateEnterFunction = UFlowControllersManager::StaticClass()->FindFunctionByName(TEXT("StateEnter"));
			CallFunction->SetFromFunction(StateEnterFunction);
			CallFunction->AllocateDefaultPins();
			UEdGraphPin* CallFunctionEnterPin = CallFunction->GetExecPin();
			UEdGraphPin* EnterNamePin = CallFunction->FindPin(TEXT("EnterName"));
			EnterNamePin->DefaultValue = PinName.ToString();
			UEdGraphPin* FunctionClassPin = CallFunction->FindPin(TEXT("StateClass"));
			if (SelfPin != nullptr) {
				UEdGraphPin* FunctionSelfPin = CallFunction->FindPin(TEXT("Parent"));
				Schema->TryCreateConnection(SelfPin, FunctionSelfPin);
			}
			UEdGraphPin* ItemIDPin = CallFunction->FindPin(TEXT("ItemID"));
			ItemIDPin->DefaultValue = NodeGuid.ToString();
			UEdGraphPin* CallbackPin = CallFunction->FindPin(TEXT("StateFinishedCallback"));
			CompilerContext.MovePinLinksToIntermediate(*StateClassPin, *FunctionClassPin);
			CompilerContext.MovePinLinksToIntermediate(*EnterPin, *CallFunctionEnterPin);
			Schema->TryCreateConnection(CallbackPin, OutputDelegatePin);
			CompilerContext.MessageLog.NotifyIntermediateObjectCreation(CallFunction, this);
		}

		Schema->TryCreateConnection(CustomEventExitNamePin, SwitchNode->GetSelectionPin());

		UEdGraphPin* EventThenPin = EventNode->FindPin(UEdGraphSchema_K2::PN_Then);
		Schema->TryCreateConnection(EventThenPin, SwitchNode->GetExecPin());

	}
	
	BreakAllNodeLinks();
}

bool UFlowStateNode::CanPasteHere(const UEdGraph* TargetGraph) const
{
	UBlueprint* Blueprint = Cast<UBlueprint>(TargetGraph->GetOuter());

	if (Blueprint->ParentClass->IsChildOf(UFlowControllerBase::StaticClass())) {
		return true;
	}

	return false;
}

bool UFlowStateNode::IsCompatibleWithGraph(UEdGraph const* Graph) const {
	UBlueprint* Blueprint = Cast<UBlueprint>(Graph->GetOuter());

	if (Blueprint->ParentClass->IsChildOf(UFlowControllerBase::StaticClass())) {
		return true;
	}

	return false;
}

bool UFlowStateNode::CheckForErrors(FKismetCompilerContext& CompilerContext)
{
	bool bError = false;

	if (false)
	{
		//CompilerContext.MessageLog.Error(*LOCTEXT("Error", "Node @@ had an input error.").ToString(), this);
		bError = true;
	}

	return bError;
}

void UFlowStateNode::GetPins(TSubclassOf<UFlowStateBase> FlowClass, TArray<FName>& Names, EPinDirection Direction) {
	if (Direction == EPinDirection::Out && bDefaultNextPin == true) {
		Names.Add(TEXT("Next"));
	}
	FlowClass.GetDefaultObject()->GetPins(Names, Direction);
	if (Direction == EPinDirection::In && bDefaultBackPin == true) {
		Names.Add(TEXT("Back"));
	}
}

void UFlowStateNode::PostEditChangeProperty(struct FPropertyChangedEvent& e) {
	Super::PostEditChangeProperty(e);

	ReconstructNode();
}

FString UFlowStateNode::GetFindReferenceSearchString() const {
	UEdGraphPin* ClassPin = FindPin(FName(TEXT("StateClass")), EEdGraphPinDirection::EGPD_Input);
	if (ClassPin->DefaultObject != nullptr) {
		FString ClassName = ClassPin->DefaultObject->GetName();
		if (ClassName.EndsWith(TEXT("_C"))) {
			ClassName.RemoveFromEnd(TEXT("_C"));
		}
		return ClassName;
	}

	return TEXT("");
}

#undef LOCTEXT_NAMESPACE