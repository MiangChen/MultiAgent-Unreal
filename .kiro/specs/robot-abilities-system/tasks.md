# Implementation Plan

- [x] 1. Extend Gameplay Tags for new abilities
  - Add new Ability Tags: Ability_Patrol, Ability_Search, Ability_Observe, Ability_Report, Ability_Charge, Ability_Formation, Ability_Avoid
  - Add new Status Tags: Status_Patrolling, Status_Searching, Status_Observing, Status_Charging, Status_InFormation, Status_Avoiding, Status_LowEnergy
  - Add new Event Tags: Event_Target_Found, Event_Target_Lost, Event_Charge_Complete, Event_Patrol_WaypointReached
  - _Requirements: 3.1, 4.1, 5.1, 6.1, 7.1, 8.1, 9.1_

- [x] 2. Implement Energy System in MARobotDogCharacter
  - [x] 2.1 Add Energy properties to MARobotDogCharacter
    - Add Energy, MaxEnergy, EnergyDrainRate properties
    - Implement DrainEnergy(), RestoreEnergy(), HasEnergy() methods
    - _Requirements: 1.1, 1.3, 1.4_
  - [x] 2.2 Implement energy display above robot head
    - Override Tick to update energy display
    - Use DrawDebugString to show "Energy: XX%" above head
    - _Requirements: 1.2_

- [x] 3. Create ChargingStation Actor
  - [x] 3.1 Create AMAChargingStation class
    - Create header and cpp files in Actor folder
    - Add StaticMeshComponent for visual representation
    - Add SphereComponent for interaction range detection
    - _Requirements: 2.1, 2.2_
  - [x] 3.2 Implement overlap detection
    - Implement OnOverlapBegin/End for robot detection
    - Add IsRobotInRange() method
    - _Requirements: 2.3_

- [x] 4. Create PatrolPath Actor
  - Create AMAPatrolPath class with Waypoints array
  - Implement GetWaypoint(), GetNextWaypointIndex() methods
  - Add editor visualization for waypoints
  - _Requirements: 3.1_

- [x] 5. Implement GA_Patrol ability
  - [x] 5.1 Create GA_Patrol class
    - Create header and cpp files
    - Implement SetPatrolPath() and SetWaypoints() methods
    - _Requirements: 3.1_
  - [x] 5.2 Implement patrol logic
    - Implement MoveToNextWaypoint() using AIController
    - Implement waypoint cycling (return to first after last)
    - Handle patrol cancellation
    - _Requirements: 3.2, 3.3, 3.4_

- [x] 6. Implement GA_Search ability
  - [x] 6.1 Create GA_Search class
    - Create header and cpp files
    - Add DetectionInterval and DetectionRange properties
    - _Requirements: 4.1, 4.2_
  - [x] 6.2 Implement human detection logic
    - Check for attached CameraSensor in CanActivateAbility
    - Implement PerformDetection() with line trace or overlap check
    - Output "Find Target" when Human detected
    - Trigger target-found event
    - _Requirements: 4.1, 4.2, 4.3, 4.4_

- [x] 7. Implement GA_Observe ability
  - [x] 7.1 Create GA_Observe class
    - Create header and cpp files
    - Add SetObserveTarget() method and ObserveRange property
    - _Requirements: 5.1, 5.3_
  - [x] 7.2 Implement observation logic
    - Implement continuous observation with timer
    - Detect when target leaves range and output "Target Lost"
    - _Requirements: 5.2, 5.4_

- [x] 8. Implement GA_Report ability
  - [x] 8.1 Create GA_Report class
    - Create header and cpp files
    - Add SetReportMessage() method and DisplayDuration property
    - _Requirements: 6.3, 6.2_
  - [x] 8.2 Implement report dialog display
    - Use GEngine->AddOnScreenDebugMessage for dialog display
    - Implement display timer and auto-hide
    - Handle message queuing for multiple reports
    - _Requirements: 6.1, 6.2, 6.4_

- [x] 9. Implement GA_Charge ability
  - [x] 9.1 Create GA_Charge class
    - Create header and cpp files
    - Implement FindNearbyChargingStation() method
    - _Requirements: 7.1_
  - [x] 9.2 Implement charging logic
    - Check ChargingStation range in CanActivateAbility
    - Restore Energy to 100% on activation
    - Output "Charging Complete" on completion
    - _Requirements: 7.1, 7.2, 7.3, 7.4_

- [x] 10. Implement GA_Formation ability
  - [x] 10.1 Create GA_Formation class
    - Create header and cpp files
    - Define EFormationType enum (Line, Column, Wedge, Diamond)
    - Add SetFormation() method with leader, type, position parameters
    - _Requirements: 8.1, 8.2_
  - [x] 10.2 Implement formation logic
    - Implement CalculateFormationOffset() for each formation type
    - Implement UpdateFormationPosition() with timer
    - Maintain position relative to leader
    - _Requirements: 8.3, 8.4_

- [x] 11. Implement GA_Avoid ability
  - [x] 11.1 Create GA_Avoid class
    - Create header and cpp files
    - Add DetectionRadius and AvoidanceStrength properties
    - _Requirements: 9.1_
  - [x] 11.2 Implement avoidance logic
    - Implement CheckObstacles() with sphere overlap
    - Implement CalculateAvoidanceVector()
    - Apply avoidance while maintaining destination progress
    - Resume direct navigation when obstacle cleared
    - _Requirements: 9.1, 9.2, 9.3, 9.4_

- [x] 12. Integrate new abilities into MAAbilitySystemComponent
  - [x] 12.1 Add ability handles and activation methods
    - Add handles for all new abilities
    - Implement TryActivatePatrol(), TryActivateSearch(), etc.
    - _Requirements: 3.1, 4.1, 5.1, 6.1, 7.1, 8.1, 9.1_
  - [x] 12.2 Initialize abilities in InitializeAbilities()
    - Grant all new abilities to RobotDog characters
    - _Requirements: 3.1, 4.1, 5.1, 6.1, 7.1, 8.1, 9.1_

- [x] 13. Add convenience methods to MARobotDogCharacter
  - Add TryPatrol(), TrySearch(), TryCharge(), etc. wrapper methods
  - Integrate energy check into movement abilities
  - _Requirements: 1.4, 3.1, 4.1, 7.1_
