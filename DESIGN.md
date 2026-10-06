---
name: frei0r Live Filter Demo
description: A local, browser-hosted camera filtering demo with the live result as the primary task.
colors:
  background: "#ffeedd"
  ink: "#251e18"
  stage: "#1e1815"
  action: "#a95025"
  active: "#1b564d"
typography:
  display: "Syne, Inter, Segoe UI, Arial, sans-serif"
  body: "Inter, Segoe UI, Arial, sans-serif"
shape:
  surface-radius: "16px"
  control-radius: "0.5rem"
---

# Design System: frei0r Live Filter Demo

## Overview

The demo is an **Operate** surface for trying a browser camera filter locally. Its
mobile-first "stage + action dock" topology keeps the camera result dominant and
puts the next filter or parameter action within one-thumb reach. It deliberately
extends the documentation site's warm, editorial character rather than creating a
separate product identity.

## Colors

| Role | Token | Use |
| --- | --- | --- |
| Paper | `#ffeedd` | Page background |
| Ink | `#251e18` | Primary text and strong controls |
| Stage | `#1e1815` | Camera frame and overlay field |
| Action | `#a95025` | Start, retry, and navigation controls |
| Action deep | `#8d4720` | Hover actions, wordmark accent, links |
| Active | `#1b564d` / `#257265` | Camera-active signal, selected filter, focus |
| Mist | `#fffaf6` | Dock and control text |

Use the teal active treatment with the visible “Selected” label and a heavier
border; state is never color-only. The dark stage is reserved for camera content,
permission, and recovery messages.

## Typography

`Syne` carries the wordmark and headings with tight letter spacing. `Inter` is the
body and control face, with system sans-serif fallbacks. Headings use a compact,
high-contrast scale (`clamp(2rem, 5vw, 3.75rem)` for the page title) while helper
copy remains a readable, relaxed 1.5 line height.

## Layout and spacing

The mobile stage fills the dynamic viewport, including safe-area padding for
controls. The camera image covers the stage with edge cropping. Desktop uses a
wide stage occupying most of the viewport, within a 120rem shell. Detailed
controls open in a bounded, scrollable panel; the live frame remains the primary
surface, without a desktop sidebar.

## Components

- **Compact header and status:** the frei0r wordmark, concise camera status, and
  explicit active indicator establish trust before an action is requested.
- **Live stage:** a viewport-sized canvas with small, high-contrast overlays. It presents
  ready, requesting, paused, denied, unavailable, and runtime-recovery states in
  the same place as the resulting image.
- **Action dock:** filter name and explanation, Previous/Next controls, a
  horizontal snap rail, and a collapsible parameter sheet. During preview it is
  opened explicitly with Controls and closed using the persistent top button.
- **Preview overlays:** rendered FPS at top left, filter name at top center, and
  dominant parameter name/value near the bottom. Previous/Next and Stop remain
  available alongside the gesture hint.
- **Filter rail:** generously sized selectable cards with active centering and a
  count; each card exposes selected state in text as well as shape and color.
- **Parameter sheet:** a single expandable section with a reset action; zero
  parameters is an explanatory state, not an empty gap.

## Interaction, motion, and feedback

Filter changes run through the existing serialized runtime scheduler. The rail
works with touch, mouse, arrow-key navigation, and normal sequential focus.
Movement on the video locks to one axis: left advances, right returns, and vertical drags
continuously adjust an expressive parameter chosen for the current filter.
Up increases and down decreases normalized values, with keyboard equivalents.
Filters without adjustable parameters expose that state in the bottom overlay.
Motion is deliberately restrained: browser scrolling may snap the rail, but
`prefers-reduced-motion` removes smooth behaviour and animation duration. Status
messages are polite, action-oriented, and explain how to recover without moving
the user away from the stage. Quality, offline, and rapid-visual-change notices
are textual so they remain understandable without transient animation.

## Accessibility

Interactive buttons and links have a 2.75rem minimum height (44px). Keyboard focus
is a 3px teal outline with an offset; forced-colors mode replaces decorative
borders and shadows with system colors. The stage uses explicit labels for camera
state, permission recovery, and activity. The active filter combines border,
inset marker, and text label, and the dock’s document order stays practical for
switch control and screen-reader navigation.

## Responsive rules

The mobile composition starts with the full-viewport stage; the project header
and resource links follow it. Controls remain reachable over the video and the
panel respects the bottom safe area. At 48rem and above, the header precedes the
larger stage and the dock heading gains a two-column layout.

## Anti-patterns to avoid

Do not add a dashboard shell, a sidebar, autoplaying decorative motion, a permanent
filter palette over the video, or a new logo/visual language. Do not hide camera
permission failures behind browser-only wording, use color alone for state, or
trade away the persistent GitHub and Telegram links for a cleaner screenshot.
