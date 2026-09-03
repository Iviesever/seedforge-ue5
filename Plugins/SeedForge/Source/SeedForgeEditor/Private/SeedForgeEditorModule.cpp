#include "Modules/ModuleManager.h"

#include "Containers/Ticker.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "ImageUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "SeedForgeGenerator.h"
#include "SeedForgeLayoutCodec.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

#define LOCTEXT_NAMESPACE "SeedForgeEditor"

DEFINE_LOG_CATEGORY_STATIC(LogSeedForgeInspector, Log, All);

namespace SeedForge::Inspector::Private
{
    const FName TabName(TEXT("SeedForgeInspector"));

    bool ParseUInt64(const FString& Text, uint64& Value)
    {
        if (Text.IsEmpty() || (Text.Len() > 1 && Text[0] == TEXT('0')))
        {
            return false;
        }
        uint64 Parsed = 0;
        for (const TCHAR Character : Text)
        {
            if (Character < TEXT('0') || Character > TEXT('9'))
            {
                return false;
            }
            const uint64 Digit = static_cast<uint64>(Character - TEXT('0'));
            if (Parsed > (MAX_uint64 - Digit) / 10ULL)
            {
                return false;
            }
            Parsed = Parsed * 10ULL + Digit;
        }
        Value = Parsed;
        return true;
    }

    class SSeedForgeInspector final : public SCompoundWidget
    {
    public:
        SLATE_BEGIN_ARGS(SSeedForgeInspector)
            : _InitialSeed(24301ULL)
        {
        }
            SLATE_ARGUMENT(uint64, InitialSeed)
        SLATE_END_ARGS()

