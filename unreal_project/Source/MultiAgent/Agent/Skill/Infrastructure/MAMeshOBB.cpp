// MAMeshOBB.cpp

#include "MAMeshOBB.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Rendering/PositionVertexBuffer.h"
#include "StaticMeshResources.h"

namespace
{
    /**
     * 3x3 对称矩阵 Jacobi 特征值分解。
     * 输入：A（in-place 修改），输出：V（特征向量为列向量），返回特征值（对角线）。
     * 算法是教科书 Jacobi 旋转，对 3x3 对称矩阵收敛极快（< 20 次迭代）。
     */
    void JacobiEigenDecomposition3x3(double A[3][3], double V[3][3], double EigVals[3])
    {
        // V 初始化为单位矩阵
        for (int32 i = 0; i < 3; ++i)
        {
            for (int32 j = 0; j < 3; ++j)
            {
                V[i][j] = (i == j) ? 1.0 : 0.0;
            }
        }

        const int32 MaxIterations = 50;
        const double Epsilon = 1e-12;

        for (int32 Iter = 0; Iter < MaxIterations; ++Iter)
        {
            // 找到非对角线绝对值最大的元素 (p, q)
            double MaxOff = 0.0;
            int32 P = 0, Q = 1;
            for (int32 i = 0; i < 3; ++i)
            {
                for (int32 j = i + 1; j < 3; ++j)
                {
                    const double Abs = FMath::Abs(A[i][j]);
                    if (Abs > MaxOff)
                    {
                        MaxOff = Abs;
                        P = i;
                        Q = j;
                    }
                }
            }

            if (MaxOff < Epsilon)
            {
                break;
            }

            // 计算旋转角
            const double App = A[P][P];
            const double Aqq = A[Q][Q];
            const double Apq = A[P][Q];
            const double Theta = (Aqq - App) / (2.0 * Apq);
            double T;
            if (FMath::Abs(Theta) < 1e15)
            {
                T = (Theta >= 0.0 ? 1.0 : -1.0) / (FMath::Abs(Theta) + FMath::Sqrt(Theta * Theta + 1.0));
            }
            else
            {
                T = 1.0 / (2.0 * Theta);
            }
            const double C = 1.0 / FMath::Sqrt(T * T + 1.0);
            const double S = T * C;

            // 旋转 A
            const double NewApp = App - T * Apq;
            const double NewAqq = Aqq + T * Apq;
            A[P][P] = NewApp;
            A[Q][Q] = NewAqq;
            A[P][Q] = 0.0;
            A[Q][P] = 0.0;

            for (int32 i = 0; i < 3; ++i)
            {
                if (i != P && i != Q)
                {
                    const double Aip = A[i][P];
                    const double Aiq = A[i][Q];
                    A[i][P] = C * Aip - S * Aiq;
                    A[P][i] = A[i][P];
                    A[i][Q] = S * Aip + C * Aiq;
                    A[Q][i] = A[i][Q];
                }
            }

            // 累乘特征向量矩阵 V
            for (int32 i = 0; i < 3; ++i)
            {
                const double Vip = V[i][P];
                const double Viq = V[i][Q];
                V[i][P] = C * Vip - S * Viq;
                V[i][Q] = S * Vip + C * Viq;
            }
        }

        for (int32 i = 0; i < 3; ++i)
        {
            EigVals[i] = A[i][i];
        }
    }

    /** 收集 Actor 下所有 StaticMeshComponent 的 LOD0 顶点（采样后转到世界空间） */
    bool CollectWorldVertices(const AActor& Actor, int32 VertexStride, TArray<FVector>& OutVerts)
    {
        TArray<UStaticMeshComponent*> MeshComps;
        Actor.GetComponents<UStaticMeshComponent>(MeshComps);
        if (MeshComps.IsEmpty())
        {
            return false;
        }

        const int32 Stride = FMath::Max(1, VertexStride);

        for (const UStaticMeshComponent* MeshComp : MeshComps)
        {
            if (!MeshComp || !MeshComp->IsVisible())
            {
                continue;
            }
            UStaticMesh* Mesh = MeshComp->GetStaticMesh();
            if (!Mesh)
            {
                continue;
            }

            const FStaticMeshRenderData* RenderData = Mesh->GetRenderData();
            if (!RenderData || RenderData->LODResources.Num() == 0)
            {
                continue;
            }

            const FStaticMeshLODResources& LOD = RenderData->LODResources[0];
            const FPositionVertexBuffer& PosBuffer = LOD.VertexBuffers.PositionVertexBuffer;
            const int32 NumVerts = static_cast<int32>(PosBuffer.GetNumVertices());
            if (NumVerts == 0)
            {
                continue;
            }

            const FTransform& Xform = MeshComp->GetComponentTransform();

            for (int32 i = 0; i < NumVerts; i += Stride)
            {
                const FVector LocalPos(PosBuffer.VertexPosition(i));
                OutVerts.Add(Xform.TransformPosition(LocalPos));
            }
        }

        return OutVerts.Num() >= 4;
    }
}

