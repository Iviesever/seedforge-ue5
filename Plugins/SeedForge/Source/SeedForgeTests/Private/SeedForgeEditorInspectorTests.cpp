#include "Framework/Docking/TabManager.h"
#include "Misc/AutomationTest.h"
#include "Modules/ModuleManager.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSeedForgeInspectorTabRegisteredTest,
    "SeedForge.Editor.InspectorTabRegistered",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSeedForgeInspectorTabRegisteredTest::RunTest(const FString& Parameters)
{
    TestTrue(
        TEXT("SeedForgeEditor module is loaded"),
        FModuleManager::Get().IsModuleLoaded(TEXT("SeedForgeEditor")));
    TestTrue(
        TEXT("SeedForge Inspector Nomad tab spawner is registered"),
        FGlobalTabmanager::Get()->HasTabSpawner(FName(TEXT("SeedForgeInspector"))));
    return true;
}

#endif
