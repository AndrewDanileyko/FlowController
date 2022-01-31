// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlowControllerNode.h"
#include "Editor/BlueprintGraph/Public/BlueprintNodeSpawner.h"
#include "Editor/BlueprintGraph/Classes/EdGraphSchema_K2.h"
#include "Editor/KismetCompiler/Public/KismetCompiler.h"
#include <Editor/BlueprintGraph/Classes/K2Node_CustomEvent.h>
#include <Editor/BlueprintGraph/Classes/K2Node_CallFunction.h>
#include <Editor/BlueprintGraph/Classes/K2Node_SwitchName.h>
#include <Editor/BlueprintGraph/Classes/K2Node_Self.h>

#define LOCTEXT_NAMESPACE "FlowControllerNode"

#pragma optimize ("", off)

void UFlowControllerNode::AllocateDefaultPins() {
	Super::AllocateDefaultPins();

	if (ControllerClassName.IsEmpty() == false) {
		UClass* Result = FindObject<UClass>(ANY_PACKAGE, *ControllerClassName);
		if (Result != nullptr) {
			TSubclassOf<UFlowControllerBase> ControllerClass = Result;

			TArray<FName> PinNames;
			GetPins(ControllerClass, PinNames, EPinDirection::In);
			for (FName PinName : PinNames) {
				CreatePin(EEdGraphPinDirection::EGPD_Input, UEdGraphSchema_K2::PC_Exec, PinName);
			}

			PinNames.Empty();
			GetPins(ControllerClass, PinNames, EPinDirection::Out);
			for (FName PinName : PinNames) {
				CreatePin(EEdGraphPinDirection::EGPD_Output, UEdGraphSchema_K2::PC_Exec, PinName);
			}
		}
	}
	
	UEdGraphPin* ClassPin = CreatePin(EEdGraphPinDirection::EGPD_Input, UEdGraphSchema_K2::PC_Class, UFlowControllerBase::StaticClass()->GetOwnerClass(), FName(TEXT("ControllerClass")));
	ClassPin->bNotConnectable = true;

	UEdGraphPin* ExchangerPin = CreatePin(EEdGraphPinDirection::EGPD_Input, UEdGraphSchema_K2::PC_Object, UObject::StaticClass(), FName(TEXT("ExchangerObject")));
}

void UFlowControllerNode::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const {
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);

		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

void UFlowControllerNode::PinDefaultValueChanged(UEdGraphPin* Pin) {
	if (Pin && Pin->PinName == "ControllerClass")
	{
		ControllerClassName = Pin->DefaultObject != nullptr ? Pin->DefaultObject->GetName() : TEXT("");
		ReconstructNode();
	}
}

FText UFlowControllerNode::GetNodeTitle(ENodeTitleType::Type TitleType) const {
	if (TitleType == ENodeTitleType::FullTitle) {
		UEdGraphPin* ClassPin = FindPin(FName(TEXT("ControllerClass")), EEdGraphPinDirection::EGPD_Input);
		if (ClassPin->DefaultObject != nullptr) {
			FString Title = TEXT("Flow Controller: ");
			FString ClassName = ClassPin->DefaultObject->GetName();
			if (ClassName.EndsWith(TEXT("_C"))) {
				ClassName.RemoveFromEnd(TEXT("_C"));
			}
			Title.Append(ClassName);
			return FText::FromString(Title);
		}
		else {
			return FText::FromString(TEXT("Flow Controller"));
		}
	}

	if (TitleType == ENodeTitleType::MenuTitle) {
		return FText::FromString(TEXT("Flow Controller Node"));
	}
	
	return FText::GetEmpty();
}