FMAMeshOBB ComputeMeshOBBFromActor(const AActor& Actor, int32 VertexStride)
{
    FMAMeshOBB OBB;

    TArray<FVector> Verts;
    if (!CollectWorldVertices(Actor, VertexStride, Verts))
    {
        return OBB;
    }

    // 1) 质心
    FVector Centroid = FVector::ZeroVector;
    for (const FVector& V : Verts)
    {
        Centroid += V;
    }
    Centroid /= static_cast<double>(Verts.Num());

    // 2) 协方差矩阵（用 double 累计避免 cm 量级下的精度损失）
    double Cov[3][3] = { {0,0,0}, {0,0,0}, {0,0,0} };
    for (const FVector& V : Verts)
    {
        const double DX = V.X - Centroid.X;
        const double DY = V.Y - Centroid.Y;
        const double DZ = V.Z - Centroid.Z;
        Cov[0][0] += DX * DX;
        Cov[0][1] += DX * DY;
        Cov[0][2] += DX * DZ;
        Cov[1][1] += DY * DY;
        Cov[1][2] += DY * DZ;
        Cov[2][2] += DZ * DZ;
    }
    Cov[1][0] = Cov[0][1];
    Cov[2][0] = Cov[0][2];
    Cov[2][1] = Cov[1][2];

    const double InvN = 1.0 / static_cast<double>(Verts.Num());
    for (int32 i = 0; i < 3; ++i)
    {
        for (int32 j = 0; j < 3; ++j)
        {
            Cov[i][j] *= InvN;
        }
    }

    // 3) 特征值分解
    double EigVecs[3][3];
    double EigVals[3];
    JacobiEigenDecomposition3x3(Cov, EigVecs, EigVals);

    // 4) 按特征值降序排序（最长主轴排在 0）
    int32 Order[3] = { 0, 1, 2 };
    for (int32 i = 0; i < 2; ++i)
    {
        for (int32 j = i + 1; j < 3; ++j)
        {
            if (EigVals[Order[j]] > EigVals[Order[i]])
            {
                Swap(Order[i], Order[j]);
            }
        }
    }

    // 协方差矩阵奇异（所有特征值都 ≈ 0）：mesh 退化为单点 / 共线
    if (EigVals[Order[0]] < 1e-6)
    {
        return OBB;
    }

    // 5) 主轴 + 投影找 OBB 范围
    FVector Axes[3];
    for (int32 k = 0; k < 3; ++k)
    {
        const int32 Col = Order[k];
        Axes[k] = FVector(EigVecs[0][Col], EigVecs[1][Col], EigVecs[2][Col]).GetSafeNormal();
    }

    // 保证右手坐标系（避免特征向量出现镜像）
    if (FVector::DotProduct(FVector::CrossProduct(Axes[0], Axes[1]), Axes[2]) < 0.f)
    {
        Axes[2] = -Axes[2];
    }

    double MinProj[3] = {  TNumericLimits<double>::Max(),  TNumericLimits<double>::Max(),  TNumericLimits<double>::Max() };
    double MaxProj[3] = { -TNumericLimits<double>::Max(), -TNumericLimits<double>::Max(), -TNumericLimits<double>::Max() };

    for (const FVector& V : Verts)
    {
        const FVector Rel = V - Centroid;
        for (int32 k = 0; k < 3; ++k)
        {
            const double Proj = FVector::DotProduct(Rel, Axes[k]);
            MinProj[k] = FMath::Min(MinProj[k], Proj);
            MaxProj[k] = FMath::Max(MaxProj[k], Proj);
        }
    }

    // 6) 中心 = 质心 + 沿三轴的中点偏移；半尺寸 = (max - min) / 2
    FVector Center = Centroid;
    for (int32 k = 0; k < 3; ++k)
    {
        const double Mid = 0.5 * (MaxProj[k] + MinProj[k]);
        Center += Axes[k] * Mid;
        OBB.Extent[k] = static_cast<float>(0.5 * (MaxProj[k] - MinProj[k]));
        OBB.Axis[k] = Axes[k];
    }
    OBB.Center = Center;
    OBB.bValid = true;

    return OBB;
}
