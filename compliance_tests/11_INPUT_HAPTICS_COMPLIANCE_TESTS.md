# OpenXR Input and Haptics Compliance Test Cases (Section 11)

This document defines compliance test cases for OpenXR Input and Haptics functionality based on the OpenXR specification section 11. These tests ensure that the ox runtime correctly implements all required behaviors.

## Test Categories

- [Action Sets](#action-sets)
- [Actions](#actions)
- [Suggested Bindings](#suggested-bindings)
- [Current Interaction Profile](#current-interaction-profile)
- [Action State Reading](#action-state-reading)
- [Action State Synchronization](#action-state-synchronization)
- [Haptics](#haptics)
- [Bound Sources](#bound-sources)
- [Priority and Multi-Binding](#priority-and-multi-binding)

---

## Action Sets

### TEST-AS-001: Create Action Set with Valid Parameters
**Must**: Runtime must create action set with valid parameters
- Create action set with valid name, localized name, and priority
- Verify XR_SUCCESS is returned
- Verify valid handle is returned

### TEST-AS-002: Empty Action Set Name
**Must**: Runtime must return XR_ERROR_NAME_INVALID for empty actionSetName
- Create action set with empty string "" for actionSetName
- Verify XR_ERROR_NAME_INVALID is returned

### TEST-AS-003: Empty Localized Action Set Name
**Must**: Runtime must return XR_ERROR_LOCALIZED_NAME_INVALID for empty localizedActionSetName
- Create action set with empty string "" for localizedActionSetName
- Verify XR_ERROR_LOCALIZED_NAME_INVALID is returned

### TEST-AS-004: Duplicate Action Set Name
**Must**: Runtime must return XR_ERROR_NAME_DUPLICATED for duplicate actionSetName
- Create action set with name "test_set"
- Create second action set with same name "test_set"
- Verify XR_ERROR_NAME_DUPLICATED is returned for second call

### TEST-AS-005: Duplicate Localized Action Set Name
**Must**: Runtime must return XR_ERROR_LOCALIZED_NAME_DUPLICATED for duplicate localizedActionSetName
- Create action set with localizedActionSetName "Test Set"
- Create second action set with same localizedActionSetName "Test Set"
- Verify XR_ERROR_LOCALIZED_NAME_DUPLICATED is returned

### TEST-AS-006: Action Set Name with Invalid Characters
**Must**: Runtime must return XR_ERROR_PATH_FORMAT_INVALID for actionSetName with invalid path characters
- Attempt to create action set with name containing invalid characters (e.g., spaces, special chars not allowed in path)
- Verify XR_ERROR_PATH_FORMAT_INVALID is returned

### TEST-AS-007: Duplicate Name After Destruction
**Must**: Runtime must allow duplicate names after conflicting action set is destroyed
- Create action set with name "test_set"
- Destroy the action set
- Create new action set with same name "test_set"
- Verify XR_SUCCESS is returned

### TEST-AS-008: Destroy Action Set
**Must**: Runtime must destroy action set and all contained actions
- Create action set
- Create multiple actions in the action set
- Destroy action set
- Verify all action handles are also destroyed

### TEST-AS-009: Action Set Priority Binding Resolution
**Must**: Runtime must ignore lower priority action sets when same input bound to higher priority
- Create two action sets with different priorities
- Bind both to same input source
- Verify higher priority action receives input, lower priority shows isActive=FALSE

### TEST-AS-010: Resource Cleanup on Instance Destroy
**Must**: Runtime must free all action set resources when instance is destroyed
- Create instance with action sets
- Destroy instance
- Verify resources are properly freed

---

## Actions

### TEST-A-001: Create Boolean Input Action
**Must**: Runtime must create boolean input action successfully
- Create action with type XR_ACTION_TYPE_BOOLEAN_INPUT
- Verify XR_SUCCESS is returned
- Verify valid handle is returned

### TEST-A-002: Create Float Input Action
**Must**: Runtime must create float input action successfully
- Create action with type XR_ACTION_TYPE_FLOAT_INPUT
- Verify XR_SUCCESS is returned

### TEST-A-003: Create Vector2f Input Action
**Must**: Runtime must create vector2f input action successfully
- Create action with type XR_ACTION_TYPE_VECTOR2F_INPUT
- Verify XR_SUCCESS is returned

### TEST-A-004: Create Pose Input Action
**Must**: Runtime must create pose input action successfully
- Create action with type XR_ACTION_TYPE_POSE_INPUT
- Verify XR_SUCCESS is returned

### TEST-A-005: Create Vibration Output Action
**Must**: Runtime must create vibration output action successfully
- Create action with type XR_ACTION_TYPE_VIBRATION_OUTPUT
- Verify XR_SUCCESS is returned

### TEST-A-006: Empty Action Name
**Must**: Runtime must return XR_ERROR_NAME_INVALID for empty actionName
- Create action with empty actionName
- Verify XR_ERROR_NAME_INVALID is returned

### TEST-A-007: Empty Localized Action Name
**Must**: Runtime must return XR_ERROR_LOCALIZED_NAME_INVALID for empty localizedActionName
- Create action with empty localizedActionName
- Verify XR_ERROR_LOCALIZED_NAME_INVALID is returned

### TEST-A-008: Duplicate Action Name in Same Action Set
**Must**: Runtime must return XR_ERROR_NAME_DUPLICATED for duplicate action names
- Create action with name "test_action" in action set
- Create second action with same name in same action set
- Verify XR_ERROR_NAME_DUPLICATED is returned

### TEST-A-009: Duplicate Localized Action Name
**Must**: Runtime must return XR_ERROR_LOCALIZED_NAME_DUPLICATED
- Create action with localizedActionName "Test Action"
- Create second action with same localizedActionName
- Verify XR_ERROR_LOCALIZED_NAME_DUPLICATED is returned

### TEST-A-010: Action Name with Invalid Path Characters
**Must**: Runtime must return XR_ERROR_PATH_FORMAT_INVALID for invalid characters in actionName
- Create action with name containing invalid path characters
- Verify XR_ERROR_PATH_FORMAT_INVALID is returned

### TEST-A-011: Create Action After Attach
**Must**: Runtime must return XR_ERROR_ACTIONSETS_ALREADY_ATTACHED if action set already attached
- Create action set
- Attach action set to session
- Attempt to create new action in attached action set
- Verify XR_ERROR_ACTIONSETS_ALREADY_ATTACHED is returned

### TEST-A-012: Subaction Paths - Valid Paths
**Must**: Runtime must accept valid subaction paths
- Create action with subactionPaths containing /user/hand/left, /user/hand/right
- Verify XR_SUCCESS is returned

### TEST-A-013: Subaction Paths - Head Path
**Must**: Runtime must accept /user/head as subaction path
- Create action with subactionPath /user/head
- Verify XR_SUCCESS is returned

### TEST-A-014: Subaction Paths - Gamepad Path
**Must**: Runtime must accept /user/gamepad as subaction path
- Create action with subactionPath /user/gamepad
- Verify XR_SUCCESS is returned

### TEST-A-015: Subaction Paths - Invalid Path
**Must**: Runtime must return XR_ERROR_PATH_UNSUPPORTED for invalid subaction paths
- Create action with invalid subaction path (not in allowed list)
- Verify XR_ERROR_PATH_UNSUPPORTED is returned

### TEST-A-016: Subaction Paths - Duplicate Paths
**Must**: Runtime must return XR_ERROR_PATH_UNSUPPORTED if same path appears twice
- Create action with subactionPaths containing /user/hand/left twice
- Verify XR_ERROR_PATH_UNSUPPORTED is returned

### TEST-A-017: Subaction Path Query Mismatch
**Must**: Runtime must return XR_ERROR_PATH_UNSUPPORTED if querying with non-specified subaction path
- Create action with subactionPaths [/user/hand/left]
- Call xrGetActionState* with subactionPath /user/hand/right
- Verify XR_ERROR_PATH_UNSUPPORTED is returned

### TEST-A-018: Subaction Path Empty Array Required
**Must**: Runtime must return XR_ERROR_PATH_UNSUPPORTED if action created with subactionPaths but queried without
- Create action with specific subactionPaths
- Call xrGetActionState* with XR_NULL_PATH
- Verify XR_ERROR_PATH_UNSUPPORTED is returned

### TEST-A-019: Destroy Action
**Must**: Runtime must allow destroying action handle
- Create action
- Destroy action
- Verify XR_SUCCESS is returned

### TEST-A-020: Destroy Action Resources Preserved
**Must**: Runtime must not free underlying resources when action destroyed
- Create action
- Create action space from action
- Destroy action handle
- Verify action space still locatable and action priority still processed

### TEST-A-021: Resources Freed on Instance Destroy
**Must**: Runtime must free all action resources when instance destroyed
- Create actions in instance
- Destroy instance
- Verify all resources properly freed

---

## Suggested Bindings

### TEST-SB-001: Suggest Bindings for Interaction Profile
**Must**: Runtime must accept suggested bindings for valid interaction profile
- Create actions
- Create valid XrInteractionProfileSuggestedBinding
- Call xrSuggestInteractionProfileBindings
- Verify XR_SUCCESS is returned

### TEST-SB-002: Replace Previous Bindings
**Must**: Runtime must discard previous bindings when called again for same profile
- Suggest bindings for interaction profile with action A
- Suggest bindings for same profile with action B
- Verify second call replaces first

### TEST-SB-003: Invalid Interaction Profile Path Format
**Must**: Runtime must return XR_ERROR_PATH_UNSUPPORTED for invalid interaction profile path format
- Suggest bindings with malformed interaction profile path
- Verify XR_ERROR_PATH_UNSUPPORTED is returned

### TEST-SB-004: Invalid Binding Path Format
**Must**: Runtime must return XR_ERROR_PATH_UNSUPPORTED for invalid binding path format
- Suggest bindings with malformed binding path
- Verify XR_ERROR_PATH_UNSUPPORTED is returned

### TEST-SB-005: Interaction Profile Not in Allowlist
**Must**: Runtime must return XR_ERROR_PATH_UNSUPPORTED for unknown interaction profile
- Suggest bindings for interaction profile not in spec allowlist
- Verify XR_ERROR_PATH_UNSUPPORTED is returned

### TEST-SB-006: Binding Path Not in Allowlist
**Must**: Runtime must return XR_ERROR_PATH_UNSUPPORTED for unknown binding path
- Suggest bindings with binding path not in spec allowlist
- Verify XR_ERROR_PATH_UNSUPPORTED is returned

### TEST-SB-007: Runtime Must Accept All Valid Bindings
**Must**: Runtime must accept every valid binding in allowlist (even if ignored)
- Suggest all valid bindings from spec allowlist
- Verify XR_SUCCESS for all valid bindings

### TEST-SB-008: Suggest Bindings After Attach
**Must**: Runtime must return XR_ERROR_ACTIONSETS_ALREADY_ATTACHED
- Create action set with actions
- Attach action set to session
- Attempt xrSuggestInteractionProfileBindings
- Verify XR_ERROR_ACTIONSETS_ALREADY_ATTACHED is returned

### TEST-SB-009: Boolean Action Bound to Boolean Input
**Must**: Boolean input sources must be bound directly to boolean actions
- Create boolean action
- Bind to boolean input (e.g., .../click)
- Verify binding works correctly

### TEST-SB-010: Boolean Action Bound to Scalar Input with Threshold
**Must**: Runtime must apply threshold to scalar values bound to boolean actions
- Create boolean action
- Bind to scalar input (e.g., .../value)
- Verify threshold is applied
- Should verify hysteresis behavior

### TEST-SB-011: Boolean Action Bound to Parent Path Uses Click
**Must**: Runtime must use .../click subpath when parent path bound to boolean action
- Create boolean action
- Bind to parent path (e.g., /user/hand/right/input/trigger)
- Verify runtime uses .../trigger/click

### TEST-SB-012: Boolean Action Parent Path Fallback to Value
**Must**: Runtime must use .../value if .../click unavailable
- Create boolean action
- Bind to parent path that has no .../click subpath
- Verify runtime uses .../value with threshold

### TEST-SB-013: Float Action Bound to Scalar Input
**Must**: Scalar input must be bound directly to float action
- Create float action
- Bind to scalar input
- Verify direct binding

### TEST-SB-014: Float Action Bound to Parent Uses Value
**Must**: Runtime must use .../value when parent path bound to float action
- Create float action
- Bind to parent path
- Verify runtime uses .../value subpath

### TEST-SB-015: Float Action Parent Fallback to Click
**Must**: Runtime must use .../click if .../value unavailable
- Create float action
- Bind to parent path with no .../value
- Verify runtime uses .../click

### TEST-SB-016: Float Action Bound to Boolean Converts to 0.0/1.0
**Must**: Runtime must convert boolean input to 0.0 or 1.0 for float action
- Create float action
- Bind to boolean input
- Verify conversion to 0.0 (false) or 1.0 (true)

### TEST-SB-017: Vector2f Action Requires Parent with X and Y
**Must**: Binding path for vector2f must be parent with .../x and .../y subpaths
- Create vector2f action
- Bind to parent path with .../x and .../y
- Verify binding works

### TEST-SB-018: Vector2f Action X and Y Mapped Correctly
**Must**: Runtime must bind .../x and .../y to vector's x and y
- Create vector2f action
- Bind to joystick/thumbstick
- Verify .../x maps to x, .../y maps to y

### TEST-SB-019: Pose Action Bound to Pose Input
**Must**: Pose input sources must be bound directly to pose actions
- Create pose action
- Bind to pose input (e.g., .../grip/pose)
- Verify binding works

### TEST-SB-020: Pose Action Parent Path Uses Pose Subpath
**Must**: Runtime must use .../pose when parent path bound to pose action
- Create pose action
- Bind to parent path
- Verify runtime uses .../pose subpath if available

---

## Current Interaction Profile

### TEST-CIP-001: Get Current Interaction Profile
**Must**: Runtime must return current interaction profile for top level user path
- Attach action sets
- Call xrGetCurrentInteractionProfile for /user/hand/left
- Verify XR_SUCCESS is returned
- Verify valid interaction profile or XR_NULL_PATH

### TEST-CIP-002: Return Only Suggested Profiles
**Must**: Runtime must return only interaction profiles with suggested bindings or XR_NULL_PATH
- Suggest bindings for profile A
- Get current interaction profile
- Verify returned profile is either A or XR_NULL_PATH

### TEST-CIP-003: May Return Profile for Non-Present Hardware
**May**: Runtime may return interaction profile for hardware not physically present
- Runtime is allowed to return known profile for emulation
- Test should document this is allowed behavior

### TEST-CIP-004: May Return Anticipated Profile
**May**: Runtime may return anticipated profile when no controller active
- Test with no active controllers
- Runtime may return profile from suggested bindings list
- Document this is allowed

### TEST-CIP-005: Get Profile Before Attach
**Must**: Runtime must return XR_ERROR_ACTIONSET_NOT_ATTACHED if called before xrAttachSessionActionSets
- Create session without attaching action sets
- Call xrGetCurrentInteractionProfile
- Verify XR_ERROR_ACTIONSET_NOT_ATTACHED is returned

### TEST-CIP-006: Invalid Top Level User Path
**Must**: Runtime must return XR_ERROR_PATH_UNSUPPORTED for invalid top level path
- Call xrGetCurrentInteractionProfile with invalid path
- Verify XR_ERROR_PATH_UNSUPPORTED is returned

### TEST-CIP-007: Emulated Profile Returns Emulated Path
**Must**: Runtime must return emulated profile path, not actual device path
- If runtime is rebinding to unanticipated device
- Runtime must return the profile it's emulating
- Not the actual hardware profile

### TEST-CIP-008: Unable to Provide Input Returns NULL
**Must**: Runtime must return XR_NULL_PATH if unable to emulate any provided profiles
- Suggest bindings for profiles runtime cannot support
- Verify XR_NULL_PATH is returned

### TEST-CIP-009: Interaction Profile Changed Event
**Must**: Runtime must send XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED when profile changes
- Attach action sets
- Trigger interaction profile change
- Verify event is queued

### TEST-CIP-010: Event Only for Suggested Profiles
**Must**: Event must only be sent for profiles with suggested bindings
- Profile changes should only trigger events for supported profiles
- Verify no events for unsupported profiles

### TEST-CIP-011: Event Only for Running Sessions
**Must**: Event must only be queued for running sessions
- Test with session in different states
- Verify event only queued when session is running

### TEST-CIP-012: Profile Change Only on xrSyncActions
**Must**: Interaction profile selection changes must only happen during xrSyncActions
- Trigger profile change between sync calls
- Verify profile doesn't change until next xrSyncActions

---

## Action State Reading

### TEST-ASR-001: Get Boolean Action State
**Must**: Runtime must return current boolean action state
- Create and bind boolean action
- Call xrGetActionStateBoolean
- Verify XR_SUCCESS and valid state returned

### TEST-ASR-002: Get Float Action State
**Must**: Runtime must return current float action state
- Create and bind float action
- Call xrGetActionStateFloat
- Verify XR_SUCCESS and valid state returned

### TEST-ASR-003: Get Vector2f Action State
**Must**: Runtime must return current vector2f action state
- Create and bind vector2f action
- Call xrGetActionStateVector2f
- Verify XR_SUCCESS and valid state returned

### TEST-ASR-004: Get Pose Action State
**Must**: Runtime must return pose action binding state
- Create and bind pose action
- Call xrGetActionStatePose
- Verify XR_SUCCESS and isActive status

### TEST-ASR-005: Action Type Mismatch
**Must**: Runtime must return XR_ERROR_ACTION_TYPE_MISMATCH for wrong getter
- Create boolean action
- Call xrGetActionStateFloat
- Verify XR_ERROR_ACTION_TYPE_MISMATCH is returned

### TEST-ASR-006: Get State for Unattached Action Set
**Must**: Runtime must return XR_ERROR_ACTIONSET_NOT_ATTACHED
- Create action in action set
- Do not attach action set to session
- Call xrGetActionState*
- Verify XR_ERROR_ACTIONSET_NOT_ATTACHED is returned

### TEST-ASR-007: State Unchanged Between Syncs
**Must**: Action state must not change between xrSyncActions calls
- Call xrSyncActions
- Call xrGetActionState* multiple times
- Verify state is identical for all calls
- Call xrSyncActions again
- Verify state may now differ

### TEST-ASR-008: State Updated After Sync
**Must**: Runtime must update action state after xrSyncActions
- Change hardware state
- Call xrSyncActions
- Call xrGetActionState*
- Verify state reflects hardware changes

### TEST-ASR-009: isActive Updates on Sync
**Must**: isActive must update after xrSyncActions when action set activated/deactivated
- Sync with action set active
- Verify isActive = TRUE
- Sync with action set inactive
- Verify isActive = FALSE

### TEST-ASR-010: Session Not Focused - isActive False
**Must**: When session not focused, isActive must be FALSE
- Lose session focus
- Call xrGetActionState*
- Verify isActive = FALSE

### TEST-ASR-011: Session Not Focused - Haptics Suppressed
**Must**: When session not focused, haptics must be suppressed
- Lose session focus
- Call xrApplyHapticFeedback
- Verify no haptic output occurs

### TEST-ASR-012: lastChangeTime Set Correctly
**Must**: lastChangeTime must be set to best estimate of physical state change
- Trigger input change
- Sync actions
- Get action state
- Verify lastChangeTime is reasonable

### TEST-ASR-013: currentState Combines Subaction Paths
**Must**: currentState must combine sources from specified subactionPaths
- Bind action to multiple subaction paths
- Trigger inputs on multiple sources
- Verify currentState combines them correctly

### TEST-ASR-014: changedSinceLastSync True on State Change
**Must**: changedSinceLastSync must be TRUE when currentState differs from previous sync
- Sync with input at state A
- Change input to state B
- Sync again
- Verify changedSinceLastSync = TRUE

### TEST-ASR-015: changedSinceLastSync False on No Change
**Must**: changedSinceLastSync must be FALSE when currentState unchanged
- Sync with input at state A
- Keep input at state A
- Sync again
- Verify changedSinceLastSync = FALSE

### TEST-ASR-016: changedSinceLastSync False on First Sync
**Must**: changedSinceLastSync must be FALSE if no previous sync
- Create action
- Sync for first time
- Get state
- Verify changedSinceLastSync = FALSE

### TEST-ASR-017: changedSinceLastSync False if Previously Inactive
**Must**: changedSinceLastSync must be FALSE if action was inactive in previous sync
- Sync with action inactive
- Activate action and change state
- Sync
- Verify changedSinceLastSync = FALSE

### TEST-ASR-018: Inactive Action Returns Zero State
**Must**: Inactive action must return zero/false for state
- Make action inactive (unbound or no source)
- Get state
- Verify currentState is 0/FALSE, changedSinceLastSync is FALSE, lastChangeTime is 0

### TEST-ASR-019: isActive True When Bound and Source Present
**Must**: isActive must be TRUE when action bound and source provides data
- Bind action to active source
- Sync
- Get state
- Verify isActive = TRUE

### TEST-ASR-020: isActive False When Unbound
**Must**: isActive must be FALSE when action is unbound
- Create action without bindings
- Sync
- Get state
- Verify isActive = FALSE

### TEST-ASR-021: isActive False When No Source Present
**Must**: isActive must be FALSE when no source is present
- Bind action but remove/disconnect hardware
- Sync
- Get state
- Verify isActive = FALSE

---

## Action State Synchronization

### TEST-SS-001: Sync Actions with Valid Parameters
**Must**: Runtime must sync actions successfully
- Attach action sets
- Create XrActionsSyncInfo with active action sets
- Call xrSyncActions
- Verify XR_SUCCESS is returned

### TEST-SS-002: Sync Unattached Action Set
**Must**: Runtime must return XR_ERROR_ACTIONSET_NOT_ATTACHED for unattached action sets
- Create action set without attaching
- Attempt to sync it
- Verify XR_ERROR_ACTIONSET_NOT_ATTACHED is returned

### TEST-SS-003: Sync Session Not Focused
**Must**: Runtime must return XR_SESSION_NOT_FOCUSED and set all actions inactive
- Lose session focus
- Call xrSyncActions
- Verify XR_SESSION_NOT_FOCUSED is returned
- Verify all action states are inactive

### TEST-SS-004: Sync Subset of Action Sets
**Must**: Runtime must sync only specified subset of bound action sets
- Attach multiple action sets
- Sync only subset
- Verify only specified action sets are active

### TEST-SS-005: Sync with Subaction Path
**Must**: Runtime must sync only specified subaction path
- Create action with multiple subaction paths
- Sync with specific subaction path
- Verify only that subaction path is active

### TEST-SS-006: Sync with NULL Subaction Path Wildcard
**Must**: XR_NULL_PATH as subactionPath must activate all subaction paths
- Create action with multiple subaction paths
- Sync with subactionPath = XR_NULL_PATH
- Verify all subaction paths are active

### TEST-SS-007: Sync with Invalid Subaction Path
**Must**: Runtime must return XR_ERROR_PATH_UNSUPPORTED for invalid subaction path
- Sync with subaction path not specified at action creation
- Verify XR_ERROR_PATH_UNSUPPORTED is returned

### TEST-SS-008: Sync Updates Binding Enumeration
**Must**: Bound sources must not change between xrSyncActions calls
- Call xrSyncActions
- Call xrEnumerateBoundSourcesForAction multiple times
- Verify same sources returned each time
- Call xrSyncActions again
- Verify sources may now differ

---

## Haptics

### TEST-H-001: Apply Haptic Feedback
**Must**: Runtime should deliver haptic feedback to appropriate device
- Create vibration output action
- Bind to haptic output
- Call xrApplyHapticFeedback with XrHapticVibration
- Verify XR_SUCCESS is returned

### TEST-H-002: Haptic Event Immediate Activation
**Must**: Output actions must activate immediately, not wait for xrSyncActions
- Apply haptic feedback
- Verify haptics trigger without calling xrSyncActions

### TEST-H-003: Latest Haptic Replaces Previous
**Must**: Runtime must cancel preceding incomplete haptic events when new one sent
- Apply haptic event with long duration
- Apply second haptic event before first completes
- Verify second event replaces first

### TEST-H-004: Haptic Session Not Focused
**Must**: Runtime must return XR_SESSION_NOT_FOCUSED and discard haptics when session not focused
- Lose session focus
- Call xrApplyHapticFeedback
- Verify XR_SESSION_NOT_FOCUSED is returned
- Verify no haptic output

### TEST-H-005: Stop Haptics on Focus Loss
**Should**: Runtime should stop in-progress haptics when session loses focus
- Start long haptic event
- Lose session focus
- Verify haptic stops

### TEST-H-006: Haptic Action Type Mismatch
**Must**: Runtime must return XR_ERROR_ACTION_TYPE_MISMATCH for non-haptic action
- Create boolean input action
- Call xrApplyHapticFeedback with input action
- Verify XR_ERROR_ACTION_TYPE_MISMATCH is returned

### TEST-H-007: Haptic Unattached Action Set
**Must**: Runtime must return XR_ERROR_ACTIONSET_NOT_ATTACHED
- Create haptic action in unattached action set
- Call xrApplyHapticFeedback
- Verify XR_ERROR_ACTIONSET_NOT_ATTACHED is returned

### TEST-H-008: XR_MIN_HAPTIC_DURATION
**Must**: Runtime must produce short pulse of minimal duration for XR_MIN_HAPTIC_DURATION
- Call xrApplyHapticFeedback with duration = XR_MIN_HAPTIC_DURATION
- Verify short haptic pulse occurs

### TEST-H-009: XR_FREQUENCY_UNSPECIFIED
**Must**: Runtime must choose optimal frequency when XR_FREQUENCY_UNSPECIFIED used
- Call xrApplyHapticFeedback with frequency = XR_FREQUENCY_UNSPECIFIED
- Verify runtime selects reasonable frequency

### TEST-H-010: Haptic Amplitude Range
**Must**: Runtime may clamp amplitude between 0.0 and 1.0
- Test haptic with amplitude = 0.0, 0.5, 1.0
- Verify values are accepted
- Implementation may clamp out-of-range values

### TEST-H-011: Haptic Duration Clamping
**May**: Runtime may clamp duration to implementation limits
- Test haptic with very long duration
- Runtime may clamp to max supported duration

### TEST-H-012: Haptic Frequency Clamping
**May**: Runtime may clamp frequency to implementation limits
- Test haptic with various frequencies
- Runtime may clamp to supported range

### TEST-H-013: Stop Haptic Feedback
**Must**: Runtime must stop in-progress haptic event
- Start long haptic event
- Call xrStopHapticFeedback
- Verify XR_SUCCESS is returned
- Verify haptic stops

### TEST-H-014: Stop Haptic When Not Focused
**Must**: xrStopHapticFeedback must return XR_SESSION_NOT_FOCUSED when session not focused
- Lose session focus
- Call xrStopHapticFeedback
- Verify XR_SESSION_NOT_FOCUSED is returned

### TEST-H-015: Haptic Multiple Outputs
**Must**: Runtime must send haptic output to all bound haptic devices when multiple bound
- Bind haptic action to multiple outputs
- Apply haptic feedback
- Verify all bound devices receive output

### TEST-H-016: Haptic with Subaction Path
**Must**: Runtime must trigger haptics only on specified subaction path device
- Create haptic action with multiple subaction paths
- Apply haptic with specific subactionPath
- Verify only that device receives haptics

---

## Bound Sources

### TEST-BS-001: Enumerate Bound Sources
**Must**: Runtime must return bound sources for action
- Create and bind action
- Attach action sets
- Call xrEnumerateBoundSourcesForAction
- Verify XR_SUCCESS and sources returned

### TEST-BS-002: Unbound Action Returns Empty
**Must**: Runtime must return sourceCountOutput=0 for unbound action
- Create action without bindings
- Enumerate bound sources
- Verify sourceCountOutput = 0 and array not modified

### TEST-BS-003: Enumerate Before Attach
**Must**: Runtime must return XR_ERROR_ACTIONSET_NOT_ATTACHED
- Create action in action set
- Do not attach action set
- Call xrEnumerateBoundSourcesForAction
- Verify XR_ERROR_ACTIONSET_NOT_ATTACHED is returned

### TEST-BS-004: Bound Sources Stable Between Syncs
**Must**: Bound sources must not change between xrSyncActions calls
- Call xrSyncActions
- Enumerate bound sources multiple times
- Verify same sources each time
- Call xrSyncActions
- Sources may now differ

### TEST-BS-005: Get Input Source Localized Name
**Must**: Runtime must return localized human-readable name
- Enumerate bound sources
- Call xrGetInputSourceLocalizedName for each source
- Verify XR_SUCCESS and non-empty string returned

### TEST-BS-006: Localized Name Before Attach
**Must**: Runtime must return XR_ERROR_ACTIONSET_NOT_ATTACHED
- Call xrGetInputSourceLocalizedName before attaching action sets
- Verify XR_ERROR_ACTIONSET_NOT_ATTACHED is returned

### TEST-BS-007: Localized Name Components - User Path
**Must**: Runtime must include user path when USER_PATH_BIT set
- Get localized name with XR_INPUT_SOURCE_LOCALIZED_NAME_USER_PATH_BIT
- Verify user path (e.g., "Left Hand") in result if available

### TEST-BS-008: Localized Name Components - Interaction Profile
**Must**: Runtime must include interaction profile when INTERACTION_PROFILE_BIT set
- Get localized name with XR_INPUT_SOURCE_LOCALIZED_NAME_INTERACTION_PROFILE_BIT
- Verify profile name (e.g., "Vive Controller") in result if available

### TEST-BS-009: Localized Name Components - Component
**Must**: Runtime must include component when COMPONENT_BIT set
- Get localized name with XR_INPUT_SOURCE_LOCALIZED_NAME_COMPONENT_BIT
- Verify component name (e.g., "Trigger") in result if available

### TEST-BS-010: Localized Name Multiple Components
**Must**: Runtime must include all requested components
- Get localized name with multiple flag bits set
- Verify all requested components in result

---

## Priority and Multi-Binding

### TEST-PMB-001: Boolean OR for Multiple Boolean Inputs
**Must**: Boolean action bound to multiple inputs must use boolean OR
- Bind boolean action to two boolean inputs
- Activate one input
- Verify currentState = TRUE
- Activate both
- Verify currentState = TRUE
- Deactivate both
- Verify currentState = FALSE

### TEST-PMB-002: Float Largest Absolute Value
**Must**: Float action bound to multiple inputs must use largest absolute value
- Bind float action to two scalar inputs
- Set one to 0.3, other to -0.7
- Verify currentState = -0.7 (largest absolute)

### TEST-PMB-003: Vector2f Longest Length
**Must**: Vector2f action bound to multiple inputs must use longest length
- Bind vector2f action to two joystick inputs
- Set one to (0.5, 0.5), other to (0.9, 0.9)
- Verify currentState = (0.9, 0.9)

### TEST-PMB-004: Pose Single Source
**Must**: Pose action must use single pose source
- Bind pose action to multiple sources
- Verify only one source is active
- Source should only change during xrSyncActions

### TEST-PMB-005: Pose Source Change on Sync Only
**Should**: Pose source should only change during xrSyncActions
- Monitor which pose source is active
- Verify changes only happen during sync calls

### TEST-PMB-006: Pose Source Change Reasons
**Should**: Pose source should only change due to user actions or external events
- Source changes should be due to picking up controller, battery dying, etc.
- Not arbitrary runtime decisions

### TEST-PMB-007: Same Input Source Definition
**Must**: Two actions bound to same source uses same identifier and location paths
- Actions bound to same input with different component segments are considered same source
- E.g., .../trigger/value and .../trigger/click are same source

### TEST-PMB-008: Priority Suppression - isActive False
**Must**: Lower priority binding must have isActive=FALSE when higher priority active
- Create two action sets with different priorities bound to same input
- Sync with both active
- Verify higher priority has isActive=TRUE
- Verify lower priority has isActive=FALSE

### TEST-PMB-009: Priority Suppression - No Feedback
**Must**: Runtime must not provide feedback for suppressed bindings
- Lower priority binding must receive no visual, haptic, or other feedback
- Verify completely suppressed behavior

### TEST-PMB-010: Priority Non-Colliding Bindings Unaffected
**Must**: Other actions in lower priority set not colliding with higher priority must work
- Action set A (priority 1) binds trigger
- Action set B (priority 0) binds trigger and grip
- Both active
- Verify B's trigger suppressed, B's grip works normally

### TEST-PMB-011: Same Priority Multiple Bindings
**Must**: When multiple action sets with same priority bound to same source, process all
- Create two action sets with same priority
- Bind both to same input
- Verify both are processed simultaneously

### TEST-PMB-012: Priority Treated as Unbound
**Must**: Suppressed binding must be treated as if it does not exist
- Verify suppressed binding behaves exactly as if binding was never suggested

---

## Edge Cases and Error Conditions

### TEST-EC-001: NULL Handle Parameters
**Must**: Runtime must return XR_ERROR_HANDLE_INVALID for NULL handles
- Call functions with XR_NULL_HANDLE
- Verify XR_ERROR_HANDLE_INVALID is returned

### TEST-EC-002: Invalid XrPath Values
**Must**: Runtime must return XR_ERROR_PATH_INVALID for invalid path values
- Use invalid XrPath values
- Verify XR_ERROR_PATH_INVALID is returned

### TEST-EC-003: Out of Memory Conditions
**Must**: Runtime must return XR_ERROR_OUT_OF_MEMORY when resources exhausted
- Create actions/action sets until resources exhausted
- Verify XR_ERROR_OUT_OF_MEMORY is returned

### TEST-EC-004: Limit Reached
**Must**: Runtime must return XR_ERROR_LIMIT_REACHED when implementation limits hit
- Create maximum number of actions/action sets
- Attempt to create one more
- Verify XR_ERROR_LIMIT_REACHED is returned

### TEST-EC-005: Thread Safety - Action Destruction
**Must**: Access to action and child handles must be externally synchronized
- Document requirement for external synchronization during destruction
- Test parallel access causes expected behavior

### TEST-EC-006: Thread Safety - Action Set Destruction
**Must**: Access to action set and child handles must be externally synchronized
- Document requirement for external synchronization during destruction
- Test parallel access causes expected behavior

### TEST-EC-007: Session State Transitions
**Must**: Verify correct behavior during session state transitions
- Test action state during IDLE, READY, SYNCHRONIZED, VISIBLE, FOCUSED states
- Verify appropriate error codes or behavior

### TEST-EC-008: Instance Lost
**Must**: Runtime must return XR_ERROR_INSTANCE_LOST when instance is lost
- Trigger instance loss
- Call action functions
- Verify XR_ERROR_INSTANCE_LOST is returned

### TEST-EC-009: Session Lost
**Must**: Runtime must return XR_ERROR_SESSION_LOST when session is lost
- Trigger session loss
- Call session-related action functions
- Verify XR_ERROR_SESSION_LOST is returned

---

## Test Execution Guidelines

### Prerequisites
- OpenXR runtime must be initialized and instance created
- Session must be created and appropriate session state achieved
- Test input devices must be available and configured
- Input simulator or hardware available for triggering inputs

### Test Data Requirements
- Valid interaction profile paths from spec allowlist
- Valid binding paths for each interaction profile
- Valid top-level user paths (/user/hand/left, /user/hand/right, /user/head, /user/gamepad)
- Test action names and localized names

### Validation Approach
- Each "Must" requirement should be a separate automated test
- Each "Should" requirement should be tested and documented if not supported
- Each "May" requirement should be documented as allowed behavior
- Return codes must be checked against expected values
- State values must be validated for correctness and consistency
- Timing constraints must be verified (e.g., state unchanged between syncs)

### Coverage Requirements
- All action types (boolean, float, vector2f, pose, vibration output)
- All valid subaction paths
- All error conditions
- Priority resolution
- Multi-binding scenarios
- Focus state handling
- Session state transitions

---

## Notes

- This document covers OpenXR 1.0 core specification section 11
- Extensions may add additional requirements not covered here
- Tests should be run with various interaction profiles to ensure broad compatibility
- Hardware-specific behavior should be documented per device
- Timing-sensitive tests may need tolerance values for real hardware
- Some tests require physical interaction or hardware simulation capabilities
