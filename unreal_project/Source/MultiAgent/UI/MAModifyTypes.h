// MAModifyTypes.h
// Modify 模式相关类型定义
// 用于多选标注功能的数据结构
// Requirements: 3.1, 3.2, 3.3, 4.1, 5.1, 5.2, 5.3

#pragma once

#include "CoreMinimal.h"
#include "MAModifyTypes.generated.h"

/**
 * 节点分类枚举
 * 用于标识场景图节点的分类类型
 * Requirements: 5.1, 5.2, 5.3
 */
UENUM(BlueprintType)
enum class EMANodeCategory : uint8
{
    /** 未指定分类 */
    None            UMETA(DisplayName = "None"),
    
    /** 建筑类型 - building, residence, hospital, warehouse */
    Building        UMETA(DisplayName = "Building"),
    
    /** 交通设施类型 - bridge, street_segment, road_segment, intersection */
    TransFacility   UMETA(DisplayName = "Trans Facility"),
    
    /** 道具类型 - antenna, watertower, statue, tree, etc. */
    Prop            UMETA(DisplayName = "Prop")
};

/**
 * 形状类型枚举
 * 用于标识多选标注的几何形状类型
 * Requirements: 4.1, 5.1, 2.5
 */
UENUM(BlueprintType)
enum class EMAShapeType : uint8
{
    /** 点类型 - 单个位置 */
    Point       UMETA(DisplayName = "Point"),
    
    /** 多边形类型 - 用于建筑物组等区域实体 */
    Polygon     UMETA(DisplayName = "Polygon"),
    
    /** 线串类型 - 用于道路等线性实体 */
    LineString  UMETA(DisplayName = "LineString"),
    
    /** 矩形类型 - 用于简单的矩形区域 */
    Rectangle   UMETA(DisplayName = "Rectangle"),
    
    /** 棱柱类型 - 用于建筑物的 3D 表示 (底面多边形 + 高度) */
    Prism       UMETA(DisplayName = "Prism")
};

/**
 * 标注输入解析结果
 * 用于存储用户输入的标注信息
 * 
 * 输入格式:
 * - 单选: "id:value, type:value"
 * - 多选 (Polygon): "id:value, type:value, shape:polygon"
 * - 多选 (LineString): "id:value, type:value, shape:linestring"
 * - 新格式: "cate:building, type:value" (自动分配 ID)
 * - 新格式: "cate:trans_facility, type:value, shape:polygon"
 * 
 * Requirements: 3.1, 3.2, 3.3, 5.1, 5.2, 5.3
 */
USTRUCT(BlueprintType)
struct MULTIAGENT_API FMAAnnotationInput
{
    GENERATED_BODY()

    /** 用户指定的 ID (可选，如果为空则自动分配) */
    UPROPERTY(BlueprintReadWrite, Category = "Annotation")
    FString Id;

    /** 节点分类 (building, trans_facility, prop) */
    UPROPERTY(BlueprintReadWrite, Category = "Annotation")
    EMANodeCategory Category;

    /** 实体类型 (building_group, road_segment, etc.) */
    UPROPERTY(BlueprintReadWrite, Category = "Annotation")
    FString Type;

    /** 形状类型 (polygon, linestring, 空表示单选) */
    UPROPERTY(BlueprintReadWrite, Category = "Annotation")
    FString Shape;

    /** 额外属性 */
    UPROPERTY(BlueprintReadWrite, Category = "Annotation")
    TMap<FString, FString> Properties;

    //=========================================================================
    // 构造函数
    //=========================================================================

    FMAAnnotationInput()
        : Category(EMANodeCategory::None)
    {
    }

    FMAAnnotationInput(const FString& InId, const FString& InType, const FString& InShape = TEXT(""))
        : Id(InId)
        , Category(EMANodeCategory::None)
        , Type(InType)
        , Shape(InShape)
    {
    }

    FMAAnnotationInput(const FString& InId, EMANodeCategory InCategory, const FString& InType, const FString& InShape = TEXT(""))
        : Id(InId)
        , Category(InCategory)
        , Type(InType)
        , Shape(InShape)
    {
    }

    //=========================================================================
    // Category 辅助方法
    //=========================================================================

    /**
     * 从字符串解析 Category
     * @param CategoryStr 分类字符串 (building, trans_facility, prop)
     * @return 对应的 EMANodeCategory 枚举值
     * Requirements: 5.1, 5.2, 5.3
     */
    static EMANodeCategory ParseCategoryFromString(const FString& CategoryStr)
    {
        if (CategoryStr.Equals(TEXT("building"), ESearchCase::IgnoreCase))
        {
            return EMANodeCategory::Building;
        }
        else if (CategoryStr.Equals(TEXT("trans_facility"), ESearchCase::IgnoreCase))
        {
            return EMANodeCategory::TransFacility;
        }
        else if (CategoryStr.Equals(TEXT("prop"), ESearchCase::IgnoreCase))
        {
            return EMANodeCategory::Prop;
        }
        return EMANodeCategory::None;
    }

