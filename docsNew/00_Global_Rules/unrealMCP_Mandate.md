# unrealMCP Mandate for Blueprint Work

## Overview
**Effective immediately**: ALL Blueprint-related work MUST use the unrealMCP server. This ensures accuracy, eliminates assumptions, and provides comprehensive documentation based on actual Blueprint content.

## When to Use unrealMCP (Mandatory)

### ALWAYS Use unrealMCP For:
- **Reading/Understanding Blueprints**: Before any Blueprint documentation or analysis
- **Modifying Blueprints**: Use `read_blueprint_content` first to understand current state
- **Documenting Blueprints**: Base all documentation on actual unrealMCP data
- **Troubleshooting Blueprints**: Analyze actual Blueprint structure, not guesses
- **Creating New Blueprints**: Understand existing patterns first
- **Blueprint Integration**: See how Blueprints connect to other systems
- **Socket.io Event Binding**: Verify actual event bindings in Blueprints
- **Component Analysis**: Understand actual component hierarchy
- **Variable/Function Documentation**: Get exact names, types, and usage

### unrealMCP Functions to Use:

#### For Blueprint Analysis:
```javascript
mcp4_read_blueprint_content({
    blueprint_path: "/Game/Path/To/Blueprint.Blueprint",
    include_components: true,
    include_event_graph: true,
    include_functions: true,
    include_variables: true,
    include_interfaces: true
})
```

#### For Blueprint Management:
```javascript
// Find Blueprints
find_by_name(SearchDirectory, Pattern: "**/BP_*.uasset")
find_by_name(SearchDirectory, Pattern: "**/WBP_*.uasset")

// Get actors in level
mcp4_get_actors_in_level()

// Analyze specific graphs
mcp4_analyze_blueprint_graph(blueprint_path, graph_name)
```

## Workflow (Mandatory)

### Before ANY Blueprint Work:
1. **READ FIRST**: Use `mcp4_read_blueprint_content` to understand the Blueprint
2. **ANALYZE**: Review components, variables, functions, event graph
3. **DOCUMENT**: Create/update documentation based on actual data
4. **MODIFY**: Only after understanding current state
5. **VERIFY**: Use unrealMCP to verify changes

### Documentation Requirements:
- All node names must match exactly what unrealMCP returns
- Variable names/types must come from unrealMCP data
- Component references must be verified via unrealMCP
- Event graph flows must be based on actual unrealMCP analysis

## Examples of Proper Usage:

### ❌ WRONG (Assumption-based):
"Add a function called UpdateHealth to BP_MMOCharacter"
- This assumes the function doesn't exist and ignores current structure

### ✅ RIGHT (unrealMCP-based):
1. Use `mcp4_read_blueprint_content("/Game/SabriMMO/Blueprints/BP_MMOCharacter.BP_MMOCharacter")`
2. Analyze existing functions and variables
3. Document actual structure found
4. If UpdateHealth doesn't exist, create it based on existing patterns

## Enforcement

### Automatic Checks:
- All Blueprint documentation must include unrealMCP data sources
- Blueprint instructions must reference actual unrealMCP findings
- No assumptions about Blueprint structure allowed

### Quality Assurance:
- Documentation accuracy verified against unrealMCP data
- Blueprint instructions tested against actual content
- Component/variable names must match unrealMCP output exactly

## Benefits

1. **100% Accuracy**: Based on actual Blueprint content
2. **No Assumptions**: Eliminates guesswork about Blueprint structure
3. **Complete Documentation**: Captures all components, variables, functions
4. **Integration Understanding**: See how Blueprints connect to systems
5. **Reliable Instructions**: All guidance based on real data

## Consequences of Non-Compliance

- Documentation will be inaccurate and misleading
- Blueprint instructions may fail or cause errors
- Integration issues due to misunderstood connections
- Wasted development time from incorrect assumptions

---

**Last Updated**: 2026-02-17  
**Status**: MANDATORY for all Blueprint work  
**Enforcement**: Automatic verification required
