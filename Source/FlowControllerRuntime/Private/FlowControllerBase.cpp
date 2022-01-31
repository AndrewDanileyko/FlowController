#include "FlowControllerBase.h"

void UFlowControllerBase::StartController(UObject* ExchangerObject, FName InputName, FOnFlowItemFinishedDelegate const& StateFinishedCallback) {
	FlowExchangerObject = ExchangerObject;

	Super::Enter(InputName, StateFinishedCallback);

	// Should be called after Super
	OnFlowControllerEnter(InputName);
}

void UFlowControllerBase::EnterChild(UFlowItemBase* Child) {
	Children.Add(Child);
	Child->FinishedCallbackParentNotify.BindUFunction(this, TEXT("OnChildFinishedNotify"));
	Child->ContextFlowExchanger = FlowExchangerObject;
}

void UFlowControllerBase::OnChildFinishedNotify(UFlowItemBase* Child) {
	Children.Remove(Child);
}

void UFlowControllerBase::RequestExit(FName ExitName) {
	for (UFlowItemBase* Child : Children) {
		Child->FinishedCallbackParentNotify.Unbind();
		Child->RequestExit(ParentExit);
	}
	Children.Empty();
	Super::RequestExit(ExitName);
}