# Zmartify Master Engineering Package v5.0

# Volume 4

# Display Software, LVGL GUI & Human–Machine Interface (HMI) Engineering Specification

## Chapter 14

# Animation Framework, Screen Transitions & Visual Feedback System

---

# 14.1 Purpose

This chapter defines the animation framework and visual feedback system used throughout the Zmartify Human–Machine Interface (HMI).

Animations are not decorative elements. Their primary purpose is to improve usability by communicating state changes, guiding user attention and providing immediate feedback while maintaining a responsive and professional interface.

The animation framework shall:

* Improve navigation
* Reinforce user actions
* Visualize controller state changes
* Highlight alarms
* Provide progress indication
* Preserve responsiveness
* Respect accessibility settings

---

# 14.2 Design Objectives

The animation system shall:

* Feel smooth and responsive
* Never delay user interaction
* Consume minimal CPU resources
* Support configurable animation levels
* Remain deterministic
* Scale to future hardware
* Integrate seamlessly with LVGL

---

# 14.3 Animation Philosophy

The Zmartify interface follows five animation principles.

### ANI-001

Function Before Decoration

Animations shall communicate information rather than entertain.

---

### ANI-002

Fast Response

Animations shall begin immediately after user interaction.

---

### ANI-003

Predictable Motion

Identical actions shall always produce identical animations.

---

### ANI-004

Minimal Distraction

Animations shall support the task without drawing unnecessary attention.

---

### ANI-005

Accessibility First

Animations shall be reducible or disabled for users who prefer minimal motion.

---

# 14.4 Animation Architecture

```text id="animation-architecture"
User Event

        │

        ▼

Display Manager

        │

        ▼

Animation Manager

        │

        ▼

LVGL Animation Engine

        │

        ▼

Display Driver
```

The Animation Manager shall coordinate all interface animations.

---

# 14.5 Animation Categories

The framework supports the following categories.

| Category     | Purpose                 |
| ------------ | ----------------------- |
| Navigation   | Screen transitions      |
| Feedback     | User interaction        |
| Status       | State changes           |
| Progress     | Long-running operations |
| Notification | Toasts and banners      |
| Alarm        | Attention guidance      |
| Decorative   | Reserved for future use |

---

# 14.6 Standard Animation Durations

| Animation         |   Duration |
| ----------------- | ---------: |
| Button Press      |      80 ms |
| Button Release    |      80 ms |
| Card Highlight    |     120 ms |
| Dialog Open       |     180 ms |
| Dialog Close      |     150 ms |
| Screen Transition |     200 ms |
| Notification      |     250 ms |
| Alarm Pulse       | Continuous |
| Progress Update   |     100 ms |

Animations shall not exceed 300 ms unless explicitly justified.

---

# 14.7 Screen Transitions

Supported transitions:

* Fade
* Slide Left
* Slide Right
* Slide Up
* Slide Down
* Cross Fade

Default navigation transition:

```text id="default-transition"
Slide Horizontal

Duration

200 ms
```

Transition direction shall reflect navigation direction.

---

# 14.8 Dialog Animations

Dialogs shall appear using:

* Fade In
* Scale Up

Workflow:

```text id="dialog-animation"
Background Dim

↓

Dialog Fade

↓

Scale

↓

Ready
```

Dialog dismissal reverses the sequence.

---

# 14.9 Button Feedback

Every button shall provide immediate visual confirmation.

Feedback includes:

* Color transition
* Slight scale reduction
* Shadow adjustment

Example:

```text id="button-feedback"
Touch

↓

Button Compresses

↓

Release

↓

Button Returns
```

Touch feedback shall begin within 50 ms.

---

# 14.10 Card Animations

Cards may animate during:

* Value updates
* Selection
* Expansion
* Alarm indication

Animations shall be subtle and avoid excessive movement.

---

# 14.11 Progress Indicators

Progress animations are used for:

* OTA updates
* Backup
* Restore
* Self-test
* Calibration
* Long-running diagnostics

Progress bars shall update smoothly without blocking the GUI thread.

---

# 14.12 Notification Animations

Notification workflow:

```text id="notification-animation"
Slide In

↓

Display

↓

Timeout

↓

Fade Out
```