    /**
     * 将 Category 枚举转换为字符串
     * @param InCategory 分类枚举值
     * @return 对应的字符串表示
     * Requirements: 5.1, 5.2, 5.3
     */
    static FString CategoryToString(EMANodeCategory InCategory)
    {
        switch (InCategory)
        {
        case EMANodeCategory::Building:
            return TEXT("building");
        case EMANodeCategory::TransFacility:
            return TEXT("trans_facility");
        case EMANodeCategory::Prop:
            return TEXT("prop");
        case EMANodeCategory::None:
        default:
            return TEXT("");
        }
    }

    /**
     * 获取当前 Category 的字符串表示
     * @return 当前分类的字符串
     */
    FORCEINLINE FString GetCategoryString() const
    {
        return CategoryToString(Category);
    }

    /**
     * 设置 Category (从字符串)
     * @param CategoryStr 分类字符串
     */
    FORCEINLINE void SetCategoryFromString(const FString& CategoryStr)
    {
        Category = ParseCategoryFromString(CategoryStr);
    }

    /**
     * 检查是否有有效的 Category
     * @return 如果 Category 不为 None 则返回 true
     */
    FORCEINLINE bool HasCategory() const
    {
        return Category != EMANodeCategory::None;
    }

    /**
     * 检查是否为 Building 类型
     */
    FORCEINLINE bool IsBuilding() const
    {
        return Category == EMANodeCategory::Building;
    }

    /**
     * 检查是否为 TransFacility 类型
     */
    FORCEINLINE bool IsTransFacility() const
    {
        return Category == EMANodeCategory::TransFacility;
    }

    /**
     * 检查是否为 Prop 类型
     */
    FORCEINLINE bool IsProp() const
    {
        return Category == EMANodeCategory::Prop;
    }

    //=========================================================================
    // 辅助方法
    //=========================================================================

    /**
     * 检查是否为多选模式
     * @return 如果 Shape 非空则为多选模式
     */
    FORCEINLINE bool IsMultiSelect() const
    {
        return !Shape.IsEmpty();
    }

    /**
     * 检查是否为 Polygon 类型
     * @return 如果 Shape 为 "polygon" (不区分大小写)
     */
    FORCEINLINE bool IsPolygon() const
    {
        return Shape.Equals(TEXT("polygon"), ESearchCase::IgnoreCase);
    }

    /**
     * 检查是否为 LineString 类型
     * @return 如果 Shape 为 "linestring" (不区分大小写)
     */
    FORCEINLINE bool IsLineString() const
    {
        return Shape.Equals(TEXT("linestring"), ESearchCase::IgnoreCase);
    }

    /**
     * 检查是否为 Rectangle 类型
     * @return 如果 Shape 为 "rectangle" (不区分大小写)
     */
    FORCEINLINE bool IsRectangle() const
    {
        return Shape.Equals(TEXT("rectangle"), ESearchCase::IgnoreCase);
    }

    /**
     * 检查是否为 Point 类型
     * @return 如果 Shape 为 "point" (不区分大小写)
     */
    FORCEINLINE bool IsPoint() const
    {
        return Shape.Equals(TEXT("point"), ESearchCase::IgnoreCase);
    }

    /**
     * 检查是否为 Prism 类型
     * @return 如果 Shape 为 "prism" (不区分大小写)
     * Requirements: 2.5
     */
    FORCEINLINE bool IsPrism() const
    {
        return Shape.Equals(TEXT("prism"), ESearchCase::IgnoreCase);
    }

    /**
     * 获取形状类型枚举值
     * @return 对应的 EMAShapeType 枚举值
     */
    EMAShapeType GetShapeType() const
    {
        if (IsPolygon())
        {
            return EMAShapeType::Polygon;
        }
        else if (IsLineString())
        {
            return EMAShapeType::LineString;
        }
        else if (IsRectangle())
        {
            return EMAShapeType::Rectangle;
        }
        else if (IsPrism())
        {
            return EMAShapeType::Prism;
        }
        return EMAShapeType::Point;
    }

    /**
     * 检查输入是否有效
     * @return 如果 Type 非空且 (Id 非空或 Category 有效) 则有效
     * 新格式允许 Id 为空 (自动分配)，但需要有效的 Category
     */
    FORCEINLINE bool IsValid() const
    {
        // Type 必须非空
        if (Type.IsEmpty())
        {
            return false;
        }
        // 旧格式: Id 非空
        // 新格式: Category 有效 (Id 可以为空，会自动分配)
        return !Id.IsEmpty() || HasCategory();
    }

    /**
     * 重置所有字段
     */
    void Reset()
    {
        Id.Empty();
        Category = EMANodeCategory::None;
        Type.Empty();
        Shape.Empty();
        Properties.Empty();
    }

    /**
     * 转换为调试字符串
     * @return 格式化的调试信息
     */
    FString ToString() const
    {
        return FString::Printf(TEXT("FMAAnnotationInput(Id=%s, Category=%s, Type=%s, Shape=%s, IsMultiSelect=%s)"),
            *Id, *GetCategoryString(), *Type, *Shape, IsMultiSelect() ? TEXT("true") : TEXT("false"));
    }
};
