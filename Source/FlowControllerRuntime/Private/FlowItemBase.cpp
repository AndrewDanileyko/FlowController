// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlowItemBase.h"
#include "../Public/FlowControllerManager.h"

void UFlowItemBase::Enter(FName InputName, FOnFlowItemFinishedDelegate const& StateFinishedCallback) {
	FinishedCallback = StateFinishedCallback;
	bShouldTick = true;
	DidEnter(InputName);
}

void UFlowItemBase::RequestExit(FName ExitName) {
	UFlowControllersManager::RequestExit(ExitName, this);
}

void UFlowItemBase::PrepareToExit(FName ExitName) {
	if (FinishedCallbackParentNotify.IsBound()) {
		FinishedCallbackParentNotify.Execute(this);
	}
	bShouldTick = false;
}

void UFlowItemBase::Exit(FName ExitName) {
	if (FinishedCallback.IsBound()) {
		FinishedCallback.Execute(ExitName);
	}
}

UFlowItemBase::~UFlowItemBase() {
	this->bShouldTick = false;
}
