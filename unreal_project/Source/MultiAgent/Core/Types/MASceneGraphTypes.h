// MASceneGraphTypes.h
// 场景图相关类型定义
// Requirements: 11.5, 11.6, 11.7

#pragma once

#include "CoreMinimal.h"
#include "MASceneGraphTypes.generated.h"

/**
 * 语义标签结构体
 * 用于描述场景中对象的语义信息，支持技能参数中的对象引用
 * 
 * JSON 格式: {"class": "...", "type": "...", "features": {"key": "value", ...}}
 * 
 * Requirements: 11.5, 11.6, 11.7
 * 
 * 使用示例:
 * - 引用机器人: {"class": "robot", "type": "UGV", "features": {"label": "UGV_01"}}
 * - 引用地面: {"class": "ground"}
 * - 引用物品: {"class": "pickup_item", "type": "box", "features": {"color": "red"}}
 */
USTRUCT(BlueprintType)
struct FMASemanticLabel
{
    GENERATED_BODY()

    //=========================================================================
    // 核心字段
    //=========================================================================

    /** 对象类别: robot, ground, pickup_item, building, prop, etc. */
    UPROPERTY(BlueprintReadWrite, Category = "SemanticLabel")
    FString Class;

    /** 对象类型: UAV, UGV, box, building, intersection, etc. */
    UPROPERTY(BlueprintReadWrite, Category = "SemanticLabel")
    FString Type;

    /** 特征属性: name, color, label, etc. */
    UPROPERTY(BlueprintReadWrite, Category = "SemanticLabel")
    TMap<FString, FString> Features;

    //=========================================================================
    // 构造函数
    //=========================================================================

    FMASemanticLabel()
    {
    }

    FMASemanticLabel(const FString& InClass)
        : Class(InClass)
    {
    }

    FMASemanticLabel(const FString& InClass, const FString& InType)
        : Class(InClass)
        , Type(InType)
    {
    }

    //=========================================================================
    // 辅助方法 - 状态检查
    //=========================================================================

    /** 标签是否为空 (没有任何有效信息) */
    bool IsEmpty() const
    {
        return Class.IsEmpty() && Type.IsEmpty() && Features.Num() == 0;
    }

    /** 是否引用机器人 */
    bool IsRobot() const
    {
        return Class.Equals(TEXT("robot"), ESearchCase::IgnoreCase);
    }

    /** 是否引用地面 */
    bool IsGround() const
    {
        return Class.Equals(TEXT("ground"), ESearchCase::IgnoreCase);
    }

    /** 是否引用可拾取物品 */
    bool IsPickupItem() const
    {
        return Class.Equals(TEXT("pickup_item"), ESearchCase::IgnoreCase);
    }

    /** 是否引用建筑物 */
    bool IsBuilding() const
    {
        return Class.Equals(TEXT("building"), ESearchCase::IgnoreCase);
    }

    /** 是否引用充电站 */
    bool IsChargingStation() const
    {
        return Class.Equals(TEXT("charging_station"), ESearchCase::IgnoreCase);
    }

    //=========================================================================
    // 辅助方法 - 特征访问
    //=========================================================================

    /** 获取名称特征 (features.name) */
    FString GetName() const
    {
        const FString* NamePtr = Features.Find(TEXT("name"));
        return NamePtr ? *NamePtr : FString();
    }

    /** 获取颜色特征 (features.color) */
    FString GetColor() const
    {
        const FString* ColorPtr = Features.Find(TEXT("color"));
        return ColorPtr ? *ColorPtr : FString();
    }

    /** 获取标签特征 (features.label) */
    FString GetLabel() const
    {
        const FString* LabelPtr = Features.Find(TEXT("label"));
        return LabelPtr ? *LabelPtr : FString();
    }

    /** 检查是否包含指定特征 */
    bool HasFeature(const FString& Key) const
    {
        return Features.Contains(Key);
    }

    /** 获取指定特征值 */
    FString GetFeature(const FString& Key, const FString& DefaultValue = FString()) const
    {
        const FString* ValuePtr = Features.Find(Key);
        return ValuePtr ? *ValuePtr : DefaultValue;
    }

    //=========================================================================
    // 辅助方法 - 匹配逻辑
    //=========================================================================

