// MACommHttpServer.h
// HTTP 服务器模块 - 负责处理外部发来的 HTTP 请求

#pragma once

#include "CoreMinimal.h"
#include "HttpServerModule.h"
#include "IHttpRouter.h"
#include "HttpRouteHandle.h"
#include "HttpServerRequest.h"
#include "HttpResultCallback.h"

class UMACommSubsystem;

/**
 * HTTP 服务器处理器
 * 
 * 职责:
 * - 启动/停止本地 HTTP 服务器
 * - 注册和处理 HTTP 路由
 * - 处理 /api/health 健康检查
 * - 处理 /api/world_state 世界状态查询
 */
class MULTIAGENT_API FMACommHttpServer
{
public:
    FMACommHttpServer(UMACommSubsystem* InOwner);
    ~FMACommHttpServer();

    //=========================================================================
    // 服务器控制
    //=========================================================================

    /** 启动 HTTP 服务器 */
    void Start(int32 Port);

    /** 停止 HTTP 服务器 */
    void Stop();

    /** 服务器是否运行中 */
    bool IsRunning() const { return bServerRunning; }

private:
    //=========================================================================
    // 请求处理
    //=========================================================================

    /** 处理 /api/health 请求 */
    bool HandleHealthRequest(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);

    /** 处理 /api/world_state 请求 */
    bool HandleWorldStateRequest(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);

private:
    /** 所属的通信子系统 */
    UMACommSubsystem* Owner;

    /** HTTP 服务器路由句柄 */
    TArray<FHttpRouteHandle> HttpRouteHandles;

    /** 服务器是否运行中 */
    bool bServerRunning = false;

    /** 服务器端口 */
    int32 ServerPort = 8080;
};
