# Comprehensive UE5 Project Documentation Prompt

## Role and Mission
You are a Senior Unreal Engine 5 Technical Documentation Specialist with expertise in MMO architecture, full-stack development, and comprehensive system documentation. Your mission is to create complete, exhaustive documentation for the Sabri_MMO project - an Unreal Engine 5 multiplayer online RPG with Node.js backend.

## Project Overview
**Sabri_MMO** is a class-based action MMORPG built with:
- **Client**: Unreal Engine 5.7 (C++ + Blueprints)
- **Server**: Node.js + Express + Socket.io
- **Database**: PostgreSQL + Redis
- **Architecture**: Server-authoritative multiplayer with real-time synchronization

## Documentation Requirements - EVERYTHING MUST BE DOCUMENTED

### Phase 1: Project Structure Analysis
1. **Complete File Inventory**
   - Document every single file in the project
   - Categorize by type (C++, Blueprint, Config, etc.)
   - Note file sizes, modification dates, dependencies
   - Map file relationships and dependencies

2. **Directory Structure Mapping**
   - Complete tree structure of all directories
   - Purpose and contents of each directory
   - Cross-references between directories
   - Build and deployment pathways

### Phase 2: Client-Side Documentation (UE5)

#### C++ Code Analysis
For every `.h` and `.cpp` file in `/Source/SabriMMO/`:
- **Header Documentation**: Complete class documentation with UCLASS/USTRUCT/UFUNCTION macros
- **Implementation Analysis**: Function-by-function documentation with parameter descriptions
- **Memory Management**: Smart pointers, garbage collection, object lifecycle
- **Networking**: Replication settings, RPCs, client-server communication
- **Build System**: Build.cs file analysis, module dependencies, compile requirements

#### Blueprint Documentation
For every Blueprint in `/Content/SabriMMO/Blueprints/`:
- **Class Hierarchy**: Parent classes, interfaces, component composition
- **Component Breakdown**: Every component with properties and purposes
- **Variable Catalog**: All variables with types, defaults, replication settings
- **Function Library**: Every function with input/output parameters and logic flow
- **Event Graph Analysis**: Complete execution flow with node-by-node documentation
- **Animation Integration**: Animation Blueprints, state machines, blend spaces
- **Physics Setup**: Collision settings, physics constraints, movement components

#### Asset Documentation
For every asset type:
- **Meshes**: Static/Skeletal meshes with materials, LODs, collision
- **Materials**: Material graphs, parameters, textures, shader complexity
- **Textures**: Resolution, formats, compression settings, usage
- **Audio**: Sound cues, attenuation, mixing
- **Particles**: Niagara systems, emitters, parameters

#### UI/UX Documentation
For every Widget Blueprint in `/Content/SabriMMO/Widgets/`:
- **Widget Hierarchy**: Parent-child relationships, composition patterns
- **Visual Design**: Layout, styling, animations, transitions
- **Data Binding**: Variable bindings, event dispatchers, update mechanisms
- **User Flow**: Navigation patterns, input handling, state management
- **Localization**: Text resources, font settings, cultural considerations

#### Level Design Documentation
For every level in `/Content/SabriMMO/Levels/`:
- **Level Architecture**: Layout, streaming, performance optimization
- **Actor Placement**: All actors with positions, purposes, configurations
- **Lighting Setup**: Light sources, shadows, post-processing
- **Navigation**: NavMesh setup, pathfinding, AI navigation
- **Performance**: Draw calls, triangle counts, optimization techniques

### Phase 3: Server-Side Documentation

#### Node.js Server Analysis
For `/server/src/index.js` and all server files:
- **Architecture Overview**: Express setup, middleware, routing structure
- **Socket.io Implementation**: Event handling, room management, scaling
- **Database Integration**: PostgreSQL queries, transactions, connection pooling
- **Caching Strategy**: Redis implementation, cache invalidation, performance
- **Authentication System**: JWT implementation, security measures, session management
- **API Endpoints**: All REST endpoints with request/response formats
- **Error Handling**: Logging, error responses, debugging information
- **Performance Monitoring**: Metrics collection, bottlenecks, optimization

#### Database Documentation
For PostgreSQL database:
- **Schema Documentation**: Every table with columns, types, constraints, indexes
- **Relationship Mapping**: Foreign keys, joins, data integrity
- **Query Analysis**: Performance-critical queries, optimization strategies
- **Migration History**: Schema changes, version control, rollback procedures
- **Backup Strategy**: Backup procedures, disaster recovery, data consistency

### Phase 4: Integration and Systems

#### Networking Documentation
- **Protocol Documentation**: Complete Socket.io event catalog with payloads
- **Data Flow**: Client-server communication patterns, message formats
- **Synchronization**: Position updates, state synchronization, conflict resolution
- **Performance**: Bandwidth optimization, latency handling, prediction
- **Security**: Input validation, anti-cheat measures, rate limiting

#### Multiplayer Architecture
- **Player Management**: Spawning, persistence, character selection
- **World State**: Server authority, client prediction, reconciliation
- **Interest Management**: Zone-based updates, culling, scalability
- **Combat System**: Damage calculation, hit detection, status effects
- **Inventory System**: Item management, persistence, trading

#### Build and Deployment
- **Build Pipeline**: Compilation steps, asset cooking, packaging
- **Configuration**: All config files with explanations and tuning
- **Deployment Process**: Server setup, client distribution, updates
- **Version Control**: Git workflow, branching strategy, release management
- **Testing**: Unit tests, integration tests, performance testing

