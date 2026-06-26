# Zmartify Master Engineering Package v5.0

# Volume 4

# Display Software, LVGL GUI & Human–Machine Interface (HMI) Engineering Specification

## Chapter 17

# HMI Testing, UI Verification & Automated GUI Validation

---

# 17.1 Purpose

This chapter defines the verification and validation strategy for the Zmartify Human–Machine Interface (HMI).

The objective is to ensure that every screen, widget, animation and user interaction performs consistently across all supported hardware platforms while preserving deterministic controller operation.

Unlike purely cosmetic testing, HMI verification forms part of the overall product validation process and is therefore considered a mandatory engineering activity.

The framework supports:

* Functional verification
* UI regression testing
* Performance testing
* Accessibility validation
* Usability verification
* Stress testing
* Endurance testing
* Automated GUI validation

---

# 17.2 Design Objectives

The HMI validation framework shall:

* Verify every screen
* Verify every widget
* Detect graphical regressions
* Measure performance
* Validate usability
* Support automated testing
* Produce repeatable results
* Integrate into CI/CD pipelines

---

# 17.3 Verification Philosophy

The verification process follows five principles.

### TEST-001

Every Screen is Testable

Each screen shall have defined acceptance criteria.

---

### TEST-002

Repeatability

Every test shall produce repeatable results.

---

### TEST-003

Automation First

Automated testing shall replace manual testing wherever practical.

---

### TEST-004

Measure Everything

Graphical performance shall be measurable rather than subjective.

---

### TEST-005

Regression Prevention

Every released feature shall remain continuously validated.

---

# 17.4 Testing Architecture

```text id="testing-architecture"
Test Framework

        │

        ▼

GUI Automation Engine

        │

        ▼

Display Manager

        │

        ▼

LVGL

        │

        ▼

Display Driver

        │

        ▼

Hardware
```

Automated testing shall interact with the Display Manager rather than directly manipulating widgets.

---

# 17.5 Test Categories

The HMI verification suite consists of:

| Category      | Purpose                   |
| ------------- | ------------------------- |
| Functional    | UI correctness            |
| Visual        | Layout verification       |
| Performance   | Speed & responsiveness    |
| Accessibility | Usability                 |
| Stress        | High-load operation       |
| Endurance     | Long-term stability       |
| Regression    | Prevent previous failures |
| Integration   | Firmware interaction      |

---

# 17.6 Functional Testing

Each screen shall verify:

* Screen creation
* Widget creation
* Navigation
* Data updates
* Event handling
* Dialog behavior
* Error handling

No screen shall be released without passing functional verification.

---

# 17.7 Widget Verification

Every widget shall be tested for:

* Creation
* Destruction
* Theme updates
* Data binding
* Touch interaction
* Accessibility
* Performance
* Error recovery

Example checklist:

```text id="widget-test"
Create

Update

Enable

Disable

Destroy
```

---

# 17.8 Navigation Testing

The navigation framework shall verify:

* Screen transitions
* Navigation history
* Back navigation
* Home navigation
* Modal dialogs
* Navigation shortcuts

Every navigation path shall terminate correctly.

---

# 17.9 Touch Interaction Testing

Touch verification includes:

* Tap
* Double Tap (reserved)
* Long Press
* Drag
* Swipe
* Scroll

Edge cases:

* Rapid tapping
* Simultaneous updates
* Invalid coordinates
* Lost touch events

---

# 17.10 Screen Verification Matrix

Each screen shall verify:

| Requirement        | Status |
| ------------------ | :----: |
| Loads Successfully |    □   |
| Correct Widgets    |    □   |
| Correct Theme      |    □   |
| Data Updates       |    □   |
| Navigation Works   |    □   |
| Performance OK     |    □   |

The matrix shall be completed for every released screen.

---

# 17.11 Theme Verification

Each supported theme shall verify:

* Colors
* Typography
* Icons
* Contrast
* Widget spacing
* Focus indicators
* Dialog styling

Visual consistency shall be maintained across all themes.

---

# 17.12 Localization Testing

Every supported language shall verify:

* Translation completeness
* UTF-8 rendering
* Text expansion
* No clipping
* Correct formatting
* Layout integrity

Languages shall be tested independently.

---

# 17.13 Accessibility Verification

Accessibility tests include:

* Touch target size
* Contrast ratio
* Font scaling
* Reduced-motion mode
* Color-independent indicators

Minimum touch target:

```text id="accessibility-target"
48 × 48 px
```

---

# 17.14 Performance Testing

Measured metrics include:

| Metric        |  Target |
| ------------- | ------: |
| FPS           |     ≥60 |
| Touch Latency |  <50 ms |
| Screen Change | <150 ms |
| Widget Update |  <10 ms |
| Dialog Open   | <180 ms |

