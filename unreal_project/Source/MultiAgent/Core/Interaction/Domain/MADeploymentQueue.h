#pragma once

#include "MAInputTypes.h"

struct FMADeploymentQueueState
{
    TArray<FMAPendingDeployment> Items;
    int32 CurrentIndex = 0;
    int32 DeployedCount = 0;

    int32 GetTotalCount() const
    {
        int32 Total = 0;
        for (const FMAPendingDeployment& Deployment : Items)
        {
            Total += Deployment.Count;
        }
        return Total;
    }

    bool HasPendingDeployments() const
    {
        return !Items.IsEmpty() && GetTotalCount() > 0;
    }
};
