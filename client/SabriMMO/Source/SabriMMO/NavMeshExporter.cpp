// NavMeshExporter.cpp — Export UE5 NavMesh geometry as OBJ for server-side pathfinding

#include "NavMeshExporter.h"
#include "NavigationSystem.h"
#include "NavMesh/RecastNavMesh.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "Engine/World.h"

// Console command: ExportNavMesh <zone_name> [output_dir]
static FAutoConsoleCommandWithWorldAndArgs GExportNavMeshCmd(
	TEXT("ExportNavMesh"),
	TEXT("Export NavMesh geometry for the current level as OBJ. Usage: ExportNavMesh <zone_name> [output_dir]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (Args.Num() < 1)
		{
			UE_LOG(LogTemp, Error, TEXT("[NavMeshExport] Usage: ExportNavMesh <zone_name> [output_dir]"));
			UE_LOG(LogTemp, Error, TEXT("[NavMeshExport] Zone names: prontera, prontera_south, prontera_north, prt_dungeon_01"));
			return;
		}

		const FString ZoneName = Args[0];
		const FString OutputDir = Args.Num() >= 2 ? Args[1] : TEXT("");

		UNavMeshExporter::ExportNavMeshToOBJ(World, ZoneName, OutputDir);
	})
);

FString UNavMeshExporter::GetDefaultOutputDirectory()
{
	// Navigate from project root to server/navmesh/
	// Project root: C:/Sabri_MMO/client/SabriMMO/ → go up 2 levels to C:/Sabri_MMO/
	FString ProjectDir = FPaths::ProjectDir();
	FString RootDir = FPaths::GetPath(FPaths::GetPath(ProjectDir)); // Up 2 levels
	return RootDir / TEXT("server") / TEXT("navmesh");
}

void UNavMeshExporter::ExportNavMeshToOBJ(
	UObject* WorldContextObject,
	const FString& ZoneName,
	const FString& OutputDirectory)
{
	if (ZoneName.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("[NavMeshExport] Zone name cannot be empty"));
		return;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("[NavMeshExport] No valid world context"));
		return;
	}

	// Get Navigation System
	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(World);
	if (!NavSys)
	{
		UE_LOG(LogTemp, Error, TEXT("[NavMeshExport] No NavigationSystem found in this level"));
		return;
	}

	// Get the default NavMesh (Recast)
	ANavigationData* NavData = NavSys->GetDefaultNavDataInstance(FNavigationSystem::DontCreate);
	ARecastNavMesh* RecastNavMesh = Cast<ARecastNavMesh>(NavData);
	if (!RecastNavMesh)
	{
		UE_LOG(LogTemp, Error, TEXT("[NavMeshExport] No RecastNavMesh found — ensure NavMesh is built in this level"));
		return;
	}

	// Extract debug geometry by iterating all tiles (UE5.7 API: per-tile)
	FRecastDebugGeometry DebugGeo;
	DebugGeo.bGatherNavMeshEdges = false;
	DebugGeo.bGatherPolyEdges = false;

	const int32 TileCount = RecastNavMesh->GetNavMeshTilesCount();
	RecastNavMesh->BeginBatchQuery();
	for (int32 TileIdx = 0; TileIdx < TileCount; ++TileIdx)
	{
		RecastNavMesh->GetDebugGeometryForTile(DebugGeo, TileIdx);
	}
	RecastNavMesh->FinishBatchQuery();

	if (DebugGeo.MeshVerts.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("[NavMeshExport] NavMesh has no geometry (checked %d tiles) — is it built?"), TileCount);
		return;
	}

	// Count total triangles
	int32 TotalTris = 0;
	for (const TArray<int32>& AreaIndices : DebugGeo.AreaIndices)
	{
		TotalTris += AreaIndices.Num() / 3;
	}

	UE_LOG(LogTemp, Log, TEXT("[NavMeshExport] Exporting %s: %d vertices, %d triangles"),
		*ZoneName, DebugGeo.MeshVerts.Num(), TotalTris);

	// Build OBJ content
	FString ObjContent;
	ObjContent.Reserve(DebugGeo.MeshVerts.Num() * 40 + TotalTris * 30 + 200);

	ObjContent += FString::Printf(TEXT("# NavMesh export for zone: %s\n"), *ZoneName);
	ObjContent += FString::Printf(TEXT("# Exported from UE5 level: %s\n"), *World->GetMapName());
	ObjContent += FString::Printf(TEXT("# Vertices: %d, Triangles: %d\n"), DebugGeo.MeshVerts.Num(), TotalTris);
	ObjContent += FString::Printf(TEXT("# Coordinate system: Recast Y-up (swapped from UE5 Z-up)\n"));
	ObjContent += TEXT("o NavMesh\n");

	// Vertices: UE5 (X,Y,Z where Z=up) → Recast (X,Y,Z where Y=up)
	// OBJ vertex: v UE_X UE_Z UE_Y (swap Y and Z)
	for (const FVector& V : DebugGeo.MeshVerts)
	{
		ObjContent += FString::Printf(TEXT("v %.4f %.4f %.4f\n"), V.X, V.Z, V.Y);
	}

	// Faces from all nav area types
	// After Y↔Z vertex swap, UE5's left-handed winding becomes the correct
	// CCW winding for Recast's right-handed Y-up system — no index swap needed.
	// OBJ indices are 1-based
	for (const TArray<int32>& AreaIndices : DebugGeo.AreaIndices)
	{
		for (int32 i = 0; i + 2 < AreaIndices.Num(); i += 3)
		{
			ObjContent += FString::Printf(TEXT("f %d %d %d\n"),
				AreaIndices[i] + 1,
				AreaIndices[i + 1] + 1,
				AreaIndices[i + 2] + 1);
		}
	}

	// Determine output path
	FString OutDir = OutputDirectory.IsEmpty() ? GetDefaultOutputDirectory() : OutputDirectory;

	// Ensure directory exists
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*OutDir))
	{
		PlatformFile.CreateDirectoryTree(*OutDir);
	}

	FString FilePath = OutDir / FString::Printf(TEXT("%s.obj"), *ZoneName);

	if (FFileHelper::SaveStringToFile(ObjContent, *FilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		UE_LOG(LogTemp, Log, TEXT("[NavMeshExport] SUCCESS: Exported %s to %s"), *ZoneName, *FilePath);
		UE_LOG(LogTemp, Log, TEXT("[NavMeshExport]   %d vertices, %d triangles"), DebugGeo.MeshVerts.Num(), TotalTris);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[NavMeshExport] FAILED: Could not write to %s"), *FilePath);
	}
}

