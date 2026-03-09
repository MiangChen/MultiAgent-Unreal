#pragma once

#include "CoreMinimal.h"

struct FMAModifySelectionState
{
    TArray<TWeakObjectPtr<AActor>> HighlightedActors;

    void Reset()
    {
        HighlightedActors.Empty();
    }

    int32 Num() const
    {
        int32 Count = 0;
        for (const TWeakObjectPtr<AActor>& Actor : HighlightedActors)
        {
            if (Actor.IsValid())
            {
                Count++;
            }
        }
        return Count;
    }

    bool Contains(AActor* Actor) const
    {
        for (const TWeakObjectPtr<AActor>& Candidate : HighlightedActors)
        {
            if (Candidate.Get() == Actor)
            {
                return true;
            }
        }
        return false;
    }

    void Add(AActor* Actor)
    {
        if (Actor && !Contains(Actor))
        {
            HighlightedActors.Add(Actor);
        }
    }

    int32 Remove(AActor* Actor)
    {
        int32 Removed = 0;
        for (int32 Index = HighlightedActors.Num() - 1; Index >= 0; --Index)
        {
            if (HighlightedActors[Index].Get() == Actor)
            {
                HighlightedActors.RemoveAt(Index);
                Removed++;
            }
        }
        return Removed;
    }

    TArray<AActor*> ToRawArray() const
    {
        TArray<AActor*> Result;
        for (const TWeakObjectPtr<AActor>& Actor : HighlightedActors)
        {
            if (Actor.IsValid())
            {
                Result.Add(Actor.Get());
            }
        }
        return Result;
    }

    AActor* GetFirst() const
    {
        for (const TWeakObjectPtr<AActor>& Actor : HighlightedActors)
        {
            if (Actor.IsValid())
            {
                return Actor.Get();
            }
        }
        return nullptr;
    }
};