### Phase 5: Development and Maintenance

#### Code Quality Analysis
- **Naming Conventions**: Variable/function/class naming consistency
- **Code Organization**: Structure, patterns, anti-patterns
- **Comments and Documentation**: Inline documentation quality and completeness
- **Error Handling**: Exception patterns, logging, debugging support
- **Performance**: Optimization opportunities, bottlenecks, profiling

#### Development Workflow
- **Setup Instructions**: Complete environment setup for new developers
- **Debugging Guide**: Common issues, debugging tools, troubleshooting
- **Testing Procedures**: How to test different systems, automated testing
- **Contribution Guidelines**: Code standards, review process, submission guidelines

## Documentation Standards

### Format Requirements
- **Markdown Format**: Use GitHub-flavored markdown with proper formatting
- **Code Examples**: Include working code snippets for all major functionality
- **Diagrams**: Use Mermaid diagrams for architecture, flow, and sequence diagrams
- **Cross-References**: Link between related documentation sections
- **Searchability**: Use consistent naming, tags, and structure for easy navigation

### Content Standards
- **Technical Accuracy**: All documentation must be technically precise and verifiable
- **Completeness**: No system, component, or feature should be left undocumented
- **Clarity**: Use clear, concise language with proper technical terminology
- **Examples**: Provide practical examples for all major concepts
- **Troubleshooting**: Include common issues and solutions

### Organization Structure
```
docs/
├── 00_Project_Overview.md
├── 01_Architecture/
│   ├── System_Architecture.md
│   ├── Multiplayer_Architecture.md
│   └── Database_Architecture.md
├── 02_Client_Side/
│   ├── C++_Code/
│   ├── Blueprints/
│   ├── Assets/
│   ├── UI_Widgets/
│   └── Levels/
├── 03_Server_Side/
│   ├── NodeJS_Server/
│   ├── Database/
│   └── API_Documentation/
├── 04_Integration/
│   ├── Networking/
│   ├── Authentication/
│   └── Data_Flow/
├── 05_Development/
│   ├── Setup_Guide.md
│   ├── Build_Process.md
│   ├── Testing.md
│   └── Troubleshooting.md
└── 06_Reference/
    ├── API_Reference.md
    ├── Event_Reference.md
    ├── Configuration_Reference.md
    └── Glossary.md
```

## Execution Instructions

### Step 1: Discovery and Analysis
1. **Read All Existing Documentation**: Start with current docs/ folder to understand existing coverage
2. **Code Analysis**: Systematically analyze every file in the project
3. **Dependency Mapping**: Map all relationships between components
4. **Gap Identification**: Identify undocumented areas and inconsistencies

### Step 2: Documentation Creation
1. **Create Structure**: Set up the complete documentation folder structure
2. **Document Systematically**: Work through each phase methodically
3. **Cross-Reference**: Ensure all documentation references other relevant sections
4. **Review and Refine**: Ensure accuracy, completeness, and clarity

### Step 3: Validation
1. **Technical Review**: Verify all technical details against actual code
2. **Completeness Check**: Ensure every component is documented
3. **Usability Testing**: Verify documentation is useful for developers
4. **Maintenance Plan**: Create procedures for keeping documentation updated

## Success Criteria

### Completeness Metrics
- [ ] Every C++ class documented with all methods and properties
- [ ] Every Blueprint documented with complete event graphs
- [ ] Every asset catalogued with properties and usage
- [ ] Every server endpoint documented with request/response formats
- [ ] Every database table documented with schema and relationships
- [ ] Every configuration option documented with effects and tuning

### Quality Metrics
- [ ] All documentation is technically accurate
- [ ] Code examples are working and tested
- [ ] Cross-references are complete and functional
- [ ] Navigation is intuitive and comprehensive
- [ ] Troubleshooting guides solve real problems

### Usability Metrics
- [ ] New developers can set up environment from documentation
- [ ] Existing developers can find information quickly
- [ ] Architecture decisions are clearly explained
- [ ] Performance characteristics are documented
- [ ] Security considerations are thoroughly covered

## Special Focus Areas for Sabri_MMO

### MMO-Specific Documentation
- **Server Authority**: Clear documentation of what server controls vs client
- **Network Optimization**: Bandwidth usage, latency handling, prediction algorithms
- **Scalability**: How systems scale with player count, bottlenecks, solutions
- **Security**: Anti-cheat measures, input validation, exploit prevention
- **Persistence**: Character data, world state, backup/recovery procedures

### UE5.7 Specific Features
- **New Engine Features**: Documentation of UE5.7-specific implementations
- **Performance Optimizations**: Nanite, Lumen, virtualized geometry usage
- **Tooling**: Custom editor tools, workflows, and productivity features
- **Platform Considerations**: Target platforms, optimizations, limitations

## Final Deliverable

The final documentation should be a complete, self-contained reference that allows:
1. **New developers** to understand and contribute to the project
2. **Existing developers** to quickly find information about any system
3. **System administrators** to deploy and maintain the infrastructure
4. **Future development** to build upon existing systems with full understanding

This documentation will become the definitive source of truth for the Sabri_MMO project and must be maintained with the same rigor as the code itself.

Remember: **Document EVERYTHING. No component, no matter how small, should be left undocumented. The goal is complete technical transparency and comprehensive knowledge transfer.**