void UNavMeshExporter::ExportCurrentLevelNavMesh(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World) return;

	// Map level names to zone names
	const FString MapName = World->GetMapName();

	// Strip any prefix (e.g., "UEDPIE_0_" in PIE mode)
	FString CleanMapName = MapName;
	int32 LastUnderscore = INDEX_NONE;
	if (CleanMapName.StartsWith(TEXT("UEDPIE_")))
	{
		// Format: UEDPIE_N_LevelName
		int32 SecondUnderscore = INDEX_NONE;
		CleanMapName.FindChar(TEXT('_'), LastUnderscore);
		if (LastUnderscore != INDEX_NONE)
		{
			CleanMapName.FindLastChar(TEXT('_'), SecondUnderscore);
			if (SecondUnderscore != INDEX_NONE && SecondUnderscore > LastUnderscore)
			{
				// Find the second underscore to get past "UEDPIE_0_"
				FString Temp = CleanMapName.Mid(LastUnderscore + 1);
				int32 NextUnderscore = INDEX_NONE;
				Temp.FindChar(TEXT('_'), NextUnderscore);
				if (NextUnderscore != INDEX_NONE)
				{
					CleanMapName = Temp.Mid(NextUnderscore + 1);
				}
			}
		}
	}

	// Level name → zone name mapping
	FString ZoneName;
	if (CleanMapName.Contains(TEXT("PrtSouth")))       ZoneName = TEXT("prontera_south");
	else if (CleanMapName.Contains(TEXT("PrtNorth")))   ZoneName = TEXT("prontera_north");
	else if (CleanMapName.Contains(TEXT("PrtDungeon01"))) ZoneName = TEXT("prt_dungeon_01");
	else if (CleanMapName.Contains(TEXT("Prontera")))   ZoneName = TEXT("prontera");
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[NavMeshExport] Unknown level '%s' — using as zone name"), *CleanMapName);
		ZoneName = CleanMapName.ToLower();
	}

	UE_LOG(LogTemp, Log, TEXT("[NavMeshExport] Auto-detected zone: %s (from level: %s)"), *ZoneName, *MapName);
	ExportNavMeshToOBJ(WorldContextObject, ZoneName);
}