void UFlowControllerNode::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) {
	Super::ExpandNode(CompilerContext, SourceGraph);

	if (CheckForErrors(CompilerContext))
	{
		BreakAllNodeLinks();
		return;
	}

	UEdGraphPin* ControllerClassPin = FindPin(FName(TEXT("ControllerClass")), EEdGraphPinDirection::EGPD_Input);
	UEdGraphPin* NodeExchangerPin = FindPin(FName(TEXT("ExchangerObject")), EEdGraphPinDirection::EGPD_Input);
	if (ControllerClassPin->DefaultObject != nullptr) {
		
		const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

		FString CtClassName = ControllerClassPin->DefaultObject->GetName();
		TSubclassOf<UFlowControllerBase> ControllerClass = FindObject<UClass>(ANY_PACKAGE, *CtClassName);
		TArray<FName> PinNames;
		GetPins(ControllerClass, PinNames, EPinDirection::Out);

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
		FString title = ControllerClassPin->DefaultObject->GetName();
		title.Append("_");
		title.Append(this->NodeGuid.ToString());
		EventNode->CustomFunctionName = FName(*title);
		EventNode->AllocateDefaultPins();
		FEdGraphPinType NamePinType;
		NamePinType.PinCategory = UEdGraphSchema_K2::PC_Name;
		UEdGraphPin* CustomEventExitNamePin = EventNode->CreateUserDefinedPin(FName(TEXT("ExitPointName")), NamePinType, EEdGraphPinDirection::EGPD_Output);
		CompilerContext.MessageLog.NotifyIntermediateObjectCreation(EventNode, this);
		UEdGraphPin* OutputDelegatePin = EventNode->FindPin(TEXT("OutputDelegate"));
		
		// Spawn Self node with ExitPointName output parameter
		UEdGraphPin* SelfPin = nullptr;
		UEdGraph* Graph = GetGraph();
		UBlueprint* BP = Graph->GetTypedOuter<UBlueprint>();
		auto BPClass = BP->GetBlueprintClass();
		if (BPClass->IsChildOf(UFlowControllerBase::StaticClass())) {
			UK2Node_Self* SelfNode = CompilerContext.SpawnIntermediateNode<UK2Node_Self>(this, SourceGraph);
			SelfNode->AllocateDefaultPins();
			SelfPin = SelfNode->FindPin(UEdGraphSchema_K2::PN_Self, EEdGraphPinDirection::EGPD_Output);
		}

		PinNames.Empty();
		GetPins(ControllerClass, PinNames, EPinDirection::In);
		for (FName PinName : PinNames) {
			if (PinName.IsEqual(FName(TEXT("Break"))) == false) {
				UEdGraphPin* EnterPin = FindPin(PinName);
					// Spawn Call Function node for each Exec pin. Target is UFlowControllersManager shared instance.
					UK2Node_CallFunction* CallFunction = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
					UFunction* StateEnterFunction = UFlowControllersManager::StaticClass()->FindFunctionByName(TEXT("StartController"));
					CallFunction->SetFromFunction(StateEnterFunction);
					CallFunction->AllocateDefaultPins();
					UEdGraphPin* CallFunctionEnterPin = CallFunction->GetExecPin();
					UEdGraphPin* EnterNamePin = CallFunction->FindPin(TEXT("EnterName"));
					EnterNamePin->DefaultValue = PinName.ToString();
					UEdGraphPin* FunctionClassPin = CallFunction->FindPin(TEXT("ControllerClass"));
					UEdGraphPin* CallbackPin = CallFunction->FindPin(TEXT("ControllerFinishedCallback"));
				if (SelfPin != nullptr) {
					UEdGraphPin* FunctionSelfPin = CallFunction->FindPin(TEXT("Parent"));
					Schema->TryCreateConnection(SelfPin, FunctionSelfPin);
				}
				UEdGraphPin* ItemIDPin = CallFunction->FindPin(TEXT("ItemID"));
				ItemIDPin->DefaultValue = NodeGuid.ToString();
				UEdGraphPin* ExchangerObjectPin = CallFunction->FindPin(TEXT("FlowExchangerObject"));
				CompilerContext.MovePinLinksToIntermediate(*ControllerClassPin, *FunctionClassPin);
				CompilerContext.MovePinLinksToIntermediate(*EnterPin, *CallFunctionEnterPin);
				Schema->TryCreateConnection(CallbackPin, OutputDelegatePin);
				CompilerContext.MessageLog.NotifyIntermediateObjectCreation(CallFunction, this);

				if (NodeExchangerPin->HasAnyConnections() == true) {
					CompilerContext.MovePinLinksToIntermediate(*NodeExchangerPin, *ExchangerObjectPin);
				}
				else {
					ExchangerObjectPin->DefaultValue = NodeExchangerPin->DefaultValue;
					ExchangerObjectPin->DefaultObject = NodeExchangerPin->DefaultObject;
				}
			}
		}

		UEdGraphPin* BreakPin = FindPin(FName(TEXT("Break")));
		if (BreakPin != nullptr && BreakPin->HasAnyConnections()) {
			UK2Node_CallFunction* CallFunction = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
			UFunction* ControllerBreakFunction = UFlowControllersManager::StaticClass()->FindFunctionByName(TEXT("BreakController"));
			CallFunction->SetFromFunction(ControllerBreakFunction);
			CallFunction->AllocateDefaultPins();
			UEdGraphPin* CallFunctionEnterPin = CallFunction->GetExecPin();
			UEdGraphPin* ItemIDPin = CallFunction->FindPin(TEXT("ControllerID"));
			ItemIDPin->DefaultValue = NodeGuid.ToString();
			CompilerContext.MovePinLinksToIntermediate(*BreakPin, *CallFunctionEnterPin);
			CompilerContext.MessageLog.NotifyIntermediateObjectCreation(CallFunction, this);
		}

		Schema->TryCreateConnection(CustomEventExitNamePin, SwitchNode->GetSelectionPin());

		UEdGraphPin* EventThenPin = EventNode->FindPin(UEdGraphSchema_K2::PN_Then);
		Schema->TryCreateConnection(EventThenPin, SwitchNode->GetExecPin());

	}
	
	BreakAllNodeLinks();
}

