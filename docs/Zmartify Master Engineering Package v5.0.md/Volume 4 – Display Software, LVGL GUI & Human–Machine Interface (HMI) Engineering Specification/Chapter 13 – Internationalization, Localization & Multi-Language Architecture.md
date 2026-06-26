# Zmartify Master Engineering Package v5.0

# Volume 4

# Display Software, LVGL GUI & Human–Machine Interface (HMI) Engineering Specification

## Chapter 13

# Internationalization, Localization & Multi-Language Architecture

---

# 13.1 Purpose

This chapter defines the Internationalization (i18n) and Localization (l10n) architecture for the Zmartify Human–Machine Interface.

The objective is to ensure that the controller can be deployed globally without requiring firmware modifications for different languages, regions or measurement systems.

The architecture shall support:

* Multiple languages
* Regional settings
* Local date/time formats
* Unit conversion
* Unicode character sets
* Right-to-left language support (future)
* OEM-specific translations

Internationalization shall be implemented independently of application logic.

---

# 13.2 Design Objectives

The localization framework shall:

* Separate text from firmware
* Support runtime language switching
* Support Unicode
* Minimize RAM usage
* Support future languages
* Enable OTA language updates
* Preserve screen layouts
* Remain independent of display resolution

---

# 13.3 Architecture

```text id="i18n-architecture"
Application Managers

        │

        ▼

Language Manager

        │

        ▼

Translation Database

        │

        ▼

Widget Manager

        │

        ▼

LVGL Labels
```

Application Managers shall never contain user-visible text.

---

# 13.4 Design Philosophy

The localization system follows five principles.

### LNG-001

Language Independent Firmware

Firmware shall never contain hardcoded UI text.

---

### LNG-002

Stable Translation Keys

Translation identifiers shall never change after release.

---

### LNG-003

Runtime Switching

Language changes shall occur without restarting the controller.

---

### LNG-004

Regional Awareness

Language and regional formatting shall be independent.

---

### LNG-005

Single Source of Truth

Every user-visible string shall originate from a centralized translation database.

---

# 13.5 Language Manager

The Language Manager is responsible for:

* Loading translation resources
* Selecting active language
* Updating widgets
* Handling fallback translations
* Managing regional formats
* Providing translation APIs
* Synchronizing language changes

The Language Manager shall be initialized during system startup.

---

# 13.6 Supported Languages

Initial release:

| Language | Code |
| -------- | ---- |
| English  | en   |
| Danish   | da   |
| German   | de   |
| French   | fr   |
| Dutch    | nl   |

Planned:

* Spanish
* Italian
* Polish
* Swedish
* Norwegian
* Finnish
* Portuguese
* Czech

Future languages shall not require firmware recompilation.

---

# 13.7 Translation Keys

All user-visible text shall use symbolic identifiers.

Example:

```text id="translation-keys"
TXT_START

TXT_STOP

TXT_CANCEL

TXT_SETTINGS

TXT_WEATHER

TXT_IRRIGATION

TXT_FLOW

TXT_PRESSURE

TXT_WARNING

TXT_ALARM
```

Widgets shall request translated strings by key rather than by literal text.

---

# 13.8 Translation Database

Example structure:

```json id="translation-example"
{
    "TXT_START": {
        "en": "Start",
        "da": "Start",
        "de": "Start",
        "fr": "Démarrer"
    }
}
```

Translation resources may be compiled into firmware or loaded from external storage.

---

# 13.9 Runtime Language Switching

Workflow:

```text id="language-switch"
User Selects Language

↓

Language Manager

↓

Load Resources

↓

Update Widgets

↓

Redraw Active Screens

↓

Save Preference
```

Screen objects shall not be recreated during language changes.

---

# 13.10 Fallback Strategy

If a translation is unavailable:

1. Use the selected language.
2. If unavailable, use English.
3. If unavailable, display the translation key.
4. Log a translation warning.

Example:

```text id="fallback-example"
TXT_WEATHER
```

Fallback behavior shall never prevent UI rendering.

---

# 13.11 Regional Settings

Regional configuration includes:

* Language
* Country
* Time Zone
* Calendar
* Number Format
* Units
* Currency (future)

Language selection shall not automatically change engineering units.

---

# 13.12 Date & Time Formats

Supported formats:

| Region | Example    |
| ------ | ---------- |
| ISO    | 2026-07-14 |
| Europe | 14-07-2026 |
| US     | 07/14/2026 |

Time formats:

```text id="time-formats"
24-hour

14:35
```

or

```text id="time-format-12"
12-hour

2:35 PM
```

---

# 13.13 Number Formatting

Examples:

English:

```text id="english-number"
1,234.56
```

