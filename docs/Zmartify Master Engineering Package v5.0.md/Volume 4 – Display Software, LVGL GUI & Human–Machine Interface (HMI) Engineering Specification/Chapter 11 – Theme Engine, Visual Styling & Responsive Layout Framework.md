# Zmartify Master Engineering Package v5.0

# Volume 4

# Display Software, LVGL GUI & Human–Machine Interface (HMI) Engineering Specification

## Chapter 11

# Theme Engine, Visual Styling & Responsive Layout Framework

---

# 11.1 Purpose

This chapter defines the visual styling architecture of the Zmartify Human–Machine Interface (HMI).

The Theme Engine provides a centralized framework for managing colors, typography, spacing, icons and visual behavior, ensuring that the interface remains consistent across all screens while supporting multiple display modes, accessibility requirements and future product families.

Unlike simple skinning systems, the Theme Engine is an integral part of the UI architecture and shall support dynamic theme switching without requiring screen recreation.

---

# 11.2 Design Objectives

The Theme Engine shall:

* Maintain visual consistency
* Support multiple themes
* Support runtime theme switching
* Support responsive layouts
* Improve readability
* Minimize rendering overhead
* Support accessibility
* Enable future product branding

---

# 11.3 Theme Architecture

The Theme Engine consists of four logical layers.

```text id="theme-architecture"
Application State

        │

        ▼

 Theme Manager

        │

        ▼

 Theme Definition

        │

        ▼

 Widget Style Engine

        │

        ▼

 LVGL Style Objects
```

Only the Theme Manager may modify active themes.

---

# 11.4 Theme Philosophy

The Zmartify design language follows these principles.

### THM-001

Function Before Decoration

Visual elements shall support usability rather than aesthetics alone.

---

### THM-002

Consistent Meaning

A color, icon or animation shall always convey the same meaning.

---

### THM-003

Minimal Cognitive Load

The interface shall reduce visual clutter and emphasize operational information.

---

### THM-004

Accessible by Design

Every theme shall remain usable under varying lighting conditions and by users with reduced vision.

---

# 11.5 Theme Components

Each theme defines:

* Color palette
* Typography
* Icons
* Widget styling
* Elevation
* Shadows
* Corner radius
* Animation timing
* Spacing
* Transparency

Themes shall not alter application behavior.

---

# 11.6 Supported Themes

The controller shall support the following themes.

| Theme         | Purpose                   |
| ------------- | ------------------------- |
| Light         | Default daytime operation |
| Dark          | Low-light environments    |
| High Contrast | Accessibility             |
| Service       | Engineering diagnostics   |

Future themes:

* Outdoor Sunlight
* OEM Branding
* Corporate Theme
* Installer Theme

---

# 11.7 Color System

The color system is semantic rather than decorative.

| Color Role  | Meaning             |
| ----------- | ------------------- |
| Primary     | Navigation          |
| Secondary   | Supporting actions  |
| Success     | Normal operation    |
| Warning     | Attention required  |
| Error       | Alarm               |
| Information | Neutral information |
| Disabled    | Inactive elements   |
| Background  | Screen background   |
| Surface     | Cards and dialogs   |

Widgets reference semantic colors rather than fixed RGB values.

---

# 11.8 Standard Color Palette

Reference palette:

| Purpose            | Suggested Color        |
| ------------------ | ---------------------- |
| Primary            | Blue                   |
| Success            | Green                  |
| Warning            | Amber                  |
| Error              | Red                    |
| Information        | Cyan                   |
| Background (Light) | White                  |
| Background (Dark)  | Near Black             |
| Surface            | Light Gray / Dark Gray |

Exact RGB values shall be defined in the implementation package.

---

# 11.9 Typography System

Standard font family:

```text id="font-family"
LVGL Montserrat
```

Future support:

* Noto Sans
* Roboto
* OEM Fonts

Typography hierarchy:

| Style   |  Size |
| ------- | ----: |
| Display | 36 px |
| Heading | 28 px |
| Section | 22 px |
| Body    | 18 px |
| Caption | 14 px |
| Status  | 16 px |

---

# 11.10 Iconography

Icons shall adhere to the following principles:

* Simple geometry
* Uniform stroke width
* Consistent proportions
* Recognizable at small sizes
* Theme independent

Standard icon categories:

* Irrigation
* Weather
* Hydraulics
* Network
* MQTT
* Settings
* Diagnostics
* Notifications
* Security
* Service

---

# 11.11 Spacing System

A standardized spacing grid shall be used throughout the interface.

Base spacing unit:

```text id="spacing-unit"
8 px
```

Recommended increments:

| Level |  Size |
| ----- | ----: |
| XS    |  4 px |
| S     |  8 px |
| M     | 16 px |
| L     | 24 px |
| XL    | 32 px |
| XXL   | 48 px |

Layouts shall avoid arbitrary spacing values.

---

# 11.12 Corner Radius

Standard corner radii:

| Widget       | Radius |
| ------------ | -----: |
| Button       |   8 px |
| Card         |  12 px |
| Dialog       |  16 px |
| Notification |  12 px |
| Input Field  |   8 px |

