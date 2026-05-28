// MACommTypeHelper.cpp
// L3 Infrastructure: 通信类型字符串映射与分类策略

#include "MACommTypeHelper.h"

DEFINE_LOG_CATEGORY(LogMACommTypes);

namespace MACommTypeHelpers
{
    FString MessageTypeToString(EMACommMessageType Type)
    {
        switch (Type)
        {
        case EMACommMessageType::UIInput:         return TEXT("ui_input");
        case EMACommMessageType::ButtonEvent:     return TEXT("button_event");
        case EMACommMessageType::TaskFeedback:    return TEXT("task_feedback");
        case EMACommMessageType::WorldState:      return TEXT("world_state");
        case EMACommMessageType::SceneChange:     return TEXT("scene_change");
        case EMACommMessageType::TaskGraph:       return TEXT("task_graph");
        case EMACommMessageType::WorldModelGraph: return TEXT("world_model_graph");
        case EMACommMessageType::SkillList:       return TEXT("skill_list");
        case EMACommMessageType::QueryRequest:    return TEXT("query_request");
        case EMACommMessageType::SkillAllocation: return TEXT("skill_allocation");
        case EMACommMessageType::ReviewResponse:  return TEXT("review_response");
        case EMACommMessageType::DecisionResponse:return TEXT("decision_response");
        case EMACommMessageType::SkillStatusUpdate: return TEXT("skill_status_update");
        case EMACommMessageType::Custom:
        default:                                  return TEXT("custom");
        }
    }

    EMACommMessageType StringToMessageType(const FString& TypeStr)
    {
        if (TypeStr == TEXT("ui_input"))           return EMACommMessageType::UIInput;
        if (TypeStr == TEXT("user_instruction"))   return EMACommMessageType::UIInput;
        if (TypeStr == TEXT("button_event"))       return EMACommMessageType::ButtonEvent;
        if (TypeStr == TEXT("task_feedback"))      return EMACommMessageType::TaskFeedback;
        if (TypeStr == TEXT("world_state"))        return EMACommMessageType::WorldState;
        if (TypeStr == TEXT("scene_change"))       return EMACommMessageType::SceneChange;
        if (TypeStr == TEXT("task_graph"))         return EMACommMessageType::TaskGraph;
        if (TypeStr == TEXT("world_model_graph"))  return EMACommMessageType::WorldModelGraph;
        if (TypeStr == TEXT("skill_list"))         return EMACommMessageType::SkillList;
        if (TypeStr == TEXT("query_request"))      return EMACommMessageType::QueryRequest;
        if (TypeStr == TEXT("skill_allocation"))   return EMACommMessageType::SkillAllocation;
        if (TypeStr == TEXT("review_response"))    return EMACommMessageType::ReviewResponse;
        if (TypeStr == TEXT("decision_response"))  return EMACommMessageType::DecisionResponse;
        if (TypeStr == TEXT("skill_status_update")) return EMACommMessageType::SkillStatusUpdate;
        return EMACommMessageType::Custom;
    }

    //=========================================================================
    // 消息类别枚举转换函数 - 匹配 Python 端 MessageCategory
    //=========================================================================

    FString MessageCategoryToString(EMAMessageCategory Category)
    {
        switch (Category)
        {
        case EMAMessageCategory::Instruction: return TEXT("instruction");
        case EMAMessageCategory::Review:      return TEXT("review");
        case EMAMessageCategory::Decision:    return TEXT("decision");
        case EMAMessageCategory::Platform:
        default:                              return TEXT("platform");
        }
    }

    EMAMessageCategory StringToMessageCategory(const FString& CategoryStr)
    {
        if (CategoryStr == TEXT("instruction")) return EMAMessageCategory::Instruction;
        if (CategoryStr == TEXT("review"))      return EMAMessageCategory::Review;
        if (CategoryStr == TEXT("decision"))    return EMAMessageCategory::Decision;
        if (CategoryStr == TEXT("platform"))    return EMAMessageCategory::Platform;
        return EMAMessageCategory::Platform; // 默认值
    }

    //=========================================================================
    // 消息方向枚举转换函数 - 匹配 Python 端 MessageDirection
    //=========================================================================

    FString MessageDirectionToString(EMAMessageDirection Direction)
    {
        switch (Direction)
        {
        case EMAMessageDirection::PythonToUE5: return TEXT("python_to_ue5");
        case EMAMessageDirection::UE5ToPython:
        default:                               return TEXT("ue5_to_python");
        }
    }

    EMAMessageDirection StringToMessageDirection(const FString& DirectionStr)
    {
        if (DirectionStr == TEXT("python_to_ue5")) return EMAMessageDirection::PythonToUE5;
        if (DirectionStr == TEXT("ue5_to_python")) return EMAMessageDirection::UE5ToPython;
        return EMAMessageDirection::UE5ToPython; // 默认值
    }

    //=========================================================================
    // 消息类型到类别的映射函数
    // 根据 MessageType 推断 MessageCategory
    //=========================================================================

    EMAMessageCategory GetCategoryForMessageType(EMACommMessageType Type)
    {
        switch (Type)
        {
        // Instruction 类别: 用户指令输入
        case EMACommMessageType::UIInput:
        case EMACommMessageType::ButtonEvent:
            return EMAMessageCategory::Instruction;

        // Review 类别: 计划审阅请求/响应
        case EMACommMessageType::TaskGraph:
        case EMACommMessageType::SkillAllocation:
        case EMACommMessageType::ReviewResponse:
            return EMAMessageCategory::Review;

        case EMACommMessageType::DecisionResponse:
            return EMAMessageCategory::Decision;

        // Platform 类别: 平台消息 (默认)
        case EMACommMessageType::TaskFeedback:
        case EMACommMessageType::WorldState:
        case EMACommMessageType::SceneChange:
        case EMACommMessageType::WorldModelGraph:
        case EMACommMessageType::SkillList:
        case EMACommMessageType::QueryRequest:
        case EMACommMessageType::SkillStatusUpdate:
        case EMACommMessageType::Custom:
        default:
            return EMAMessageCategory::Platform;
        }
    }
}