European:

```text id="europe-number"
1.234,56
```

Formatting shall follow the active regional profile.

---

# 13.14 Unit Localization

Supported engineering units:

| Parameter   | Metric | Imperial |
| ----------- | ------ | -------- |
| Temperature | °C     | °F       |
| Pressure    | bar    | psi      |
| Flow        | L/min  | GPM      |
| Rain        | mm     | inches   |
| Wind        | m/s    | mph      |
| Water       | Liters | Gallons  |

Internal calculations shall always use SI units.

Unit conversion occurs only within the presentation layer.

---

# 13.15 Unicode Support

The HMI shall support UTF-8 encoding throughout the interface.

Examples:

* Danish: Ø, Å, Æ
* German: Ä, Ö, Ü
* French: É, È, Ç
* Spanish: Ñ
* Polish: Ł, Ż

All fonts shall include required glyphs for supported languages.

---

# 13.16 Text Expansion

Different languages require varying text lengths.

Example:

| English     | German                |
| ----------- | --------------------- |
| Settings    | Einstellungen         |
| Weather     | Wetter                |
| Diagnostics | Diagnoseinformationen |

Layouts shall allow approximately 30% text expansion without clipping.

---

# 13.17 Right-to-Left Support (Future)

The architecture reserves support for RTL languages.

Future capabilities:

* Arabic
* Hebrew

Requirements:

* Mirrored layouts
* Reversed navigation
* RTL typography
* RTL widget alignment

Version 5.0 remains left-to-right only.

---

# 13.18 Icon Independence

Icons shall not contain embedded language.

Example:

Correct:

```text id="icon-correct"
⚙
```

Incorrect:

```text id="icon-text"
Settings
```

Text shall remain separate from icons.

---

# 13.19 Language Persistence

Selected language shall be stored in non-volatile memory.

The controller shall restore the user's preferred language after reboot.

Default language:

```text id="default-language"
English
```

OEM firmware may specify an alternative default.

---

# 13.20 Translation Quality

Translation requirements:

* Technical accuracy
* Consistent terminology
* Engineering terminology preserved
* No automatic machine translation in production releases without review

Glossaries shall be maintained for engineering terms such as:

* Flow
* Pressure
* Water Budget
* Evapotranspiration
* Hydraulic Health

---

# 13.21 OTA Language Updates

Future firmware versions may support independent language package updates.

Workflow:

```text id="language-update"
Download Language Pack

↓

Verify Integrity

↓

Install

↓

Activate

↓

Reload UI
```

Language updates shall not modify firmware binaries.

---

# 13.22 Accessibility

Localization shall support:

* Larger fonts
* Simplified language packs (future)
* Screen reader compatibility (future)
* High-contrast themes
* User-adjustable text scaling

Text scaling shall preserve layout integrity.

---

# 13.23 Developer Guidelines

Developers shall:

* Never hardcode visible text
* Always use translation keys
* Avoid text embedded in graphics
* Keep strings concise
* Reuse existing keys whenever possible
* Document new translation keys

All new UI components shall be localization-ready.

---

# 13.24 Performance Requirements

| Operation          |  Target |
| ------------------ | ------: |
| Language Switch    | <500 ms |
| Widget Update      |  <10 ms |
| Screen Refresh     | <150 ms |
| Translation Lookup |   <1 ms |
| Memory Overhead    | <2% RAM |

Language switching shall not interrupt active irrigation or controller operation.

---

# 13.25 Future Extensions

The localization framework supports future capabilities including:

* Downloadable language packs
* Community translations
* Voice prompts
* Text-to-speech integration
* AI-assisted translation validation
* Customer-specific terminology
* Multi-language user profiles
* Cloud-synchronized language preferences

These enhancements shall integrate through the Language Manager without affecting application logic.

---

# 13.26 Engineering Notes

The internationalization architecture ensures that the Zmartify controller can be deployed globally while maintaining a single firmware codebase. By separating translation resources, regional formatting and engineering unit conversion from application logic, the platform significantly reduces maintenance effort and simplifies future market expansion.

The use of stable translation keys and centralized language management also enables efficient OTA language updates and OEM customization without impacting the integrity of the firmware.

---

# 13.27 Chapter Summary

This chapter defines the Internationalization and Localization framework for the Zmartify Human–Machine Interface.

The architecture provides comprehensive support for multiple languages, regional formats, engineering units and future global deployments while preserving a clean separation between application logic and presentation. Together with the Theme Engine and Widget Library, it establishes the final layer required for a truly international and scalable HMI platform.

---

# End of Chapter 13

**Next Chapter**

**Chapter 14 – Animation Framework, Screen Transitions & Visual Feedback System**