Rounded corners improve touch usability and visual consistency.

---

# 11.13 Elevation & Shadows

Elevation is used to distinguish interface layers.

| Element          | Elevation |
| ---------------- | --------: |
| Background       |         0 |
| Cards            |         1 |
| Dialogs          |         2 |
| Notifications    |         3 |
| Floating Buttons |         4 |

Shadows shall remain subtle and shall not reduce readability.

---

# 11.14 Responsive Layout Framework

The interface shall adapt automatically to supported display resolutions.

Reference resolutions:

| Resolution      | Status    |
| --------------- | --------- |
| 800 × 480       | Native    |
| 1024 × 600      | Supported |
| 1280 × 720      | Planned   |
| Future Displays | Scalable  |

Layouts shall prioritize proportional scaling over fixed coordinates.

---

# 11.15 Grid System

The responsive layout framework is based on a 12-column grid.

```text id="grid-layout"
|1|2|3|4|5|6|7|8|9|10|11|12|
```

Widgets may span one or more columns depending on available space.

This enables consistent layouts across multiple display sizes.

---

# 11.16 Breakpoints

Responsive breakpoints:

| Width       | Layout   |
| ----------- | -------- |
| ≤800 px     | Compact  |
| 801–1024 px | Standard |
| >1024 px    | Expanded |

Future products may introduce additional breakpoints.

---

# 11.17 Card Scaling

Cards shall resize proportionally while preserving:

* Minimum touch area
* Typography hierarchy
* Icon proportions
* Internal spacing

Information density shall increase on larger displays without reducing readability.

---

# 11.18 Adaptive Navigation

The navigation framework shall adapt according to display size.

Compact displays:

```text id="compact-nav"
Bottom Navigation
```

Standard displays:

```text id="standard-nav"
Top Navigation Bar
```

Large displays (future):

```text id="large-nav"
Navigation Rail + Dashboard
```

Navigation behavior remains identical across layouts.

---

# 11.19 Theme Switching

Theme changes shall occur dynamically.

Workflow:

```text id="theme-switch"
User Request

↓

Theme Manager

↓

Load Theme

↓

Update Styles

↓

Refresh Widgets

↓

Redraw Screen
```

Widget instances shall not be recreated during theme changes.

---

# 11.20 Day & Night Mode

Automatic theme switching may be based on:

* Time of day
* Ambient light sensor (future)
* User preference
* Manual override

Transitions shall be animated over approximately 300 ms.

---

# 11.21 Accessibility Themes

Accessibility features include:

* Increased contrast
* Larger typography
* Reduced animation
* Color-independent status indicators
* Enhanced focus outlines

Accessibility settings shall be stored per user profile where supported.

---

# 11.22 Animation Integration

Animations shall respect the active theme.

Examples:

* Fade transitions
* Card elevation
* Button press feedback
* Alarm pulse
* Notification slide-in

Animations shall be disabled or reduced when accessibility mode is enabled.

---

# 11.23 Resource Management

Theme resources include:

* Colors
* Fonts
* Icons
* Style definitions

These resources shall be loaded once during initialization and reused throughout the application to minimize memory consumption.

---

# 11.24 Branding Support

Future OEM products shall customize:

* Accent colors
* Logos
* Startup screen
* Fonts (optional)
* Product name

Brand customization shall not require modification of application logic or screen layouts.

---

# 11.25 Performance Requirements

| Operation           |  Target |
| ------------------- | ------: |
| Theme Load          | <100 ms |
| Theme Switch        | <300 ms |
| Widget Restyle      |  <10 ms |
| Full Screen Refresh | <150 ms |
| Memory Overhead     | <5% RAM |

Theme changes shall not interrupt ongoing controller operation.

---

# 11.26 Future Extensions

The Theme Framework supports future capabilities including:

* Dynamic wallpapers
* User-selectable accent colors
* Animated backgrounds
* Seasonal themes
* Automatic corporate branding
* OLED-optimized themes
* Multi-display synchronization

These enhancements shall preserve compatibility with the existing widget framework.

---

# 11.27 Engineering Notes

The Theme Engine provides more than visual customization—it establishes a consistent visual language across the entire Zmartify ecosystem. By separating style definitions from widget behavior, the platform enables future branding, accessibility improvements and display technologies without requiring changes to application logic.

The responsive layout framework further ensures that the same interface architecture can scale from embedded controllers to larger touch panels, desktop simulators and future cloud-based dashboards while maintaining a familiar user experience.

---

# 11.28 Chapter Summary

This chapter defines the Theme Engine, visual styling architecture and responsive layout framework for the Zmartify Human–Machine Interface.

By standardizing colors, typography, spacing, iconography and adaptive layouts, the Theme Engine provides a scalable and maintainable visual foundation that supports current hardware platforms while preparing the HMI for future display technologies and product families.

---

# End of Chapter 11

**Next Chapter**

**Chapter 12 – Touch Interaction, Gesture Recognition & Input Processing Framework**
