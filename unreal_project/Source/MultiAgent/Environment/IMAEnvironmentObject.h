// IMAEnvironmentObject.h
// 环境对象接口 - 所有可被场景查询系统查找的环境对象都应实现此接口

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IMAEnvironmentObject.generated.h"

// 前向声明
struct FMASemanticLabel;
class AMAFire;

UINTERFACE(MinimalAPI, Blueprintable)
class UMAEnvironmentObject : public UInterface
{
    GENERATED_BODY()
};

/**
 * 环境对象接口
 * 所有可被场景查询系统查找的环境对象都应实现此接口
 * 
 * 实现此接口的类可以通过 FMAUESceneQuery 进行统一查询，
 * 支持按标签、类型、特征等条件进行匹配。
 * 
 * 典型实现类:
 * - AMACargo (货物)
 * - AMAComponent (组装组件)
 * - MAPerson (人类 NPC)
 * - MAVehicle (车辆) - 未来扩展
 * - MAHazard (危险物) - 未来扩展
 * 
 * 设计原则:
 * - Actor 只存储数据 (ObjectLabel, ObjectType, Features)
 * - 数据的设置通过 MASceneGraphManager 委托 MADynamicNodeManager 完成
 * - 接口提供只读访问方法
 */
class MULTIAGENT_API IMAEnvironmentObject
{
    GENERATED_BODY()

public:
    //=========================================================================
    // 核心属性访问 (纯虚方法 - 子类必须实现)
    //=========================================================================

    /** 
     * 获取对象标签 (用于显示和匹配)
     * @return 对象的语义标签字符串
     */
    virtual FString GetObjectLabel() const = 0;
    
    /** 
     * 获取对象类型 (cargo, person, vehicle, hazard 等)
     * @return 对象类型字符串
     */
    virtual FString GetObjectType() const = 0;
    
    /** 
     * 获取对象特征 (color, subtype, status 等键值对)
     * @return 特征键值对映射的常量引用
     */
    virtual const TMap<FString, FString>& GetObjectFeatures() const = 0;

    //=========================================================================
    // 特征查询 (提供默认实现)
    //=========================================================================
    
    /** 
     * 获取特征值
     * @param Key 特征键
     * @param DefaultValue 默认值 (当特征不存在时返回)
     * @return 特征值或默认值
     */
    virtual FString GetFeature(const FString& Key, const FString& DefaultValue = FString()) const;
    
    /** 
     * 检查是否有特征
     * @param Key 特征键
     * @return 是否存在该特征
     */
    virtual bool HasFeature(const FString& Key) const;

    //=========================================================================
    // 匹配逻辑 (提供默认实现)
    //=========================================================================
    
    /** 
     * 检查是否匹配语义标签
     * 
     * 默认实现的匹配规则:
     * 1. 如果查询标签的 Type 非空，检查是否与对象类型匹配
     * 2. 查询标签的所有 Features 必须在对象中存在且值匹配 (忽略大小写)
     * 3. 特别处理 "label" 和 "name" 特征，与 GetObjectLabel() 进行匹配
     * 
     * @param Label 要匹配的语义标签
     * @return 是否匹配
     */
    virtual bool MatchesSemanticLabel(const struct FMASemanticLabel& Label) const;

    //=========================================================================
    // 附着火焰管理 (提供默认实现)
    //=========================================================================
    
    /**
     * 设置附着的火焰对象
     * @param Fire 火焰对象指针
     */
    virtual void SetAttachedFire(AMAFire* Fire) { /* 默认空实现 */ }
    
    /**
     * 获取附着的火焰对象
     * @return 火焰对象指针，如果没有则返回 nullptr
     */
    virtual AMAFire* GetAttachedFire() const { return nullptr; }
    
    /**
     * 是否有附着的火焰
     * @return 是否有附着火焰
     */
    virtual bool HasAttachedFire() const { return GetAttachedFire() != nullptr; }
};
