# Zmartify Master Engineering Package v5.0

# Volume 4

# Display Software, LVGL GUI & Human–Machine Interface (HMI) Engineering Specification

## Chapter 12

# Touch Interaction, Gesture Recognition & Input Processing Framework

---

# 12.1 Purpose

This chapter defines the touch interaction architecture and input processing framework used by the Zmartify Human–Machine Interface.

The touch subsystem transforms raw touch controller events into deterministic user interactions while ensuring high responsiveness, predictable behavior and complete separation between user interface logic and irrigation control.

The framework supports:

* Single-touch interaction
* Multi-touch hardware compatibility
* Gesture recognition
* Touch filtering
* Debouncing
* Event routing
* Accessibility
* Future input devices

---

# 12.2 Design Objectives

The Input Processing Framework shall:

* Provide immediate user feedback
* Prevent accidental activation
* Filter noisy touch events
* Support responsive interaction
* Remain hardware independent
* Minimize latency
* Support future gesture extensions
* Never interfere with irrigation control

---

# 12.3 Input Architecture

```text id="input-architecture"
GT911 Touch Controller

          │

          ▼

ESP-IDF Touch Driver

          │

          ▼

LVGL Input Driver

          │

          ▼

Input Manager

          │

          ▼

Gesture Recognizer

          │

          ▼

Screen Manager

          │

          ▼

Widget Callback

          │

          ▼

Display Manager

          │

          ▼

Application Manager
```

Only the Display Manager shall communicate with the Application Managers.

---

# 12.4 Input Processing Philosophy

The touch subsystem follows four engineering principles.

### INP-001

Immediate Feedback

Every valid touch shall generate visible feedback within 50 ms.

---

### INP-002

Deterministic Processing

Identical user interactions shall always produce identical results.

---

### INP-003

No Hidden Gestures

Critical functionality shall never depend upon undocumented gestures.

---

### INP-004

Safe Operation

Potentially destructive actions shall require explicit confirmation.

---

# 12.5 Supported Input Types

Current implementation:

| Input      | Status    |
| ---------- | --------- |
| Single Tap | Mandatory |
| Long Press | Mandatory |
| Drag       | Mandatory |
| Scroll     | Mandatory |
| Swipe      | Mandatory |

Future implementation:

* Multi-touch
* Pinch Zoom
* Rotation
* Stylus
* External Mouse
* Keyboard
* Rotary Encoder

---

# 12.6 Touch States

Each touch point progresses through defined states.

```text id="touch-states"
Idle

↓

Touch Down

↓

Pressed

↓

Moved

↓

Released

↓

Idle
```

Each transition shall generate appropriate LVGL events.

---

# 12.7 Touch Sampling

Recommended sampling frequency:

```text id="sampling-rate"
100–200 Hz
```

Touch sampling shall remain independent of display refresh frequency.

The system shall support burst processing during rapid user interaction.

---

# 12.8 Event Queue

Touch events are processed asynchronously.

```text id="touch-queue"
Touch Driver

↓

Input Queue

↓

Input Manager

↓

Gesture Recognition

↓

Widget Dispatch
```

The event queue prevents blocking of the GUI task.

---

# 12.9 Debouncing

Touch events shall be filtered to prevent false activations.

Recommended debounce interval:

```text id="debounce"
20 ms
```

Debouncing shall occur before gesture recognition.

---

# 12.10 Hit Testing

Each touch event undergoes hit testing.

Processing order:

```text id="hit-test"
Touch Position

↓

Topmost Widget

↓

Visible?

↓

Enabled?

↓

Accept Touch?

↓

Dispatch Event
```

Invisible widgets shall never receive touch events.

---

# 12.11 Tap Recognition

A tap consists of:

* Touch Down
* Limited movement
* Touch Release

Maximum movement:

```text id="tap-distance"
10 px
```

Maximum duration:

```text id="tap-duration"
500 ms
```

---

# 12.12 Long Press Recognition

Long press activates secondary actions.

Threshold:

```text id="long-press"
800 ms
```

Typical applications:

* Context menus
* Engineering details
* Advanced configuration
* Service shortcuts

---

# 12.13 Swipe Recognition

Supported directions:

* Left
* Right
* Up
* Down

Minimum travel distance:

```text id="swipe-distance"
60 px
```

Typical applications:

| Swipe | Action          |
| ----- | --------------- |
| Left  | Next Screen     |
| Right | Previous Screen |
| Down  | Notifications   |
| Up    | Reserved        |

---

# 12.14 Scroll Processing

Scrollable widgets include:

* Lists
* Tables
* Charts
* Event Log
* Alarm History

Scrolling shall provide:

* Momentum
* Edge resistance
* Automatic deceleration

Overscroll effects shall remain subtle.

---

# 12.15 Drag Operations

Drag interactions support:

* Sliders
* Scrollbars
* Chart cursors
* Timeline selection

Dragging shall update continuously while maintaining 60 FPS where possible.

---

# 12.16 Future Multi-Touch Support

The architecture reserves support for:

* Pinch Zoom
* Two-finger Scroll
* Rotation
* Three-finger Shortcuts

These gestures are not required for Version 5.0.

---

# 12.17 Widget Focus

Input focus is managed centrally.

Only one widget may receive keyboard-style focus.

Focus sequence:

```text id="focus-sequence"
Previous Widget

↓

Lose Focus

↓

Next Widget

↓

Gain Focus
```

Focus indicators shall be theme aware.

---

# 12.18 Input Feedback

Every accepted interaction shall provide immediate feedback.

Feedback types:

* Button highlight
* Ripple animation
* Color transition
* Click animation
* Sound (optional)
* Haptic feedback (future)

Feedback shall never exceed:

```text id="feedback-latency"
50 ms
```

---

# 12.19 Confirmation Workflow

Critical operations require confirmation.

Examples:

* Factory Reset
* Delete Program
* Stop Irrigation
* Firmware Update
* Restore Backup

Workflow:

```text id="confirmation-workflow"
User Action

↓

Confirmation Dialog

↓

Cancel

or

Confirm

↓

Execute
```

---

# 12.20 Input Locking

During critical operations the interface may temporarily lock user input.

Examples:

* OTA installation
* Factory reset
* Configuration restore

Input locking shall display a progress indicator.

Emergency alarm acknowledgement shall remain available.

---

# 12.21 Touch Calibration

Capacitive touch calibration includes:

* Coordinate verification
* Edge detection
* Orientation
* Sensitivity

Calibration is available through Service Mode.

Factory calibration shall normally remain sufficient.

---

# 12.22 Error Handling

Input errors include:

* Lost touch events
* Invalid coordinates
* Driver timeout
* Controller communication failure

Recovery strategy:

```text id="input-recovery"
Detect Error

↓

Log Event

↓

Retry

↓

Reset Driver

↓

Restore Input
```

The GUI shall recover automatically whenever possible.

---

# 12.23 Accessibility

Touch interaction shall support:

* Large touch targets
* Reduced precision requirements
* High-contrast focus indicators
* Longer timeout options
* Reduced gesture dependency

Minimum touch target:

```text id="touch-target"
48 × 48 px
```

Preferred target:

```text id="preferred-touch"
56 × 56 px
```

---

# 12.24 Input Security

Administrative actions require authentication before execution.

Authentication shall occur after user selection but before the operation is committed.

Touch events alone shall never bypass authorization mechanisms.

---

# 12.25 Performance Requirements

| Metric              |  Target |
| ------------------- | ------: |
| Touch Detection     |  <10 ms |
| Touch Feedback      |  <50 ms |
| Gesture Recognition |  <20 ms |
| Screen Response     | <100 ms |
| Scroll Refresh      |  60 FPS |
| Drag Latency        |  <16 ms |

---

# 12.26 Future Extensions

The Input Framework supports future technologies including:

* Voice Commands
* Rotary Encoder Navigation
* Bluetooth Remote Controls
* External Keyboards
* Stylus Support
* NFC-Based Authentication
* Gesture Cameras
* Eye Tracking (research)

These additions shall integrate through the Input Manager without modifying application logic.

---

# 12.27 Engineering Notes

The Input Processing Framework has been designed to isolate physical input devices from the graphical user interface and application logic. By introducing dedicated input management, gesture recognition and centralized event routing, the platform achieves deterministic behavior while remaining adaptable to future interaction technologies.

The architecture also ensures that touch responsiveness remains consistent even under high processor load, preserving a high-quality user experience during simultaneous irrigation control, network communication and display updates.

---

# 12.28 Chapter Summary

This chapter defines the touch interaction architecture, gesture recognition system and input processing framework for the Zmartify Human–Machine Interface.

The framework provides responsive, predictable and extensible user interaction through centralized event processing, standardized gesture recognition and robust error handling. Together with the preceding chapters on navigation, widgets and themes, it completes the core interaction model upon which the remaining HMI functionality is built.

---

# End of Chapter 12

**Next Chapter**

**Chapter 13 – Internationalization, Localization & Multi-Language Architecture**