        void Construct(const FArguments& Arguments)
        {
            ChildSlot
            [
                SNew(SBorder)
                .Padding(24.0f)
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(STextBlock)
                        .Text(LOCTEXT("Title", "SeedForge Inspector"))
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 22))
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 6.0f, 0.0f, 18.0f)
                    [
                        SNew(STextBlock)
                        .Text(LOCTEXT(
                            "Description",
                            "Generate one deterministic dungeon layout, inspect its identity, and export its canonical JSON evidence."))
                        .AutoWrapText(true)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(STextBlock)
                        .Text(LOCTEXT("SeedLabel", "Unsigned 64-bit seed"))
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 5.0f, 0.0f, 14.0f)
                    [
                        SAssignNew(SeedInput, SEditableTextBox)
                        .Text(FText::AsNumber(Arguments._InitialSeed, &FNumberFormattingOptions::DefaultNoGrouping()))
                        .HintText(LOCTEXT("SeedHint", "0 to 18446744073709551615"))
                        .OnTextCommitted(this, &SSeedForgeInspector::OnSeedCommitted)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        [
                            SNew(SButton)
                            .Text(LOCTEXT("GenerateButton", "Generate Layout"))
                            .ToolTipText(LOCTEXT("GenerateTooltip", "Run the deterministic C++ generator with the default configuration."))
                            .OnClicked(this, &SSeedForgeInspector::OnGenerate)
                        ]
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .Padding(10.0f, 0.0f, 0.0f, 0.0f)
                        [
                            SNew(SButton)
                            .Text(LOCTEXT("ExportButton", "Export Canonical JSON"))
                            .ToolTipText(LOCTEXT("ExportTooltip", "Write the current versioned layout document under Artifacts/Reports/Inspector."))
                            .IsEnabled_Lambda([this]() { return bHasDocument; })
                            .OnClicked(this, &SSeedForgeInspector::OnExport)
                        ]
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 20.0f, 0.0f, 16.0f)
                    [
                        SNew(SSeparator)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(STextBlock)
                        .Text(LOCTEXT("EvidenceLabel", "Current evidence"))
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 8.0f, 0.0f, 0.0f)
                    [
                        SNew(SBorder)
                        .Padding(16.0f)
                        [
                            SAssignNew(StatusText, STextBlock)
                            .Text(LOCTEXT("ReadyStatus", "Ready. Enter a seed and generate a layout."))
                            .AutoWrapText(true)
                        ]
                    ]
                ]
            ];

            GenerateSeed(Arguments._InitialSeed);
        }

    private:
        FReply OnGenerate()
        {
            uint64 Seed = 0;
            const FString SeedText = SeedInput.IsValid()
                ? SeedInput->GetText().ToString().TrimStartAndEnd()
                : FString();
            if (!ParseUInt64(SeedText, Seed))
            {
                bHasDocument = false;
                StatusText->SetText(LOCTEXT(
                    "InvalidSeedStatus",
                    "Invalid seed. Use decimal digits in the uint64 range with no sign or leading zeros."));
                return FReply::Handled();
            }

            return GenerateSeed(Seed);
        }

        FReply GenerateSeed(uint64 Seed)
        {
            FSeedForgeLayoutDocument Candidate;
            const double StartSeconds = FPlatformTime::Seconds();
            FSeedForgeResult Generated = FSeedForgeGenerator::Generate(Seed, Candidate.Config);
            const double ElapsedMilliseconds = (FPlatformTime::Seconds() - StartSeconds) * 1000.0;
            if (!Generated.IsSuccess())
            {
                bHasDocument = false;
                StatusText->SetText(FText::FromString(TEXT("Generation failed: ") + Generated.ErrorMessage));
                return FReply::Handled();
            }

            Candidate.Layout = MoveTemp(Generated.Layout);
            CurrentDocument = MoveTemp(Candidate);
            bHasDocument = true;
            const int32 WalkableCellCount = CurrentDocument.Layout.GetCanonicalWalkableCells().Num();
            StatusText->SetText(FText::FromString(FString::Printf(
                TEXT("Seed: %llu\nCanonical hash: %llu\nRooms: %d    Walkable cells: %d\nGeneration time: %.6f ms"),
                CurrentDocument.Layout.Seed,
                CurrentDocument.Layout.CanonicalHash,
                CurrentDocument.Layout.Rooms.Num(),
                WalkableCellCount,
                ElapsedMilliseconds)));
            return FReply::Handled();
        }

        FReply OnExport()
        {
            if (!bHasDocument)
            {
                return FReply::Handled();
            }

            const FString Directory = FPaths::Combine(
                FPaths::ProjectDir(),
                TEXT("Artifacts/Reports/Inspector"));
            const FString OutputPath = FPaths::Combine(
                Directory,
                FString::Printf(TEXT("layout-%llu.json"), CurrentDocument.Layout.Seed));
            IFileManager::Get().MakeDirectory(*Directory, true);
            const bool bSaved = FFileHelper::SaveStringToFile(
                FSeedForgeLayoutCodec::ExportCanonicalJson(CurrentDocument),
                *OutputPath,
                FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
            StatusText->SetText(FText::FromString(
                bSaved
                    ? TEXT("Exported canonical JSON:\n") + OutputPath
                    : TEXT("Export failed. Could not write:\n") + OutputPath));
            return FReply::Handled();
        }

        void OnSeedCommitted(const FText& Text, ETextCommit::Type CommitType)
        {
            if (CommitType == ETextCommit::OnEnter)
            {
                OnGenerate();
            }
        }

        TSharedPtr<SEditableTextBox> SeedInput;
        TSharedPtr<STextBlock> StatusText;
        FSeedForgeLayoutDocument CurrentDocument;
        bool bHasDocument = false;
    };
}

class FSeedForgeEditorModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
            SeedForge::Inspector::Private::TabName,
            FOnSpawnTab::CreateRaw(this, &FSeedForgeEditorModule::SpawnInspectorTab))
            .SetDisplayName(LOCTEXT("InspectorTabTitle", "SeedForge Inspector"))
            .SetTooltipText(LOCTEXT(
                "InspectorTabTooltip",
                "Inspect and export deterministic SeedForge layouts."))
            .SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsMiscCategory());

        if (FParse::Value(
                FCommandLine::Get(),
                TEXT("SeedForgeInspectorCapture="),
                PendingCapturePath)
            && !PendingCapturePath.IsEmpty())
        {
            FString SeedText;
            if (FParse::Value(FCommandLine::Get(), TEXT("SeedForgeInspectorSeed="), SeedText))
            {
                uint64 ParsedSeed = 0;
                if (SeedForge::Inspector::Private::ParseUInt64(SeedText, ParsedSeed))
                {
                    CaptureSeed = ParsedSeed;
                }
                else
                {
                    UE_LOG(
                        LogSeedForgeInspector,
                        Error,
                        TEXT("Invalid -SeedForgeInspectorSeed value '%s'."),
                        *SeedText);
                }
            }
            CaptureTicker = FTSTicker::GetCoreTicker().AddTicker(
                FTickerDelegate::CreateRaw(this, &FSeedForgeEditorModule::TickInspectorCapture),
                1.0f);
        }
    }

    virtual void ShutdownModule() override
    {
        if (CaptureTicker.IsValid())
        {
            FTSTicker::GetCoreTicker().RemoveTicker(CaptureTicker);
            CaptureTicker.Reset();
        }
        FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(
            SeedForge::Inspector::Private::TabName);
    }

