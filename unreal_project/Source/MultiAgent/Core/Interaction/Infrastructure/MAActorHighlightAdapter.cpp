// Actor highlight and recursive root/child traversal.

#include "MAActorHighlightAdapter.h"

#include "Components/PrimitiveComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

AActor* FMAActorHighlightAdapter::FindRootActor(AActor* Actor) const
{
    if (!Actor)
    {
        return nullptr;
    }

    AActor* Current = Actor;
    AActor* Parent = Current->GetAttachParentActor();
    while (Parent)
    {
        Current = Parent;
        Parent = Current->GetAttachParentActor();
    }

    return Current;
}

void FMAActorHighlightAdapter::CollectActorTree(AActor* RootActor, TArray<AActor*>& OutActors) const
{
    if (!RootActor)
    {
        return;
    }

    OutActors.Add(RootActor);

    TArray<AActor*> AttachedActors;
    RootActor->GetAttachedActors(AttachedActors);
    for (AActor* Child : AttachedActors)
    {
        CollectActorTree(Child, OutActors);
    }
}

void FMAActorHighlightAdapter::SetActorTreeHighlight(AActor* Actor, bool bHighlight) const
{
    if (!Actor)
    {
        return;
    }

    TArray<AActor*> ActorTree;
    CollectActorTree(FindRootActor(Actor), ActorTree);
    for (AActor* ActorNode : ActorTree)
    {
        SetSingleActorHighlight(ActorNode, bHighlight);
    }
}

void FMAActorHighlightAdapter::SetSingleActorHighlight(AActor* Actor, bool bHighlight) const
{
    if (!Actor)
    {
        return;
    }

    TArray<UPrimitiveComponent*> Components;
    Actor->GetComponents<UPrimitiveComponent>(Components);

    auto CreateHighlightMaterial = [](UObject* Outer, const FLinearColor& Color) -> UMaterialInstanceDynamic*
    {
        static UMaterial* UnlitMaterial = nullptr;
        if (!UnlitMaterial)
        {
            UnlitMaterial = LoadObject<UMaterial>(nullptr,
                TEXT("/Engine/EngineMaterials/DefaultDeferredDecalMaterial.DefaultDeferredDecalMaterial"));

            if (!UnlitMaterial)
            {
                UnlitMaterial = LoadObject<UMaterial>(nullptr,
                    TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"));
            }
        }

        if (!UnlitMaterial)
        {
            return nullptr;
        }

        UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(UnlitMaterial, Outer);
        if (!DynMat)
        {
            return nullptr;
        }

        DynMat->SetVectorParameterValue(TEXT("BaseColor"), Color);
        DynMat->SetVectorParameterValue(TEXT("Color"), Color);
        return DynMat;
    };

    const FLinearColor HighlightColor(0.0f, 1.0f, 0.5f);

    for (UPrimitiveComponent* Component : Components)
    {
        if (!Component)
        {
            continue;
        }

        Component->SetRenderCustomDepth(bHighlight);
        Component->SetCustomDepthStencilValue(bHighlight ? 1 : 0);

        if (UMeshComponent* MeshComp = Cast<UMeshComponent>(Component))
        {
            if (bHighlight)
            {
                if (UMaterialInstanceDynamic* DynMat = CreateHighlightMaterial(MeshComp, HighlightColor))
                {
                    MeshComp->SetOverlayMaterial(DynMat);
                }
            }
            else
            {
                MeshComp->SetOverlayMaterial(nullptr);
            }
        }
    }
}