Critical notifications remain visible until acknowledged.

---

# 14.13 Alarm Animations

Alarm animations guide user attention without becoming distracting.

Alarm effects:

* Pulsing border
* Gentle color transition
* Warning icon animation

Critical alarms shall not flash excessively to avoid user fatigue.

---

# 14.14 Live Data Updates

Real-time data shall animate intelligently.

Examples:

* Flow value changes
* Pressure updates
* Water usage
* Health score

Only the changed value shall animate.

Entire cards shall not be redrawn unnecessarily.

---

# 14.15 Chart Animations

Engineering charts shall support:

* Smooth scrolling
* Incremental updates
* Cursor movement
* Zoom transitions

Historical data loading may use fade transitions.

---

# 14.16 Navigation Feedback

Navigation interactions shall provide visual confirmation.

Examples:

* Active tab highlight
* Menu expansion
* Breadcrumb updates
* Navigation bar transitions

Users shall always understand their current location within the interface.

---

# 14.17 Loading States

Loading animations are required when operations exceed approximately:

```text id="loading-threshold"
300 ms
```

Examples:

* Network connection
* Weather download
* Configuration restore
* OTA preparation

Loading indicators shall display progress whenever measurable.

---

# 14.18 Skeleton Screens

Future versions may replace loading spinners with skeleton placeholders.

Benefits include:

* Improved perceived performance
* Reduced visual interruption
* Better layout continuity

Skeleton screens are planned for Version 6.x.

---

# 14.19 Animation Levels

Users may configure animation intensity.

| Level    | Description                 |
| -------- | --------------------------- |
| Full     | Complete animations         |
| Reduced  | Shortened animations        |
| Minimal  | Essential feedback only     |
| Disabled | No non-essential animations |

Accessibility settings override user preferences where required.

---

# 14.20 Performance Constraints

The Animation Manager shall:

* Maintain 60 FPS where possible
* Avoid excessive CPU usage
* Avoid unnecessary redraws
* Batch simultaneous animations

Target frame budget:

```text id="frame-budget"
16.7 ms
```

Per rendered frame.

---

# 14.21 Animation Queue

Simultaneous animations are coordinated by the Animation Manager.

```text id="animation-queue"
Animation Request

↓

Priority Assignment

↓

Queue

↓

Scheduler

↓

Execution

↓

Completion
```

High-priority alarm animations preempt decorative effects.

---

# 14.22 Synchronization

Animations shall synchronize with:

* Touch feedback
* Screen updates
* Widget state changes
* Controller events

Animations shall never delay application state changes.

---

# 14.23 Accessibility

Reduced-motion mode shall:

* Remove unnecessary movement
* Shorten transitions
* Preserve essential feedback
* Maintain usability

Critical status changes shall remain visually distinguishable without relying on motion.

---

# 14.24 Engineering Guidelines

Developers shall:

* Reuse existing animation definitions
* Avoid custom animation timings
* Prefer semantic animation names
* Keep transitions consistent
* Avoid simultaneous conflicting effects

Animation logic shall remain separate from application logic.

---

# 14.25 Future Extensions

The framework is designed to support:

* Physics-based transitions
* GPU acceleration (future hardware)
* Animated dashboards
* Adaptive animation speed
* AI-driven visual guidance
* Context-aware animations

These additions shall remain compatible with the existing Animation Manager.

---

# 14.26 Engineering Notes

The animation framework has been designed to reinforce user understanding rather than enhance aesthetics alone. Carefully controlled motion improves usability by clarifying navigation, confirming user actions and highlighting important state changes while avoiding unnecessary visual distraction.

Centralizing all animations within the Animation Manager also ensures consistent timing, reduced maintenance effort and efficient rendering performance on the ESP32-S3 platform.

---

# 14.27 Chapter Summary

This chapter defines the Animation Framework, screen transitions and visual feedback system for the Zmartify Human–Machine Interface.

By standardizing animations across navigation, interaction, notifications and alarms, the framework creates a responsive and intuitive user experience while maintaining deterministic behavior, accessibility and efficient resource utilization.

---

# End of Chapter 14

**Next Chapter**

**Chapter 15 – Performance Optimization, Memory Management & GUI Resource Architecture**