private:
    TSharedRef<SDockTab> SpawnInspectorTab(const FSpawnTabArgs& Args)
    {
        TSharedRef<SDockTab> Tab = SNew(SDockTab)
            .TabRole(ETabRole::NomadTab)
            [
                SAssignNew(InspectorWidget, SeedForge::Inspector::Private::SSeedForgeInspector)
                .InitialSeed(CaptureSeed)
            ];
        InspectorTab = Tab;
        return Tab;
    }

    bool TickInspectorCapture(float DeltaSeconds)
    {
        ++CaptureAttempts;
        if (!InspectorTab.IsValid())
        {
            InspectorTab = FGlobalTabmanager::Get()->TryInvokeTab(
                SeedForge::Inspector::Private::TabName);
            return true;
        }

        const TSharedPtr<SeedForge::Inspector::Private::SSeedForgeInspector> Widget = InspectorWidget.Pin();
        const FVector2D LocalSize = Widget.IsValid()
            ? Widget->GetCachedGeometry().GetLocalSize()
            : FVector2D::ZeroVector;
        if (!Widget.IsValid() || LocalSize.X < 200.0 || LocalSize.Y < 200.0)
        {
            if (CaptureAttempts < 20)
            {
                return true;
            }
            UE_LOG(LogSeedForgeInspector, Error, TEXT("Inspector never reached a capturable layout size."));
            FPlatformMisc::RequestExit(false);
            return false;
        }

        TArray<FColor> Pixels;
        FIntVector ImageSize;
        if (!FSlateApplication::Get().TakeScreenshot(Widget.ToSharedRef(), Pixels, ImageSize))
        {
            if (CaptureAttempts < 20)
            {
                return true;
            }
            UE_LOG(LogSeedForgeInspector, Error, TEXT("FSlateApplication could not capture the Inspector widget."));
            FPlatformMisc::RequestExit(false);
            return false;
        }

        const FString AbsolutePath = FPaths::ConvertRelativePathToFull(PendingCapturePath);
        IFileManager::Get().MakeDirectory(*FPaths::GetPath(AbsolutePath), true);
        TArray64<uint8> PngBytes;
        FImageUtils::PNGCompressImageArray(ImageSize.X, ImageSize.Y, Pixels, PngBytes);
        if (!FFileHelper::SaveArrayToFile(PngBytes, *AbsolutePath))
        {
            UE_LOG(LogSeedForgeInspector, Error, TEXT("Could not save Inspector capture '%s'."), *AbsolutePath);
            FPlatformMisc::RequestExit(false);
            return false;
        }

        UE_LOG(
            LogSeedForgeInspector,
            Display,
            TEXT("Inspector capture passed path='%s' width=%d height=%d bytes=%lld."),
            *AbsolutePath,
            ImageSize.X,
            ImageSize.Y,
            PngBytes.Num());
        FPlatformMisc::RequestExit(false);
        return false;
    }

    FString PendingCapturePath;
    uint64 CaptureSeed = 24301ULL;
    int32 CaptureAttempts = 0;
    FTSTicker::FDelegateHandle CaptureTicker;
    TWeakPtr<SDockTab> InspectorTab;
    TWeakPtr<SeedForge::Inspector::Private::SSeedForgeInspector> InspectorWidget;
};

IMPLEMENT_MODULE(FSeedForgeEditorModule, SeedForgeEditor)

#undef LOCTEXT_NAMESPACE
