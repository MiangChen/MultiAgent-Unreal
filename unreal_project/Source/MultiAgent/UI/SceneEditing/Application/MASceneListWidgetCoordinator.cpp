#include "MASceneListWidgetCoordinator.h"

#include "../Presentation/MASceneListWidget.h"
#include "../../../Core/SceneGraph/Feedback/MASceneGraphFeedback.h"

namespace
{
FMASceneListSectionModel BuildSectionModel(const FMASceneGraphNodesFeedback& Feedback)
{
    FMASceneListSectionModel Model;
    if (!Feedback.bSuccess)
    {
        Model.EmptyText = Feedback.Message;
        return Model;
    }

    Model.CountText = FString::Printf(TEXT("(%d)"), Feedback.Nodes.Num());
    for (const FMASceneGraphNode& Node : Feedback.Nodes)
    {
        FMASceneListItemModel Item;
        Item.Id = Node.Id;
        Item.Label = Node.Label.IsEmpty() ? Node.Id : Node.Label;
        Model.Items.Add(MoveTemp(Item));
    }

    return Model;
}
}

FMASceneListSectionModel FMASceneListWidgetCoordinator::BuildGoalSection(UMASceneListWidget* Widget) const
{
    return Widget ? BuildSectionModel(Widget->RuntimeLoadGoals()) : FMASceneListSectionModel();
}

FMASceneListSectionModel FMASceneListWidgetCoordinator::BuildZoneSection(UMASceneListWidget* Widget) const
{
    return Widget ? BuildSectionModel(Widget->RuntimeLoadZones()) : FMASceneListSectionModel();
}
