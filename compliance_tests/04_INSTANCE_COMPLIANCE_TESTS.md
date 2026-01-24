# OpenXR Instance Compliance Test Cases (Section 4)

This document defines compliance test cases for OpenXR Instance functionality based on the OpenXR specification section 4. These tests ensure that the ox runtime correctly implements all required behaviors for instance creation, lifecycle, properties, and enumeration.

## Test Categories

- [API Layer Enumeration](#api-layer-enumeration)
- [Extension Enumeration](#extension-enumeration)
- [Instance Creation](#instance-creation)
- [Instance Destruction](#instance-destruction)
- [Instance Properties](#instance-properties)
- [Instance Lost](#instance-lost)
- [Enumerated Type String Functions](#enumerated-type-string-functions)
- [Concurrent Instances](#concurrent-instances)

---

## API Layer Enumeration

### TEST-ALE-001: Enumerate API Layers - Two-Call Idiom
**Must**: Runtime must support two-call idiom for enumerating API layers
- Call xrEnumerateApiLayerProperties with propertyCapacityInput=0 and NULL properties
- Verify XR_SUCCESS is returned
- Verify propertyCountOutput is set to required count
- Allocate buffer with returned count
- Call xrEnumerateApiLayerProperties again with allocated buffer
- Verify XR_SUCCESS is returned
- Verify propertyCountOutput contains actual count written

### TEST-ALE-002: Enumerate API Layers - Sufficient Buffer
**Must**: Runtime must return success when buffer is sufficient
- Call xrEnumerateApiLayerProperties with sufficient propertyCapacityInput
- Verify XR_SUCCESS is returned
- Verify propertyCountOutput <= propertyCapacityInput
- Verify properties array is populated

### TEST-ALE-003: Enumerate API Layers - Insufficient Buffer
**Must**: Runtime must return XR_ERROR_SIZE_INSUFFICIENT when buffer too small
- Get required count via two-call idiom
- Call xrEnumerateApiLayerProperties with propertyCapacityInput < required count
- Verify XR_ERROR_SIZE_INSUFFICIENT is returned
- Verify propertyCountOutput is set to required capacity

### TEST-ALE-004: Enumerate API Layers - NULL Count Pointer
**Must**: Runtime must return XR_ERROR_VALIDATION_FAILURE for NULL propertyCountOutput
- Call xrEnumerateApiLayerProperties with propertyCountOutput=NULL
- Verify XR_ERROR_VALIDATION_FAILURE is returned

### TEST-ALE-005: Enumerate API Layers - Non-Zero Capacity with NULL Buffer
**Must**: Runtime must return XR_ERROR_VALIDATION_FAILURE when propertyCapacityInput > 0 but properties is NULL
- Call xrEnumerateApiLayerProperties with propertyCapacityInput > 0 and properties=NULL
- Verify XR_ERROR_VALIDATION_FAILURE is returned

### TEST-ALE-006: Enumerate API Layers - Structure Type Validation
**Must**: Runtime must fill XrApiLayerProperties.type with XR_TYPE_API_LAYER_PROPERTIES
- Enumerate API layers successfully
- Verify each returned structure has type=XR_TYPE_API_LAYER_PROPERTIES

### TEST-ALE-007: Enumerate API Layers - Field Population
**Must**: Runtime must populate all XrApiLayerProperties fields
- Enumerate API layers successfully
- For each layer, verify:
  - layerName is non-empty null-terminated string
  - specVersion is valid version number
  - layerVersion is populated
  - description is null-terminated string (may be empty)

### TEST-ALE-008: Enumerate API Layers - May Change Between Calls
**May**: List of available layers may change between calls
- Document that two calls may return different results
- Test that runtime handles external changes gracefully

### TEST-ALE-009: Enumerate API Layers - Enabled Layers Persist
**Must**: Once instance created, enabled layers remain valid for instance lifetime
- Enumerate layers and note available layers
- Create instance with specific layer enabled
- Remove layer from system (if possible in test environment)
- Verify instance continues to function with enabled layer
- Verify layer remains functional for instance lifetime

---

## Extension Enumeration

### TEST-EE-001: Enumerate Extensions - Two-Call Idiom
**Must**: Runtime must support two-call idiom for enumerating extensions
- Call xrEnumerateInstanceExtensionProperties with propertyCapacityInput=0, layerName=NULL
- Verify XR_SUCCESS is returned
- Verify propertyCountOutput is set to required count
- Allocate buffer with returned count
- Call again with allocated buffer
- Verify XR_SUCCESS and properties populated

### TEST-EE-002: Enumerate Extensions - Insufficient Buffer
**Must**: Runtime must return XR_ERROR_SIZE_INSUFFICIENT when buffer too small
- Get required count via two-call idiom
- Call with propertyCapacityInput < required count
- Verify XR_ERROR_SIZE_INSUFFICIENT is returned
- Verify propertyCountOutput set to required capacity

### TEST-EE-003: Enumerate Extensions - NULL layerName
**Must**: Runtime must enumerate runtime extensions when layerName is NULL
- Call xrEnumerateInstanceExtensionProperties with layerName=NULL
- Verify XR_SUCCESS is returned
- Verify extensions returned are runtime extensions

### TEST-EE-004: Enumerate Extensions - Specific Layer
**Must**: Runtime must enumerate layer-specific extensions when layerName provided
- Call xrEnumerateInstanceExtensionProperties with valid layerName
- Verify XR_SUCCESS is returned
- Verify extensions returned are for specified layer

### TEST-EE-005: Enumerate Extensions - Invalid Layer Name
**Must**: Runtime must return XR_ERROR_API_LAYER_NOT_PRESENT for non-existent layer
- Call xrEnumerateInstanceExtensionProperties with non-existent layerName
- Verify XR_ERROR_API_LAYER_NOT_PRESENT is returned

### TEST-EE-006: Enumerate Extensions - NULL layerName Must Be UTF-8
**Must**: If layerName is not NULL, it must be null-terminated UTF-8 string
- Validate parameter checking for non-NULL layerName
- Verify proper UTF-8 validation

### TEST-EE-007: Enumerate Extensions - Structure Type Validation
**Must**: Runtime must fill XrExtensionProperties.type with XR_TYPE_EXTENSION_PROPERTIES
- Enumerate extensions successfully
- Verify each structure has type=XR_TYPE_EXTENSION_PROPERTIES

### TEST-EE-008: Enumerate Extensions - Field Population
**Must**: Runtime must populate all XrExtensionProperties fields
- Enumerate extensions successfully
- For each extension, verify:
  - extensionName is non-empty null-terminated string
  - extensionVersion is populated

### TEST-EE-009: Enumerate Extensions - NULL Count Pointer
**Must**: Runtime must return XR_ERROR_VALIDATION_FAILURE for NULL propertyCountOutput
- Call xrEnumerateInstanceExtensionProperties with propertyCountOutput=NULL
- Verify XR_ERROR_VALIDATION_FAILURE is returned

### TEST-EE-010: Enumerate Extensions - May Change Between Calls
**May**: List of available extensions may change between calls
- Document that two calls may return different results due to external changes
- Test runtime handles changes gracefully

---

## Instance Creation

### TEST-IC-001: Create Instance - Valid Parameters
**Must**: Runtime must create instance with valid parameters
- Create XrInstanceCreateInfo with valid applicationInfo
- Set valid applicationName (non-empty)
- Set valid apiVersion (e.g., XR_API_VERSION_1_0)
- Call xrCreateInstance
- Verify XR_SUCCESS is returned
- Verify valid XrInstance handle is returned

### TEST-IC-002: Create Instance - Empty Application Name
**Must**: Runtime must return XR_ERROR_NAME_INVALID for empty applicationName
- Create XrInstanceCreateInfo with applicationInfo.applicationName=""
- Call xrCreateInstance
- Verify XR_ERROR_NAME_INVALID is returned
- Verify no instance is created

### TEST-IC-003: Create Instance - Unsupported API Version
**Must**: Runtime must return XR_ERROR_API_VERSION_UNSUPPORTED for unsupported apiVersion
- Create XrInstanceCreateInfo with apiVersion the runtime doesn't support (e.g., very high version)
- Call xrCreateInstance
- Verify XR_ERROR_API_VERSION_UNSUPPORTED is returned
- Verify no instance is created

### TEST-IC-004: Create Instance - API Layer Not Present
**Must**: Runtime must return XR_ERROR_API_LAYER_NOT_PRESENT if specified layer not found
- Create XrInstanceCreateInfo with non-existent layer in enabledApiLayerNames
- Call xrCreateInstance
- Verify XR_ERROR_API_LAYER_NOT_PRESENT is returned
- Verify no instance is created

### TEST-IC-005: Create Instance - Extension Not Present
**Must**: Runtime must return XR_ERROR_EXTENSION_NOT_PRESENT if specified extension not found
- Create XrInstanceCreateInfo with non-existent extension in enabledExtensionNames
- Call xrCreateInstance
- Verify XR_ERROR_EXTENSION_NOT_PRESENT is returned
- Verify no instance is created

### TEST-IC-006: Create Instance - Valid API Layer
**Must**: Runtime must enable specified API layer
- Enumerate available API layers
- Create instance with one available layer in enabledApiLayerNames
- Verify XR_SUCCESS is returned
- Verify layer is active for instance lifetime

### TEST-IC-007: Create Instance - Valid Extension
**Must**: Runtime must enable specified extension
- Enumerate available extensions
- Create instance with one available extension in enabledExtensionNames
- Verify XR_SUCCESS is returned
- Verify extension functions are available

### TEST-IC-008: Create Instance - Multiple API Layers
**Must**: Runtime must enable multiple API layers
- Create instance with multiple valid layers in enabledApiLayerNames
- Verify XR_SUCCESS is returned
- Verify all layers are active

### TEST-IC-009: Create Instance - Multiple Extensions
**Must**: Runtime must enable multiple extensions
- Create instance with multiple valid extensions in enabledExtensionNames
- Verify XR_SUCCESS is returned
- Verify all extension functions are available

### TEST-IC-010: Create Instance - Layer and Extension Together
**Must**: Runtime must enable both if extension provided by layer and both specified
- Find extension provided by a layer
- Create instance with both layer and extension specified
- Verify XR_SUCCESS is returned
- Verify both layer and extension are enabled

### TEST-IC-011: Create Instance - Extension Dependency Not Enabled
**Must**: Runtime must return XR_ERROR_EXTENSION_DEPENDENCY_NOT_ENABLED if dependency missing
- Find extension with dependencies
- Create instance with extension but without required dependency
- Verify XR_ERROR_EXTENSION_DEPENDENCY_NOT_ENABLED is returned
- Verify no instance is created

### TEST-IC-012: Create Instance - Limit Reached
**May**: Runtime may return XR_ERROR_LIMIT_REACHED if max concurrent instances exceeded
- Create maximum allowed concurrent instances
- Attempt to create one more instance
- If XR_ERROR_LIMIT_REACHED returned, verify previous instances still valid
- Document runtime's concurrent instance limit

### TEST-IC-013: Create Instance - Structure Type Validation
**Must**: XrInstanceCreateInfo.type must be XR_TYPE_INSTANCE_CREATE_INFO
- Create XrInstanceCreateInfo with type=XR_TYPE_INSTANCE_CREATE_INFO
- Verify validation accepts correct type

### TEST-IC-014: Create Instance - CreateFlags Must Be Zero
**Must**: XrInstanceCreateInfo.createFlags must be 0
- Attempt to create instance with createFlags != 0
- Verify XR_ERROR_VALIDATION_FAILURE is returned (implicit validation)

### TEST-IC-015: Create Instance - NULL createInfo
**Must**: Runtime must return XR_ERROR_VALIDATION_FAILURE for NULL createInfo
- Call xrCreateInstance with createInfo=NULL
- Verify XR_ERROR_VALIDATION_FAILURE is returned

### TEST-IC-016: Create Instance - NULL instance
**Must**: Runtime must return XR_ERROR_VALIDATION_FAILURE for NULL instance pointer
- Call xrCreateInstance with instance=NULL
- Verify XR_ERROR_VALIDATION_FAILURE is returned

### TEST-IC-017: Create Instance - Platform-Specific Extension Missing
**Must**: Runtime must return XR_ERROR_INITIALIZATION_FAILED if mandatory platform extension missing
- On platforms with mandatory extensions (e.g., Android requires XR_KHR_android_create_instance)
- Create instance without platform-specific extension structure in next chain
- Verify XR_ERROR_INITIALIZATION_FAILED is returned

### TEST-IC-018: Create Instance - Wrong Platform Extension
**May**: Runtime may return XR_ERROR_INITIALIZATION_FAILED for wrong platform extension
- Include platform extension for different platform in next chain
- Verify runtime handles gracefully (may return XR_ERROR_INITIALIZATION_FAILED)

### TEST-IC-019: Create Instance - Valid Engine Name
**Must**: Runtime must accept valid engineName (may be empty)
- Create instance with engineName="" (empty string)
- Verify XR_SUCCESS is returned
- Create instance with engineName="TestEngine"
- Verify XR_SUCCESS is returned

### TEST-IC-020: Create Instance - ApplicationInfo Field Lengths
**Must**: Runtime must validate string lengths
- Verify applicationName length <= XR_MAX_APPLICATION_NAME_SIZE (including null terminator)
- Verify engineName length <= XR_MAX_ENGINE_NAME_SIZE (including null terminator)

### TEST-IC-021: Create Instance - API Version 1.0
**Must**: Runtime must support XR_API_VERSION_1_0
- Create instance with apiVersion=XR_API_VERSION_1_0
- Verify XR_SUCCESS is returned

### TEST-IC-022: Create Instance - API Version 1.1
**Should**: Runtime should support XR_API_VERSION_1_1 if implementing OpenXR 1.1
- Create instance with apiVersion=XR_API_VERSION_1_1
- Verify XR_SUCCESS or XR_ERROR_API_VERSION_UNSUPPORTED
- Document supported API versions

### TEST-IC-023: Create Instance - Duplicate Layer Names
**Must**: Runtime must handle duplicate layer names gracefully
- Create instance with same layer name twice in enabledApiLayerNames
- Verify runtime behavior (may enable once or return error)

### TEST-IC-024: Create Instance - Duplicate Extension Names
**Must**: Runtime must handle duplicate extension names gracefully
- Create instance with same extension name twice in enabledExtensionNames
- Verify runtime behavior (may enable once or return error)

### TEST-IC-025: Create Instance - Zero API Layer Count with NULL Names
**Must**: Runtime must accept enabledApiLayerCount=0 with enabledApiLayerNames=NULL
- Create instance with enabledApiLayerCount=0 and enabledApiLayerNames=NULL
- Verify XR_SUCCESS is returned

### TEST-IC-026: Create Instance - Zero Extension Count with NULL Names
**Must**: Runtime must accept enabledExtensionCount=0 with enabledExtensionNames=NULL
- Create instance with enabledExtensionCount=0 and enabledExtensionNames=NULL
- Verify XR_SUCCESS is returned

### TEST-IC-027: Create Instance - Non-Zero Count with NULL Names
**Must**: Runtime must return XR_ERROR_VALIDATION_FAILURE for non-zero count with NULL names
- Create instance with enabledApiLayerCount > 0 but enabledApiLayerNames=NULL
- Verify XR_ERROR_VALIDATION_FAILURE is returned
- Create instance with enabledExtensionCount > 0 but enabledExtensionNames=NULL
- Verify XR_ERROR_VALIDATION_FAILURE is returned

---

## Instance Destruction

### TEST-ID-001: Destroy Instance - Valid Handle
**Must**: Runtime must destroy instance successfully
- Create instance
- Call xrDestroyInstance with valid handle
- Verify XR_SUCCESS is returned

### TEST-ID-002: Destroy Instance - Invalid Handle
**Must**: Runtime must return XR_ERROR_HANDLE_INVALID for invalid instance
- Call xrDestroyInstance with XR_NULL_HANDLE
- Verify XR_ERROR_HANDLE_INVALID is returned

### TEST-ID-003: Destroy Instance - Destroys Child Handles
**Must**: Runtime must destroy all child handles when instance destroyed
- Create instance
- Create child handles (session, action sets, etc.)
- Destroy instance
- Verify all child handles are destroyed and unusable

### TEST-ID-004: Destroy Instance - Thread Safety
**Must**: Access to instance and child handles must be externally synchronized
- Document that destroying instance while child handles in use requires external synchronization
- Concurrent access to instance during destruction is application error

### TEST-ID-005: Destroy Instance - Resource Cleanup
**Must**: Runtime must free all resources associated with instance
- Create instance with multiple child objects
- Destroy instance
- Verify all resources are properly freed (no memory leaks)

### TEST-ID-006: Destroy Instance - After Instance Lost
**Must**: Application must destroy instance after XR_ERROR_INSTANCE_LOST
- Trigger instance loss
- Destroy instance
- Verify XR_SUCCESS is returned

---

## Instance Properties

### TEST-IP-001: Get Instance Properties - Valid Handle
**Must**: Runtime must return instance properties successfully
- Create instance
- Call xrGetInstanceProperties with valid instance handle
- Verify XR_SUCCESS is returned
- Verify XrInstanceProperties is populated

### TEST-IP-002: Get Instance Properties - Invalid Handle
**Must**: Runtime must return XR_ERROR_HANDLE_INVALID for invalid instance
- Call xrGetInstanceProperties with invalid/NULL instance handle
- Verify XR_ERROR_HANDLE_INVALID is returned

### TEST-IP-003: Get Instance Properties - NULL Properties Pointer
**Must**: Runtime must return XR_ERROR_VALIDATION_FAILURE for NULL properties
- Call xrGetInstanceProperties with instanceProperties=NULL
- Verify XR_ERROR_VALIDATION_FAILURE is returned

### TEST-IP-004: Get Instance Properties - Structure Type
**Must**: Runtime must validate XrInstanceProperties.type is XR_TYPE_INSTANCE_PROPERTIES
- Set up XrInstanceProperties with correct type
- Call xrGetInstanceProperties
- Verify type remains XR_TYPE_INSTANCE_PROPERTIES

### TEST-IP-005: Get Instance Properties - Runtime Version
**Must**: Runtime must populate runtimeVersion field
- Call xrGetInstanceProperties
- Verify runtimeVersion is set to valid version number
- Verify version format follows XR_MAKE_VERSION encoding

### TEST-IP-006: Get Instance Properties - Runtime Name
**Must**: Runtime must populate runtimeName with non-empty string
- Call xrGetInstanceProperties
- Verify runtimeName is non-empty null-terminated string
- Verify length <= XR_MAX_RUNTIME_NAME_SIZE

### TEST-IP-007: Get Instance Properties - Instance Lost
**Must**: Runtime must return XR_ERROR_INSTANCE_LOST after instance loss
- Create instance
- Trigger instance loss
- Call xrGetInstanceProperties
- Verify XR_ERROR_INSTANCE_LOST is returned

---

## Instance Lost

### TEST-IL-001: Instance Lost Error - First Occurrence
**Must**: Runtime must return XR_ERROR_INSTANCE_LOST when instance becomes unusable
- Trigger instance loss condition
- Call any instance or child function
- Verify XR_ERROR_INSTANCE_LOST is returned

### TEST-IL-002: Instance Lost Error - Subsequent Calls
**Must**: Runtime must continue returning XR_ERROR_INSTANCE_LOST after first occurrence
- Trigger instance loss
- Call multiple different functions
- Verify all return XR_ERROR_INSTANCE_LOST

### TEST-IL-003: Instance Lost Error - All Child Functions
**Must**: All functions involving instance or child handles must return XR_ERROR_INSTANCE_LOST
- Trigger instance loss
- Call functions on sessions, action sets, actions, spaces, etc.
- Verify all return XR_ERROR_INSTANCE_LOST

### TEST-IL-004: Instance Lost Error - Destroy Still Works
**Must**: xrDestroyInstance must still work after instance lost
- Trigger instance loss
- Call xrDestroyInstance
- Verify XR_SUCCESS is returned (or XR_ERROR_INSTANCE_LOST is acceptable)

### TEST-IL-005: Instance Lost Event - May Be Generated
**May**: Runtime may generate XrEventDataInstanceLossPending event
- Monitor events when instance loss is about to occur
- Document whether runtime generates event
- If event generated, verify lossTime is in the future

### TEST-IL-006: Instance Lost Event - lossTime Field
**Must**: If XrEventDataInstanceLossPending generated, lossTime must be future time
- Receive XrEventDataInstanceLossPending event
- Verify lossTime > current time when event received
- Verify instance becomes unusable at or after lossTime

### TEST-IL-007: Instance Lost Event - Application Response
**Must**: Application must destroy instance after loss
- Receive instance loss indication
- Call xrDestroyInstance
- Verify resources are cleaned up

### TEST-IL-008: Instance Recreation After Loss
**Must**: Runtime must allow instance recreation after loss and cleanup
- Trigger instance loss
- Destroy instance
- Wait past lossTime if event was received
- Attempt xrCreateInstance
- Verify either XR_SUCCESS or XR_ERROR_RUNTIME_UNAVAILABLE (if maintenance ongoing)

### TEST-IL-009: Instance Lost - Runtime Unavailable During Maintenance
**Must**: Runtime must return XR_ERROR_RUNTIME_UNAVAILABLE during maintenance
- After instance loss and destruction
- If runtime performing maintenance
- Call xrCreateInstance
- Verify XR_ERROR_RUNTIME_UNAVAILABLE is returned

### TEST-IL-010: Instance Lost - Recovery After Maintenance
**Must**: Runtime must return to XR_SUCCESS after maintenance completes
- Wait for maintenance to complete
- Retry xrCreateInstance
- Verify XR_SUCCESS is returned when runtime ready

---

## Enumerated Type String Functions

### TEST-ETSF-001: xrResultToString - Valid Result Values
**Must**: Runtime must return correct string for valid XrResult values
- Call xrResultToString with XR_SUCCESS (value 0)
- Verify buffer contains "XR_SUCCESS"
- Call with various success codes
- Verify correct strings returned

### TEST-ETSF-002: xrResultToString - Error Result Values
**Must**: Runtime must return correct string for error XrResult values
- Call xrResultToString with XR_ERROR_RUNTIME_FAILURE
- Verify buffer contains "XR_ERROR_RUNTIME_FAILURE"
- Call with various error codes
- Verify correct strings returned

### TEST-ETSF-003: xrResultToString - Unknown Success Value
**Must**: Runtime must return "XR_UNKNOWN_SUCCESS_" for unknown positive values
- Call xrResultToString with unknown positive result value
- Verify buffer contains "XR_UNKNOWN_SUCCESS_" followed by decimal number

### TEST-ETSF-004: xrResultToString - Unknown Failure Value
**Must**: Runtime must return "XR_UNKNOWN_FAILURE_" for unknown negative values
- Call xrResultToString with unknown negative result value
- Verify buffer contains "XR_UNKNOWN_FAILURE_" followed by decimal number

### TEST-ETSF-005: xrResultToString - Invalid Instance Handle
**Must**: Runtime must return XR_ERROR_HANDLE_INVALID for invalid instance
- Call xrResultToString with invalid instance handle
- Verify XR_ERROR_HANDLE_INVALID is returned

### TEST-ETSF-006: xrResultToString - Buffer Size
**Must**: Runtime must respect XR_MAX_RESULT_STRING_SIZE buffer size
- Verify buffer is exactly XR_MAX_RESULT_STRING_SIZE (64) characters
- Verify all strings fit within buffer with null terminator

### TEST-ETSF-007: xrResultToString - Extension Result Codes
**Must**: Runtime must return correct strings for enabled extension result codes
- Enable extension that adds result codes
- Call xrResultToString with extension result code
- Verify correct extension result string returned

### TEST-ETSF-008: xrStructureTypeToString - Valid Structure Types
**Must**: Runtime must return correct string for valid XrStructureType values
- Call xrStructureTypeToString with XR_TYPE_INSTANCE_CREATE_INFO
- Verify buffer contains "XR_TYPE_INSTANCE_CREATE_INFO"
- Test various core structure types

### TEST-ETSF-009: xrStructureTypeToString - Unknown Structure Type
**Must**: Runtime must return "XR_UNKNOWN_STRUCTURE_TYPE_" for unknown values
- Call xrStructureTypeToString with unknown structure type value
- Verify buffer contains "XR_UNKNOWN_STRUCTURE_TYPE_" followed by decimal number

### TEST-ETSF-010: xrStructureTypeToString - Invalid Instance Handle
**Must**: Runtime must return XR_ERROR_HANDLE_INVALID for invalid instance
- Call xrStructureTypeToString with invalid instance handle
- Verify XR_ERROR_HANDLE_INVALID is returned

### TEST-ETSF-011: xrStructureTypeToString - Buffer Size
**Must**: Runtime must respect XR_MAX_STRUCTURE_NAME_SIZE buffer size
- Verify buffer is exactly XR_MAX_STRUCTURE_NAME_SIZE (64) characters
- Verify all strings fit within buffer with null terminator

### TEST-ETSF-012: xrStructureTypeToString - Extension Structure Types
**Must**: Runtime must return correct strings for enabled extension structure types
- Enable extension that adds structure types
- Call xrStructureTypeToString with extension structure type
- Verify correct extension structure string returned

### TEST-ETSF-013: String Functions - Instance Lost
**Must**: Runtime must return XR_ERROR_INSTANCE_LOST after instance loss
- Trigger instance loss
- Call xrResultToString and xrStructureTypeToString
- Verify XR_ERROR_INSTANCE_LOST is returned

---

## Concurrent Instances

### TEST-CI-001: Multiple Instances - Minimum Support
**Must**: Runtime must support at least one instance per process
- Create one instance
- Verify XR_SUCCESS is returned
- Verify instance is fully functional

### TEST-CI-002: Multiple Instances - Maximum Allowed
**May**: Runtime may support multiple concurrent instances
- Create first instance successfully
- Attempt to create second instance
- If XR_SUCCESS, verify both instances are independent and functional
- If XR_ERROR_LIMIT_REACHED, document limit as 1
- Destroy instances in reverse order

### TEST-CI-003: Multiple Instances - Independence
**Must**: If multiple instances supported, they must be independent
- Create multiple instances (if supported)
- Create child objects in each instance
- Verify operations on one instance don't affect others
- Verify handles from one instance don't work with another

### TEST-CI-004: Multiple Instances - Simultaneous Operations
**Must**: If multiple instances supported, simultaneous operations must work
- Create multiple instances
- Perform operations on both concurrently
- Verify both work correctly without interference

### TEST-CI-005: Multiple Instances - Sequential Creation
**Must**: Runtime must allow creating instances sequentially
- Create instance
- Destroy instance
- Create new instance
- Verify XR_SUCCESS is returned
- Verify new instance is fully functional

### TEST-CI-006: Multiple Instances - Resource Isolation
**Must**: If multiple instances supported, resources must be isolated
- Create multiple instances
- Exhaust resources in one instance (if possible)
- Verify other instances still functional

---

## State Tracking and Object Relationships

### TEST-STOR-001: Instance Parent Tracking
**Must**: Runtime must track instance as parent of child objects
- Create instance
- Create session from instance
- Verify session operations use correct instance
- Verify loader/runtime tracks parent-child relationship

### TEST-STOR-002: Instance State in API Calls
**Must**: Intended instance must be inferred from highest ascendant
- Create instance and session
- Call xrEndFrame on session
- Verify runtime correctly infers instance from session
- Verify no need to pass instance explicitly

### TEST-STOR-003: Instance State Storage
**May**: Instance state may be stored in loader, layers, or runtime
- Document that exact storage is implementation-dependent
- Verify state is accessible regardless of where stored

---

## Edge Cases and Error Conditions

### TEST-EC-001: NULL Handle Parameters
**Must**: Runtime must return XR_ERROR_HANDLE_INVALID for NULL instance handles
- Call functions requiring instance handle with XR_NULL_HANDLE
- Verify XR_ERROR_HANDLE_INVALID is returned

### TEST-EC-002: Out of Memory
**Must**: Runtime must return XR_ERROR_OUT_OF_MEMORY when resources exhausted
- Attempt to create instance when memory exhausted (if testable)
- Verify XR_ERROR_OUT_OF_MEMORY is returned

### TEST-EC-003: Runtime Failure
**Must**: Runtime must return XR_ERROR_RUNTIME_FAILURE for internal errors
- Document that runtime may return this for unexpected internal errors
- Verify error is returned for runtime-specific failure conditions

### TEST-EC-004: Validation Failure
**Must**: Runtime must return XR_ERROR_VALIDATION_FAILURE for validation errors
- Pass invalid parameters (NULL pointers, invalid structure types)
- Verify XR_ERROR_VALIDATION_FAILURE is returned

### TEST-EC-005: Runtime Unavailable
**Must**: Runtime must return XR_ERROR_RUNTIME_UNAVAILABLE when runtime cannot be loaded
- Test xrCreateInstance when runtime is not available
- Verify XR_ERROR_RUNTIME_UNAVAILABLE is returned

### TEST-EC-006: Initialization Failed
**Must**: Runtime must return XR_ERROR_INITIALIZATION_FAILED for initialization errors
- Test scenarios where initialization cannot complete
- Verify XR_ERROR_INITIALIZATION_FAILED is returned

### TEST-EC-007: UTF-8 String Validation
**Must**: Runtime must validate UTF-8 strings in API layer and extension names
- Pass invalid UTF-8 sequences in layer/extension names
- Verify appropriate error is returned

### TEST-EC-008: String Array Validation
**Must**: Runtime must validate string arrays for layer and extension names
- Pass invalid string array pointers
- Verify XR_ERROR_VALIDATION_FAILURE is returned

---

## Implicitly vs Explicitly Enabled Layers

### TEST-IEL-001: Implicitly Enabled Layers - Function Equivalence
**Must**: Implicitly and explicitly enabled layers must function equivalently
- Enable layer implicitly (via loader mechanism if available)
- Enable same layer explicitly in enabledApiLayerNames
- Verify both produce same behavior

### TEST-IEL-002: Explicitly Enabling Implicit Layer
**Must**: Explicitly enabling already-implicit layer must have no additional effect
- Enable layer implicitly
- Also enable same layer explicitly
- Verify no error and no duplicate behavior

---

## Test Execution Guidelines

### Prerequisites
- OpenXR loader must be available and functional
- Test environment must allow instance creation and destruction
- Ability to enumerate available API layers and extensions
- Ability to trigger error conditions (if possible)

### Test Data Requirements
- Valid application names and engine names
- Valid API versions (XR_API_VERSION_1_0, XR_API_VERSION_1_1)
- Known API layer names (from enumeration)
- Known extension names (from enumeration)
- Invalid/unknown layer and extension names for negative testing

### Validation Approach
- Each "Must" requirement should be a separate automated test
- Each "Should" requirement should be tested and documented if not supported
- Each "May" requirement should be documented as allowed behavior
- Return codes must be checked against expected values
- Structure fields must be validated for correct values
- String lengths and null terminators must be validated
- Buffer size parameters must follow two-call idiom correctly

### Coverage Requirements
- All success paths (valid instance creation, property queries, etc.)
- All error paths (invalid parameters, missing layers/extensions, etc.)
- Two-call idiom for all enumeration functions
- Buffer size validation (sufficient, insufficient, zero)
- Instance lifecycle (create, use, destroy)
- Instance loss scenarios
- Multiple instances (if supported)
- Thread safety requirements (documentation)
- Platform-specific requirements (Android, etc.)

---

## Notes

- This document covers OpenXR 1.0/1.1 core specification section 4
- Platform-specific extensions (e.g., XR_KHR_android_create_instance) add additional requirements
- Some tests may require platform-specific test harnesses
- Runtime unavailability and instance loss may be difficult to test automatically
- Concurrent instance support is optional; tests should adapt to runtime capabilities
- String functions must support both core and extension enum values
- Two-call idiom must be tested thoroughly for all enumeration functions
- Instance parent-child relationship tracking is critical for loader/runtime architecture

