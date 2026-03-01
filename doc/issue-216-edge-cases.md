# Issue #216 Multi-Sync Input — Edge Cases & Verification Notes

## Automated checks available in this environment

- `cmake` is not installed in the current runner, so local compile/test execution is blocked.
- Validation is therefore limited to source-level verification plus CI-on-PR.

## Edge cases covered by this patch

1. **Persist user preference**
   - New config key: `MultiSyncInput` (`K_MULTI_SYNC_INPUT` / `V_MULTI_SYNC_INPUT`).
   - Read at startup and restored in menu check state.
   - Written immediately when toggled and again during shutdown.

2. **Focus-loss safety**
   - Existing logic disables Multi-Sync when app loses focus.
   - With persistence writeback in place, disabled state is retained across restart.

3. **No-op safety when null/invalid connections**
   - Dispatch paths return early if source is null.
   - Mirroring only runs with active source tab and at least 2 connections.

4. **Coordinate bounds on heterogeneous resolutions**
   - Mirrored pointer coordinates are scaled and clamped in `[0..width-1]` / `[0..height-1]`.

## PR CI expectation

- Upstream GitHub Actions (CI workflow) should provide authoritative build/test evidence after PR submission.
