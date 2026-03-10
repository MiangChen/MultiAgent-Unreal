// MACommHttpServer.cpp
// HTTP 服务器模块实现

#include "Core/Comm/Infrastructure/MACommHttpServer.h"
#include "Core/Comm/Runtime/MACommSubsystem.h"
#include "Core/SceneGraph/Runtime/MASceneGraphManager.h"
#include "HttpServerModule.h"
#include "IHttpRouter.h"
#include "HttpPath.h"
#include "HttpRequestHandler.h"
#include "HttpServerResponse.h"

DEFINE_LOG_CATEGORY_STATIC(LogMACommHttpServer, Log, All);

FMACommHttpServer::FMACommHttpServer(UMACommSubsystem* InOwner)
    : Owner(InOwner)
{
}

FMACommHttpServer::~FMACommHttpServer()
{
    Stop();
}

//=============================================================================
// 服务器控制
//=============================================================================

void FMACommHttpServer::Start(int32 Port)
{
    if (bServerRunning)
    {
        UE_LOG(LogMACommHttpServer, Warning, TEXT("Start: Server already running"));
        return;
    }

    ServerPort = Port;

    // 获取 HTTP 服务器模块
    FHttpServerModule& HttpServerModule = FHttpServerModule::Get();
    
    // 启动 HTTP 监听器
    TSharedPtr<IHttpRouter> HttpRouter = HttpServerModule.GetHttpRouter(ServerPort);
    if (!HttpRouter.IsValid())
    {
        UE_LOG(LogMACommHttpServer, Error, TEXT("Start: Failed to get HTTP router on port %d, continuing in client-only mode"), ServerPort);
        return;
    }

    // 注册 /api/health 路由
    FHttpRequestHandler HealthHandler;
    HealthHandler.BindRaw(this, &FMACommHttpServer::HandleHealthRequest);
    FHttpRouteHandle HealthRouteHandle = HttpRouter->BindRoute(
        FHttpPath(TEXT("/api/health")),
        EHttpServerRequestVerbs::VERB_GET,
        HealthHandler
    );
    
    if (HealthRouteHandle.IsValid())
    {
        HttpRouteHandles.Add(HealthRouteHandle);
        UE_LOG(LogMACommHttpServer, Log, TEXT("Start: Registered /api/health route"));
    }
    else
    {
        UE_LOG(LogMACommHttpServer, Warning, TEXT("Start: Failed to register /api/health route"));
    }

    // 注册 /api/world_state 路由
    FHttpRequestHandler WorldStateHandler;
    WorldStateHandler.BindRaw(this, &FMACommHttpServer::HandleWorldStateRequest);
    FHttpRouteHandle WorldStateRouteHandle = HttpRouter->BindRoute(
        FHttpPath(TEXT("/api/world_state")),
        EHttpServerRequestVerbs::VERB_GET,
        WorldStateHandler
    );
    
    if (WorldStateRouteHandle.IsValid())
    {
        HttpRouteHandles.Add(WorldStateRouteHandle);
        UE_LOG(LogMACommHttpServer, Log, TEXT("Start: Registered /api/world_state route"));
    }
    else
    {
        UE_LOG(LogMACommHttpServer, Warning, TEXT("Start: Failed to register /api/world_state route"));
    }

    // 启动监听器
    HttpServerModule.StartAllListeners();
    
    bServerRunning = true;
    UE_LOG(LogMACommHttpServer, Log, TEXT("Start: HTTP server started on port %d"), ServerPort);
}

void FMACommHttpServer::Stop()
{
    if (!bServerRunning)
    {
        return;
    }

    // 清理路由句柄
    FHttpServerModule& HttpServerModule = FHttpServerModule::Get();
    TSharedPtr<IHttpRouter> HttpRouter = HttpServerModule.GetHttpRouter(ServerPort, false);
    
    if (HttpRouter.IsValid())
    {
        for (const FHttpRouteHandle& Handle : HttpRouteHandles)
        {
            if (Handle.IsValid())
            {
                HttpRouter->UnbindRoute(Handle);
            }
        }
    }
    
    HttpRouteHandles.Empty();
    bServerRunning = false;
    
    UE_LOG(LogMACommHttpServer, Log, TEXT("Stop: HTTP server stopped"));
}

