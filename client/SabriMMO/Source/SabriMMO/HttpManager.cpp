// Fill out your copyright notice in the Description page of Project Settings.

#include "HttpManager.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Http.h"

void UHttpManager::TestServerConnection(UObject* WorldContextObject)
{
    UE_LOG(LogTemp, Log, TEXT("Testing server connection..."));
    HealthCheck(WorldContextObject);
}

void UHttpManager::HealthCheck(UObject* WorldContextObject)
{
    TSharedPtr<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->OnProcessRequestComplete().BindStatic(&UHttpManager::OnHealthCheckResponse);
    Request->SetURL(TEXT("http://localhost:3000/health"));
    Request->SetVerb(TEXT("GET"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->ProcessRequest();

    UE_LOG(LogTemp, Log, TEXT("Sending health check request to: http://localhost:3000"));
}

void UHttpManager::OnHealthCheckResponse(TSharedPtr<IHttpRequest> Request, TSharedPtr<IHttpResponse> Response, bool bWasSuccessful)
{
    if (bWasSuccessful && Response.IsValid())
    {
        int32 ResponseCode = Response->GetResponseCode();
        FString ResponseContent = Response->GetContentAsString();

        UE_LOG(LogTemp, Log, TEXT("Health Check Response Code: %d"), ResponseCode);
        UE_LOG(LogTemp, Log, TEXT("Health Check Response: %s"), *ResponseContent);

        if (ResponseCode == 200)
        {
            UE_LOG(LogTemp, Display, TEXT("✓ Server is ONLINE and connected to database!"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("✗ Server returned error code: %d"), ResponseCode);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("✗ Failed to connect to server. Is it running on port 3000?"));
    }
}
