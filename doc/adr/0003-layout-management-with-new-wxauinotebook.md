# 3. Layout Management with new wxAuiNotebook

Date: 2026-03-29

## Status

Accepted

## Context

After using `wxAuiNotebook` which provides a tabbed interface as its
ground state similar to the previously used `wxNotebook`, MultiVNC has
gained the technical capability to split tabs into sub-frames within
the application window, enabling MDI-style layouts — side-by-side,
top/bottom splits, tiled grids, or any user-defined arrangement.

The question was how to expose this capability to users in a way that
is consistent, discoverable, and honest to the underlying technical
model.

### Options Considered

#### Option 1: Free drag only
Expose wxAuiNotebook's native drag-to-split behavior with no
additional UI.  Maximum flexibility, but poor discoverability and no
easy way to go back to a sane layout after dragging around.

#### Option 2: Free drag + "Tile / Untile" toggle
Supplement free drag with a one-click tile/untile action. Better
discoverability, but new connections are added to the active TabCtrl,
departing from the tiled view.

#### Option 3: Forced tiled mode (persistent)
A discrete "forced tiled mode" where all connections are auto-arranged
in a grid and new connections always add a new tile cell/TabCtrl.
Clean answer to the new-connection problem, but tiled mode degrades
back to tabbed behavior at higher connection counts — inconsistent UX.

#### Option 4: Tri-state mode switcher (Free / Tabbed / Tiled)
A View menu or toolbar switcher between three named modes free,
force-tabbed, force-tiled. Force-tiled has the same problems as in
Option 3.

## Decision

Adopt an extended Option 2, i.e. free move with added actions in a
tab's context menu for explicit discoverability:

* Split left
* Split right
* Split to top
* Split to bottom
* Arrange all as grid
* Reset all to tabs

The global View menu gains the global entries:

* Arrange all as grid
* Reset all to tabs

All of these are greyed out if there are less than two connections.
"Reset all to tabs" is also greyed out if all connections are already
in a single TabCtrl.

## Consequences

### Layout behavior
- The default state is the standard tab strip, unchanged from the
  existing UI in previous versions.
- Users can drag tabs to split them into panes in any arrangement they
  choose.
- The user's spatial layout is never auto-modified by the application
  — new connections always open as a tab in the currently active
  TabCtrl, leaving any existing split arrangement undisturbed. This is
  at least not too bad: the new connection appears where the user's
  focus already is.
- "Reset all to tabs" collapses all panes back to the single tab
  strip.
- While the "Arrange all as grid" entry might create impractical
  layouts, the feedback loop is instant. The escape hatch is one click
  away as "Reset all to tabs" in the context menu and View menu.

### Discoverability
The free layout feature is not super self-evident; discoverability is
helped by adding explicit context menu items, a natural place users
look first.

### Context menu and global menu placement

All layout actions are surfaced in the per-tab context menu as the
primary discoverability surface — the context menu contains:

- "Split left" / "Split right" / "Split to top" / "Split to bottom" —
  per-tab actions that move this specific tab into a new pane in the
  chosen direction
- "Arrange all as grid" — a one-shot command that arranges all
  currently open connections into a grid at that moment; not a
  persistent mode
- "Reset all to tabs" — collapses all panes back to the single tab
  strip

The two global actions ("Arrange all as grid" and "Reset all to tabs")
operate on all connections rather than just the right-clicked
tab. Their wording makes this explicit, resolving the conceptual
mismatch of global actions appearing in a per-tab menu.

"Reset all to tabs" and "Arrange all as grid" are additionally
available in the View menu since they are the primary escape hatch and
layout shortcut respectively and should be findable without
right-clicking a tab.