    /**
     * 检查另一个语义标签是否匹配当前标签
     * 
     * 匹配规则:
     * 1. 如果当前标签的 Class 非空，则目标的 Class 必须匹配 (忽略大小写)
     * 2. 如果当前标签的 Type 非空，则目标的 Type 必须匹配 (忽略大小写)
     * 3. 当前标签的所有 Features 必须在目标中存在且值匹配 (忽略大小写)
     * 
     * @param Other 要匹配的目标标签
     * @return 是否匹配
     */
    bool Matches(const FMASemanticLabel& Other) const
    {
        // 检查 Class
        if (!Class.IsEmpty() && !Class.Equals(Other.Class, ESearchCase::IgnoreCase))
        {
            return false;
        }

        // 检查 Type
        if (!Type.IsEmpty() && !Type.Equals(Other.Type, ESearchCase::IgnoreCase))
        {
            return false;
        }

        // 检查所有 Features
        for (const auto& Pair : Features)
        {
            const FString* OtherValue = Other.Features.Find(Pair.Key);
            if (!OtherValue || !Pair.Value.Equals(*OtherValue, ESearchCase::IgnoreCase))
            {
                return false;
            }
        }

        return true;
    }

    /**
     * 检查是否匹配指定的类别和类型
     * 
     * @param InClass 要匹配的类别 (空字符串表示不检查)
     * @param InType 要匹配的类型 (空字符串表示不检查)
     * @return 是否匹配
     */
    bool MatchesClassAndType(const FString& InClass, const FString& InType = FString()) const
    {
        if (!InClass.IsEmpty() && !Class.Equals(InClass, ESearchCase::IgnoreCase))
        {
            return false;
        }

        if (!InType.IsEmpty() && !Type.Equals(InType, ESearchCase::IgnoreCase))
        {
            return false;
        }

        return true;
    }

    //=========================================================================
    // 辅助方法 - 字符串转换
    //=========================================================================

    /** 转换为可读字符串 (用于日志) */
    FString ToString() const
    {
        FString Result = FString::Printf(TEXT("Class=%s, Type=%s"), *Class, *Type);
        
        if (Features.Num() > 0)
        {
            Result += TEXT(", Features={");
            bool bFirst = true;
            for (const auto& Pair : Features)
            {
                if (!bFirst)
                {
                    Result += TEXT(", ");
                }
                Result += FString::Printf(TEXT("%s=%s"), *Pair.Key, *Pair.Value);
                bFirst = false;
            }
            Result += TEXT("}");
        }
        
        return Result;
    }

    //=========================================================================
    // 静态工厂方法
    //=========================================================================

    /** 创建机器人引用标签 */
    static FMASemanticLabel MakeRobot(const FString& RobotLabel, const FString& AgentType = FString())
    {
        FMASemanticLabel Label(TEXT("robot"), AgentType);
        if (!RobotLabel.IsEmpty())
        {
            Label.Features.Add(TEXT("label"), RobotLabel);
        }
        return Label;
    }

    /** 创建地面引用标签 */
    static FMASemanticLabel MakeGround()
    {
        return FMASemanticLabel(TEXT("ground"));
    }

    /** 创建可拾取物品引用标签 */
    static FMASemanticLabel MakePickupItem(const FString& ItemLabel = FString(), const FString& Color = FString())
    {
        FMASemanticLabel Label(TEXT("pickup_item"));
        if (!ItemLabel.IsEmpty())
        {
            Label.Features.Add(TEXT("label"), ItemLabel);
        }
        if (!Color.IsEmpty())
        {
            Label.Features.Add(TEXT("color"), Color);
        }
        return Label;
    }

    /** 创建建筑物引用标签 */
    static FMASemanticLabel MakeBuilding(const FString& BuildingLabel = FString())
    {
        FMASemanticLabel Label(TEXT("building"));
        if (!BuildingLabel.IsEmpty())
        {
            Label.Features.Add(TEXT("label"), BuildingLabel);
        }
        return Label;
    }

    /** 创建充电站引用标签 */
    static FMASemanticLabel MakeChargingStation(const FString& StationLabel = FString())
    {
        FMASemanticLabel Label(TEXT("charging_station"));
        if (!StationLabel.IsEmpty())
        {
            Label.Features.Add(TEXT("label"), StationLabel);
        }
        return Label;
    }
};
