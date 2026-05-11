// SpawnRegionExporter.h — Export ASpawnAllowVolume / ASpawnDenyVolume actors as JSON for server-side spawn generation.
// Usage: open the level in editor, run console command:  ExportSpawnRegions <zone_name>
// Auto-detect form:                                       ExportCurrentLevelSpawnRegions
// Output: server/spawn_regions/<zone_name>.json (world-space axis-aligned box bounds).

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SpawnRegionExporter.generated.h"

UCLASS()
class SABRIMMO_API USpawnRegionExporter : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Export all SpawnAllowVolume + SpawnDenyVolume actors from the current level as JSON.
	 *
	 * @param WorldContextObject  World context (auto-filled in Blueprint)
	 * @param ZoneName            Zone identifier (e.g., "prontera_south") — used as filename
	 * @param OutputDirectory     Directory to write the JSON file (default: project's server/spawn_regions/)
	 */
	UFUNCTION(BlueprintCallable, Category = "Spawn Region Export", meta = (WorldContext = "WorldContextObject"))
	static void ExportSpawnRegionsToJSON(
		UObject* WorldContextObject,
		const FString& ZoneName,
		const FString& OutputDirectory = TEXT("")
	);

	/** Export using the level name → zone name mapping (mirrors NavMeshExporter). */
	UFUNCTION(BlueprintCallable, Category = "Spawn Region Export", meta = (WorldContext = "WorldContextObject"))
	static void ExportCurrentLevelSpawnRegions(UObject* WorldContextObject);

private:
	static FString GetDefaultOutputDirectory();
	static FString MapNameToZone(const FString& MapName);
	static FString EscapeJsonString(const FString& In);
};
