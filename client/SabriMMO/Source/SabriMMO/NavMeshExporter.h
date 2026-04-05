// NavMeshExporter.h — Export UE5 NavMesh geometry as OBJ for server-side pathfinding
// Usage: Open level in editor, run console command: ExportNavMesh <zone_name>
// Output: server/navmesh/<zone_name>.obj (coordinates swapped for Recast Y-up)

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NavMeshExporter.generated.h"

UCLASS()
class SABRIMMO_API UNavMeshExporter : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Export the current level's NavMesh geometry as an OBJ file.
	 * Coordinates are converted from UE5 Z-up to Recast Y-up (swap Y and Z).
	 * Triangle winding is reversed to maintain upward normals after the swap.
	 *
	 * @param WorldContextObject  World context (auto-filled in Blueprint)
	 * @param ZoneName            Zone identifier (e.g., "prontera_south") — used as filename
	 * @param OutputDirectory     Directory to write the OBJ file (default: project's server/navmesh/)
	 */
	UFUNCTION(BlueprintCallable, Category = "NavMesh Export", meta = (WorldContext = "WorldContextObject"))
	static void ExportNavMeshToOBJ(
		UObject* WorldContextObject,
		const FString& ZoneName,
		const FString& OutputDirectory = TEXT("")
	);

	/** Export all known zones (requires each level to be loaded — use for batch export) */
	UFUNCTION(BlueprintCallable, Category = "NavMesh Export", meta = (WorldContext = "WorldContextObject"))
	static void ExportCurrentLevelNavMesh(UObject* WorldContextObject);

private:
	static FString GetDefaultOutputDirectory();
};