Performance shall be measured using the Diagnostics framework.

---

# 17.15 Stress Testing

Stress scenarios:

* Rapid screen switching
* Multiple alarms
* Continuous flow updates
* Simultaneous MQTT events
* Theme changes
* Language switching
* OTA progress

The GUI shall remain responsive throughout testing.

---

# 17.16 Endurance Testing

Continuous runtime:

```text id="endurance-runtime"
30 Days

Minimum

365 Days

Target
```

Monitor:

* Heap usage
* Widget count
* Animation count
* FPS
* Queue depth

Memory growth shall remain negligible.

---

# 17.17 Memory Leak Detection

Validation includes:

* Widget allocation
* Widget destruction
* Screen cache
* Dialog lifecycle
* Animation lifecycle

Repeated screen creation shall not increase memory consumption.

---

# 17.18 Automated Screenshot Testing

Reference screenshots shall be maintained for:

* Every screen
* Every theme
* Every language
* Every dialog
* Every alarm

Automated comparison shall detect:

* Layout changes
* Missing widgets
* Incorrect colors
* Clipped text
* Rendering regressions

---

# 17.19 Event Injection Testing

The framework shall inject:

* Zone Started
* Zone Completed
* Flow Alarm
* Pressure Alarm
* Weather Update
* MQTT Disconnect
* OTA Update
* Configuration Change

The HMI shall respond correctly to every event.

---

# 17.20 Alarm Verification

Every alarm shall verify:

* Popup appearance
* Correct severity
* Color
* Icon
* Recommended action
* Acknowledgement
* History entry

Critical alarms shall interrupt normal workflow appropriately.

---

# 17.21 Recovery Testing

Failure scenarios include:

* Display disconnect
* Touch failure
* Memory exhaustion
* Queue overflow
* LVGL assertion
* Driver restart

The GUI shall recover automatically whenever possible.

---

# 17.22 Integration Testing

Integration verification includes:

* Display Manager
* Event Bus
* MQTT updates
* Application Managers
* OTA Manager
* Configuration Manager

Cross-module communication shall remain deterministic.

---

# 17.23 CI/CD Integration

Automated GUI validation shall execute:

* On every commit
* Before firmware release
* During nightly builds
* During release candidate validation

Regression failures shall block release approval.

---

# 17.24 Acceptance Criteria

A release candidate shall satisfy:

| Requirement         | Pass |
| ------------------- | :--: |
| Functional Tests    |   ✔  |
| Widget Tests        |   ✔  |
| Navigation Tests    |   ✔  |
| Accessibility Tests |   ✔  |
| Performance Tests   |   ✔  |
| Endurance Tests     |   ✔  |
| Regression Tests    |   ✔  |

No critical test failures are permitted.

---

# 17.25 Engineering Dashboard

Diagnostic metrics include:

```text id="gui-test-dashboard"
Screens Tested

48

Widgets Tested

212

Pass Rate

100 %

FPS

60

Heap

41 %

Coverage

98.7 %
```

This dashboard is available in Service Mode.

---

# 17.26 Future Extensions

The testing framework supports:

* AI-assisted visual regression
* Automated usability scoring
* Eye-tracking validation
* Cloud-based device farms
* Hardware-in-the-loop GUI testing
* Multi-display synchronization testing

Future capabilities shall integrate with the existing verification architecture.

---

# 17.27 Relationship to Other Volumes

This chapter supports:

| Volume    | Relationship                   |
| --------- | ------------------------------ |
| Volume 2  | Firmware integration testing   |
| Volume 5  | Hardware validation            |
| Volume 6  | Production acceptance testing  |
| Volume 7  | System verification procedures |
| Volume 10 | SDK demonstration applications |

Volume 7 expands these principles into complete product qualification procedures.

---

# 17.28 Engineering Notes

The HMI verification strategy treats the graphical interface as a critical subsystem rather than a cosmetic feature. Every widget, interaction and animation is therefore subject to measurable acceptance criteria, enabling repeatable validation throughout the product lifecycle.

By combining automated testing, performance measurement and long-duration endurance validation, the framework ensures that future firmware revisions preserve both functionality and user experience while minimizing regression risk.

---

# 17.29 Chapter Summary

This chapter defines the verification and validation framework for the Zmartify Human–Machine Interface.

Through comprehensive functional, visual, performance and endurance testing, the HMI can be verified with the same engineering rigor as the underlying firmware. The resulting framework provides a reliable foundation for continuous development, automated quality assurance and long-term product evolution.

---

# End of Chapter 17

**Next Chapter**

**Chapter 18 – HMI Deployment, Manufacturing Configuration & Future Evolution**
