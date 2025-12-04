# Requirements Document

## Introduction

本需求文档定义了 MultiAgent-Unreal 项目中机器人能力系统的扩展功能。该系统为机器人（主要是 RobotDog）添加电量管理、巡逻、搜索、观察、报告、编队和避障等能力，使机器人能够执行更复杂的自主任务。

## Glossary

- **RobotDog**: 机器狗角色类 (`AMARobotDogCharacter`)，继承自 `AMACharacter`
- **GAS**: Gameplay Ability System，UE5 的技能系统框架
- **GA (GameplayAbility)**: GAS 中的技能类
- **Energy**: 机器人的电量属性，范围 0-100%
- **ChargingStation**: 充电站 Actor，机器人可在此充电
- **PatrolPath**: 巡逻路径，由多个路径点组成
- **CameraSensor**: 摄像头传感器，可附着到机器人上
- **Human**: 人类角色类 (`AMAHumanCharacter`)
- **Formation**: 编队，多个机器人按特定阵型移动
- **ASC**: AbilitySystemComponent，GAS 核心组件

## Requirements

### Requirement 1: Energy System

**User Story:** As a simulation operator, I want robots to have an energy system with visual display, so that I can monitor their operational status.

#### Acceptance Criteria

1. THE RobotDog SHALL have an Energy attribute with a value range of 0 to 100 percent.
2. WHILE the RobotDog is active, THE System SHALL display the current energy percentage above the RobotDog's head in the format "Energy: XX%".
3. WHILE the RobotDog is executing any movement ability, THE Energy attribute SHALL decrease at a configurable rate per second.
4. WHEN the Energy attribute reaches 0 percent, THE RobotDog SHALL stop executing all movement-related abilities.

### Requirement 2: Charging Station

**User Story:** As a simulation operator, I want a charging station where robots can recharge, so that robots can continue operating after depleting energy.

#### Acceptance Criteria

1. THE System SHALL provide a ChargingStation Actor class that can be placed in the level.
2. THE ChargingStation SHALL have a visual representation resembling a warehouse or docking area.
3. WHEN a RobotDog enters the ChargingStation's interaction range, THE System SHALL enable the GA_Charge ability for that RobotDog.
4. WHEN the GA_Charge ability is activated, THE RobotDog's Energy attribute SHALL be set to 100 percent.

### Requirement 3: GA_Patrol Ability

**User Story:** As a simulation operator, I want robots to patrol along a defined path, so that they can autonomously monitor an area.

#### Acceptance Criteria

1. THE GA_Patrol ability SHALL accept a PatrolPath parameter containing an ordered array of waypoint locations.
2. WHILE GA_Patrol is active, THE RobotDog SHALL move sequentially through each waypoint in the PatrolPath.
3. WHEN the RobotDog reaches the last waypoint in the PatrolPath, THE RobotDog SHALL return to the first waypoint and continue the patrol cycle.
4. WHEN GA_Patrol is cancelled, THE RobotDog SHALL stop at its current position.

### Requirement 4: GA_Search Ability

**User Story:** As a simulation operator, I want robots with cameras to detect humans in their field of view, so that they can perform surveillance tasks.

#### Acceptance Criteria

1. THE GA_Search ability SHALL require the RobotDog to have an attached CameraSensor.
2. WHILE GA_Search is active, THE CameraSensor SHALL perform detection checks at a configurable interval.
3. WHEN the CameraSensor detects a Human character within its field of view, THE System SHALL output "Find Target" to the log.
4. WHEN a Human is detected, THE GA_Search ability SHALL trigger a target-found event that other systems can respond to.

### Requirement 5: GA_Observe Ability

**User Story:** As a simulation operator, I want sensors to have an observe ability, so that they can monitor specific areas or targets.

#### Acceptance Criteria

1. THE GA_Observe ability SHALL be available to Sensor-type Actors.
2. WHILE GA_Observe is active, THE Sensor SHALL continuously capture data from its observation area.
3. WHEN GA_Observe is activated with a target Actor parameter, THE Sensor SHALL track and observe that specific target.
4. WHEN the observed target leaves the Sensor's range, THE GA_Observe ability SHALL output a "Target Lost" notification.

### Requirement 6: GA_Report Ability

**User Story:** As a simulation operator, I want robots to report their findings, so that I can receive information about detected events.

#### Acceptance Criteria

1. THE GA_Report ability SHALL display a dialog box on screen with the report content.
2. WHEN GA_Report is activated, THE System SHALL display the report message for a configurable duration.
3. THE GA_Report ability SHALL accept a message string parameter for the report content.
4. WHEN multiple reports are triggered simultaneously, THE System SHALL queue and display them sequentially.

### Requirement 7: GA_Charge Ability

**User Story:** As a simulation operator, I want robots to charge at charging stations, so that they can restore their energy.

#### Acceptance Criteria

1. THE GA_Charge ability SHALL only be activatable when the RobotDog is within a ChargingStation's interaction range.
2. WHEN GA_Charge is activated, THE RobotDog's Energy attribute SHALL be restored to 100 percent.
3. WHEN GA_Charge completes, THE System SHALL output "Charging Complete" to the log.
4. IF the RobotDog is not within a ChargingStation's range, THEN THE GA_Charge ability activation SHALL fail.

### Requirement 8: GA_Formation Ability

**User Story:** As a simulation operator, I want multiple robots to move in formation, so that they can coordinate their movements.

#### Acceptance Criteria

1. THE GA_Formation ability SHALL accept a formation type parameter defining the arrangement pattern.
2. THE GA_Formation ability SHALL accept a leader Actor parameter that the formation follows.
3. WHILE GA_Formation is active, THE RobotDog SHALL maintain its assigned position relative to the formation leader.
4. WHEN the formation leader moves, THE RobotDog SHALL adjust its position to maintain the formation shape.

### Requirement 9: GA_Avoid Ability

**User Story:** As a simulation operator, I want robots to avoid obstacles and other agents, so that they can navigate safely.

#### Acceptance Criteria

1. THE GA_Avoid ability SHALL detect obstacles within a configurable detection radius.
2. WHEN an obstacle is detected in the RobotDog's movement path, THE RobotDog SHALL calculate an avoidance trajectory.
3. WHILE avoiding an obstacle, THE RobotDog SHALL maintain progress toward its original destination.
4. WHEN the obstacle is no longer in the movement path, THE RobotDog SHALL resume direct navigation to the destination.
