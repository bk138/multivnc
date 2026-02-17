# Issue #216 MVP verification notes (Desktop)

## What was added

- New toggle in **View → Multi-Sync Input**.
- When enabled, keyboard and mouse input from the active viewer tab is sent to:
  - the active connection (existing behavior), and
  - all other open connections (new mirrored behavior).
- Toggle state is persisted in config key `MultiSyncInput`.

## Manual verification steps

1. Start MultiVNC desktop with at least two live VNC tabs connected.
2. Keep **View → Multi-Sync Input** disabled.
   - Move mouse / type in active tab.
   - Confirm only active connection receives input.
3. Enable **View → Multi-Sync Input**.
   - Move mouse / click / type in active tab.
   - Confirm all open connections receive the same events.
4. Change active tab and repeat step 3.
5. Restart app and verify toggle state persists.

## Known MVP limits

- Input fan-out is best effort and assumes similar remote UI state/screen geometry.
- No per-connection include/exclude list yet (all open tabs are targeted).
- No dedicated toolbar button yet; toggle is in View menu only.
