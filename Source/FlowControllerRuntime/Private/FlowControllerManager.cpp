#include "FlowControllerManager.h"

static UFlowControllersManager* instance;

UFlowControllersManager* UFlowControllersManager::SharedInstance() {
	if (instance == nullptr) {
		instance = NewObject<UFlowControllersManager>();
		instance->bShouldTick = true;
		instance->AddToRoot();
	}

	return instance;
}

void UFlowControllersManager::Tick(float DeltaTime) {
	if (this->ExitRequests.Num() > 0) {
		TArray<FItemExitRequest> ExitRequested;
		for (FItemExitRequest request : ExitRequests) {
			TArray<FName> Pins;
			request.FlowItem->GetPins(Pins, EPinDirection::Out);
			if (Pins.Contains(request.ExitName) || request.ExitName.IsEqual(ParentExit) || request.ExitName.IsEqual(ItemBroken)) {
				if (Pins.Contains(request.ExitName)) {
					request.FlowItem->PrepareToExit(request.ExitName);
				}
				else {
					request.FlowItem->PrepareToExit(ItemTerminate);
				}
				ExitRequested.Add(request);
			}
			else {
				request.FlowItem->UnimplementedExitName(request.ExitName);
			}
		}
		ExitRequests.Empty();

		for (FItemExitRequest request : ExitRequested) {
			TArray<FName> Pins;
			request.FlowItem->GetPins(Pins, EPinDirection::Out);
			if (Pins.Contains(request.ExitName) || request.ExitName.IsEqual(ItemBroken)) {
				request.FlowItem->Exit(request.ExitName);
			}
			if (Controllers.Contains(request.FlowItem->ItemID)) {
				Controllers.Remove(request.FlowItem->ItemID);
			}
		}
	}
};

void UFlowControllersManager::StateEnter(FName EnterName, TSubclassOf<UFlowStateBase> StateClass, FString ItemID, UFlowControllerBase* Parent, FOnFlowItemFinishedDelegate const& StateFinishedCallback) {
	UFlowControllersManager* Instance = UFlowControllersManager::SharedInstance();
	if (Parent != nullptr) {
		UFlowItemBase* Item = NewObject<UFlowItemBase>(Instance, StateClass);
		if (Item != nullptr) {
			Item->ItemID = ItemID;
			Parent->EnterChild(Item);
			Item->Enter(EnterName, StateFinishedCallback);
		}
	}
}

void UFlowControllersManager::StartController(FName EnterName, UObject* FlowExchangerObject, TSubclassOf<UFlowControllerBase> ControllerClass, FString ItemID, UFlowControllerBase* Parent, FOnFlowItemFinishedDelegate const& ControllerFinishedCallback) {
	UFlowControllersManager* Instance = UFlowControllersManager::SharedInstance();
	UFlowControllerBase* Controller = NewObject<UFlowControllerBase>(Instance, ControllerClass);
	if (Controller != nullptr) {
		Controller->ItemID = ItemID;
		if (Parent != nullptr) {
			Parent->EnterChild(Controller);
		}
		Controller->StartController(FlowExchangerObject, EnterName, ControllerFinishedCallback);
		Instance->Controllers.Add(ItemID, Controller);
	}
}

void UFlowControllersManager::BreakController(FString ControllerID) {
	UFlowControllersManager* Instance = UFlowControllersManager::SharedInstance();
	if (Instance->Controllers.Contains(ControllerID)) {
		UFlowControllerBase* Controller = Instance->Controllers[ControllerID];
		Controller->RequestExit(FName(TEXT("Broken")));
	}
}

void UFlowControllersManager::RequestExit(FName ExitName, UFlowItemBase* FlowItem) {
	UFlowControllersManager* Instance = UFlowControllersManager::SharedInstance();
	FItemExitRequest request;
	request.ExitName = ExitName;
	request.FlowItem = FlowItem;
	Instance->ExitRequests.Add(request);
}