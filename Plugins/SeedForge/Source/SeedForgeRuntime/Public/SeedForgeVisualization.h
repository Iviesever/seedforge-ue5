#pragma once

#include "CoreMinimal.h"
#include "SeedForgeTypes.h"

struct SEEDFORGERUNTIME_API FSeedForgeVisualizationPlan
{
    TArray<FTransform> FloorTransforms;
    TArray<FTransform> WallTransforms;
    FBox WorldBounds = FBox(EForceInit::ForceInit);
};

class SEEDFORGERUNTIME_API FSeedForgeVisualizationPlanner
{
public:
    static FSeedForgeVisualizationPlan Build(
        const FSeedForgeLayout& Layout,
        float CellSize,
        float FloorThickness,
        float WallHeight,
        float WallThickness);
};