//=============================================================================
// 请求处理
//=============================================================================

bool FMACommHttpServer::HandleHealthRequest(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
    UE_LOG(LogMACommHttpServer, Verbose, TEXT("HandleHealthRequest: Received health check request"));
    
    // 构建响应
    FString ResponseBody = TEXT("{\"status\": \"ok\"}");
    
    // 创建响应对象
    TUniquePtr<FHttpServerResponse> Response = FHttpServerResponse::Create(ResponseBody, TEXT("application/json; charset=utf-8"));
    
    // 调用完成回调
    OnComplete(MoveTemp(Response));
    
    return true;
}

bool FMACommHttpServer::HandleWorldStateRequest(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
    UE_LOG(LogMACommHttpServer, Log, TEXT("Python -> UE5: Received world_state query request"));
    
    // 解析查询参数
    FString CategoryFilter;
    FString TypeFilter;
    FString LabelFilter;
    
    // 从 URL 查询参数中提取过滤条件
    for (const auto& Param : Request.QueryParams)
    {
        if (Param.Key.Equals(TEXT("category"), ESearchCase::IgnoreCase))
        {
            CategoryFilter = Param.Value;
        }
        else if (Param.Key.Equals(TEXT("type"), ESearchCase::IgnoreCase))
        {
            TypeFilter = Param.Value;
        }
        else if (Param.Key.Equals(TEXT("label"), ESearchCase::IgnoreCase))
        {
            LabelFilter = Param.Value;
        }
    }
    
    if (!CategoryFilter.IsEmpty() || !TypeFilter.IsEmpty() || !LabelFilter.IsEmpty())
    {
        UE_LOG(LogMACommHttpServer, Log, TEXT("HandleWorldStateRequest: Filters - category=%s, type=%s, label=%s"),
            *CategoryFilter, *TypeFilter, *LabelFilter);
    }
    
    // 构建世界状态 JSON，包含错误处理
    FString ResponseBody;
    
    // 获取场景图管理器
    UMASceneGraphManager* SceneGraphManager = nullptr;
    if (Owner)
    {
        if (UGameInstance* GameInstance = Owner->GetGameInstance())
        {
            SceneGraphManager = GameInstance->GetSubsystem<UMASceneGraphManager>();
        }
    }
    
    if (!SceneGraphManager)
    {
        UE_LOG(LogMACommHttpServer, Error, TEXT("HandleWorldStateRequest: SceneGraphManager not available - internal error"));
        ResponseBody = TEXT("{\"error\": \"Internal server error: SceneGraphManager not available\"}");
        TUniquePtr<FHttpServerResponse> Response = FHttpServerResponse::Create(ResponseBody, TEXT("application/json; charset=utf-8"));
        Response->Code = EHttpServerResponseCodes::ServerError;
        OnComplete(MoveTemp(Response));
        return true;
    }
    
    // 委托给 MASceneGraphManager 构建世界状态 JSON
    ResponseBody = SceneGraphManager->BuildWorldStateJson(CategoryFilter, TypeFilter, LabelFilter);
    
    // 验证 JSON 有效性
    TSharedPtr<FJsonValue> JsonValue;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);
    if (!FJsonSerializer::Deserialize(Reader, JsonValue) || !JsonValue.IsValid())
    {
        UE_LOG(LogMACommHttpServer, Error, TEXT("HandleWorldStateRequest: Failed to serialize world state JSON"));
        UE_LOG(LogMACommHttpServer, Error, TEXT("Raw content: %s"), *ResponseBody);
        ResponseBody = TEXT("{\"error\": \"Internal server error: JSON serialization failed\"}");
        TUniquePtr<FHttpServerResponse> Response = FHttpServerResponse::Create(ResponseBody, TEXT("application/json; charset=utf-8"));
        Response->Code = EHttpServerResponseCodes::ServerError;
        OnComplete(MoveTemp(Response));
        return true;
    }
    
    // 创建成功响应对象
    TUniquePtr<FHttpServerResponse> Response = FHttpServerResponse::Create(ResponseBody, TEXT("application/json; charset=utf-8"));
    
    // 调用完成回调
    OnComplete(MoveTemp(Response));
    
    UE_LOG(LogMACommHttpServer, Log, TEXT("UE5 -> Python: Sent world_state response"));
    
    return true;
}
