// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "HttpManager.generated.h"

class IHttpRequest;
class IHttpResponse;

UCLASS()
class SABRIMMO_API UHttpManager : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Network", meta = (WorldContext = "WorldContextObject"))
    static void TestServerConnection(UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, Category = "Network", meta = (WorldContext = "WorldContextObject"))
    static void HealthCheck(UObject* WorldContextObject);

    static void OnHealthCheckResponse(TSharedPtr<IHttpRequest> Request, TSharedPtr<IHttpResponse> Response, bool bWasSuccessful);
};