bool UFlowControllerNode::CheckForErrors(FKismetCompilerContext& CompilerContext)
{
	bool bError = false;

	if (false)
	{
		//CompilerContext.MessageLog.Error(*LOCTEXT("Error", "Node @@ had an input error.").ToString(), this);
		bError = true;
	}

	return bError;
}

void UFlowControllerNode::GetPins(TSubclassOf<UFlowControllerBase> ControllerClass, TArray<FName>& Names, EPinDirection Direction) {
	ControllerClass.GetDefaultObject()->GetPins(Names, Direction);
	if (Direction == EPinDirection::In && bBreakPin == true) {
		Names.Add(TEXT("Break"));
	}
	if (Direction == EPinDirection::Out && bBreakPin == true) {
		Names.Add(TEXT("Broken"));
	}
}

UObject* UFlowControllerNode::GetJumpTargetForDoubleClick() const
{
	UEdGraphPin* ClassPin = FindPin(TEXT("ControllerClass"));

	auto ControllerClass = Cast<UClass>(ClassPin->DefaultObject);

	if (ControllerClass != NULL)
	{
		UBlueprintGeneratedClass* ParentClass = Cast<UBlueprintGeneratedClass>(ControllerClass);
		if (ParentClass != NULL && ParentClass->ClassGeneratedBy != NULL)
		{
			UBlueprint* Blueprint = Cast<UBlueprint>(ParentClass->ClassGeneratedBy);

			if (Blueprint != nullptr) {
				return Blueprint;
			}
		}
	}

	return nullptr;
}

void UFlowControllerNode::PostEditChangeProperty(struct FPropertyChangedEvent& e) {
	Super::PostEditChangeProperty(e);

	ReconstructNode();
}

FString UFlowControllerNode::GetFindReferenceSearchString() const {
	UEdGraphPin* ClassPin = FindPin(FName(TEXT("ControllerClass")), EEdGraphPinDirection::EGPD_Input);
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